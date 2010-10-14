import os
import ctypes
import subprocess
from contextlib import nested
import logging
logger = logging.getLogger("imagedb")

import numpy as np

from danutils.ostools import chdir
from danutils.misc import replacedict
from danutils.lib import getlib
here = os.path.split(__file__)[0]
imagedb_so = getlib(here,'imagedb_wrap.so')

imagedb_so.imagedb_wrap.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

byte_types = {
    np.dtype("int8") : "8",
    np.dtype("int16") : "16",
    np.dtype("int32") : "32",
    np.dtype("uint8") : "8",
    np.dtype("uint16") : "16",
    np.dtype("uint32") : "32",
    np.dtype("float32") : "32f",
    np.dtype("float64") : "64f"
}

rebyte_map = {
    np.dtype('int64'): np.dtype('int32'),
    np.dtype('bool'): np.dtype('float32'),
}

field_types = {
    1 : "l",
    2 : "la",
    3 : "rgb",
    4 : "rgba",
}

def imagedb_compatible(img):
    """Coerce img to a type that imagedb knows how to handle.
    
    In particular, imagedb doesn't take bool or int64 arrays. This function
    will also copy and reorder e.g. images with the wrong endianness.
    """
    if img.dtype == np.dtype('bool'):
        return img.astype('uint8')*255
    order = img.dtype.byteorder
    kind = img.dtype.kind
    bytes = img.dtype.itemsize
    # force int arrays to unsigned
    kind = {'i':'u'}.get(kind, kind)
    # Can't handle int64 and up
    if kind == 'i' and bytes > 4:
        bytes = 4
    # Also can't handle float128 and up
    elif kind == 'f' and bytes > 8:
        bytes = 8
    new_str = '{order}{kind}{bytes}'.format(**locals())
    if new_str == img.dtype.str:
        return img # No change
    return img.astype(new_str)
    

def _ensure_imagedb():
    '''Compile imagedb if it's not already here '''
    imagedb_prog = os.path.join(here,'imagedb')
    if not os.path.exists(imagedb_prog):
        logger.info("imagedb executable is not present, making it")
        with nested(chdir(here), replacedict(os.environ, PLATFORM=os.uname()[0])):
            PIPE = subprocess.PIPE
            proc = subprocess.Popen(["make"], stdout = PIPE, stderr = PIPE)
            if proc.wait():
                errormsg = proc.stderr.read()
                msg = "Unable to build imagedb executable - Error was:\n {errormsg}".format(**locals())
                logger.error(msg)
                raise IOError(msg)
    return imagedb_prog

def imagedb(image, title = None, frame = None, fields = None, flip = 1, **kwargs):
    '''Wrapper around <http://www.cs.princeton.edu/~cdecoro/imagedb/>
    
    The following imagedb options are arguments to this function, with
    sensible defaults:
        title(t), frame(fr), fields(f), flip
    The following options are harvested automatically from the image:
        h, w, b
    Other options can be passed as **kwargs to this function; see the imagedb
    website at the URL above for the full list of options.
    '''
    imagedb_prog = _ensure_imagedb()
    image = imagedb_compatible(image)
    h, w, d = np.atleast_3d(image).shape
    b = byte_types[image.dtype]
    f = fields if fields is not None else field_types[d]
    t = title if title is not None else "Untitled"
    fr = frame if frame is not None else ""
    format_string = ("h={h} w={w} b={b} f={f} t='{t}' fr={fr} "
                     "pg={imagedb_prog}").format(**locals()) 
    format_string += ' '.join(["{0}={1}".format(k,v) for k,v in kwargs.iteritems()])
    if flip: format_string += " flip"
    # TODO rather than copy the array, extract the stride info and pass it
    # into row skip and col skip parameters.
    x = np.ascontiguousarray(image)
    imagedb_so.imagedb_wrap(x.ctypes.data, format_string)
    return x

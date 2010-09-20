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
    np.int8 : "8",
    np.int16 : "16",
    np.int32 : "32",
    np.uint8 : "8",
    np.uint16 : "16",
    np.uint32 : "32",
    np.float32 : "32f",
    np.float64 : "64f"
}

field_types = {
    1 : "l",
    2 : "la",
    3 : "rgb",
    4 : "rgba",
}

def _ensure_imagedb():
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
    
    Options to imagedb are taken from the numpy.ndarray image.
    title - title to use
    '''
    # TODO rather than copy the array, extract the stride info and pass it
    # into row skip and col skip parameters.
    imagedb_prog = _ensure_imagedb()
    h, w, d = np.atleast_3d(image).shape
    b = byte_types[image.dtype.type]
    f = fields if fields is not None else field_types[d]
    t = title if title is not None else "Untitled"
    fr = frame if frame is not None else ""
    format_string = ("h={h} w={w} b={b} f={f} t='{t}' fr={fr} flip={flip} "
                     "pg={imagedb_prog}").format(**locals()) 
    format_string += ["{0}={1}".format(k,v) for k,v in kwargs.iteritems()]
    imagedb_so.imagedb_wrap(image.copy().ctypes.data, format_string)

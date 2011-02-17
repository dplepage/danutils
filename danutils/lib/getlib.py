import os.path as pth
import subprocess
import ctypes
import os
import logging

def getlib(here, libname, srcs=None, extra_opts = [], cmd=None):
    '''Utility for loading a library in ctypes, compiling it if needed.'''
    libfile = pth.join(here, libname)
    logger = logging.getLogger('getlib')
    # Compile it if it's missing
    if not os.path.exists(libfile):
        logger.info("Attempting to build shared library: {libname}".format(**locals()))
        if cmd is None:
            if srcs is not None:
                srcs = ['{here}/{src}'.format(**locals()) for src in srcs]
            else:
                srcs = [libfile.replace('.so','.cpp')]
            cmd = ['g++','-shared'] + srcs + ['-o', libfile, '-fPIC'] + extra_opts
        logger.info("Building {libname} with cmd '{0}'".format(' '.join(cmd), **locals()))
        proc = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        if proc.wait():
            errormsg = proc.stderr.read()
            msg = "Unable to build library {libfile} - Error was:\n {errormsg}".format(**locals())
            logger.error(msg)
            raise IOError(msg)
    logger.debug("Loading library {libname}".format(**locals()))
    return ctypes.cdll.LoadLibrary(libfile)


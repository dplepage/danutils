from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

name = "ValDer"

ext_modules = [ 
    Extension(name,[name+".pyx"],language="c++"),
    ]

setup(name="ValDer_Sparse",
    version="1.0",
    ext_modules=ext_modules,
    cmdclass = {'build_ext':build_ext})

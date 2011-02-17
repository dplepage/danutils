import os
import ctypes
import numpy as np
from numpy.ctypeslib import ndpointer
from danutils.ostools import tmpdir, chdir

def fastcomp(code):
    with tmpdir() as d:
        with chdir(d):
            open('foo.cpp','w').write(code)
            os.system('g++ foo.cpp -shared -o foo.so -fPIC')
            lib = ctypes.cdll.LoadLibrary('foo.so')
            return lib

sq = fastcomp("""
extern "C" {
void square(double* x, double* y, int n) {
    for(int i = 0; i < n; i++)
        y[i] = x[i]*x[i];
}
}
""")


x = np.random.random((4,4))
y = np.empty_like(x)

print sq.square

# 
# sq.square.argtypes = [ndpointer(), ndpointer(), ctypes.c_int]
# sq.square(x,y,16)
# print x
# print y
# print x*x
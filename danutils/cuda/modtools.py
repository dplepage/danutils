"""
Tools for manipulating pycuda code modules
"""
import pycuda.driver as cuda

def setGlobal(mod, name, val):
    target, ksize = mod.get_global(name)
    assert ksize == len(val)
    cuda.memcpy_htod(target,val)

import pycuda.autoinit
import scipy
from scipy.ndimage import convolve1d
from autoprep import AutoPrepModule

def iDivUp(x, y):
    return scipy.ceil(x*1.0/y)

def makeKernel(sdev, rad=None):
    """
    Create a gaussian derivative kernel, with standard deviation sdev.

    If rad is specified the kernel will be truncated to size 2*rad+1. If rad is 
    None, it defaults to four times the standard deviation.
    """
    if rad is None:
        rad = scipy.ceil(sdev*4)
    x = scipy.mgrid[-rad : rad + 1]
    kernelg = scipy.exp(-(x*x)/(2.0*(sdev)**2))
    kernelg /= kernelg.sum()
    kerneld = (kernelg * -x/(sdev)**2)
    return kerneld

def main(w, h, sdev):
    data = scipy.random.random((w,h)).astype('float32')
    data_gpu = driver.to_device(data)
    kernel = makeKernel(sdev)
    
    data_w, data_h = w,h
    radius = kernel.len()//2
    
    """Ground Truth"""
    ground_x = ndi.convolve1d(data, kernel, axis=0, mode = "constant")
    ground_y = ndi.convolve1d(data, kernel, axis=1, mode = "constant")
    
    """Variable-size Data"""
    src = open("autoprep_example/convolutionSeparable.cu").read()
    data = dict(data_w = w, data_h = h, radius = radius, row_tile_w = 128,
                column_tile_w = 16, column_tile_h = 48, sm)
    conv = AutoPrepModule(src, vars = dict(data_w=w, data_h=h, radius=radius, 
                                           iDivUp = iDivUp))
    varout_x = driver.mem_alloc(data.nbytes())
    varout_y = driver.mem_alloc(data.nbytes())
    conv.d_Kernel = kernel
    rowGrid = (iDivUp(data_w, row_tile_w), data_h)
    conv.convolutionRowGPU(varout_x, data_gpu, data_w, data_h,
                           grid = rowGrid)
    colGrid = (iDivUp(data_w, column_tile_w), iDivUp(data_h, column_tile_h))
    dconv.convolutionColumnGPU(varout_x, data_gpu, data_w, data_h,
                           grid=(iDivUp(data_w, row_tile_w), data_h))

    """Known-size Data"""
    src = open("autoprep_example/knownSizeConvolution.cu").read()

if __name__ == '__main__':
    import sys
    w, h = map(int, sys.argv[1:3])
    sdev = float(sys.argv[3])
    main(w, h, sdev)



src_known = open("autoprep_example/knownSizeConvolution.cu").read()

func = mod.get_function("W_kernel")

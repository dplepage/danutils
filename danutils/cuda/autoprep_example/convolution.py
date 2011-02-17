from scipy.ndimage import convolve1d


sdev = 10
rad = int(sdev*4)
x = scipy.mgrid[-rad : rad + 1]
kernelg = scipy.exp(-(x*x)/(2.0*(sdev)**2))
kernelg /= kernelg.sum()  # Normalized Gaussian
kerneld = (kernelg * -x/(sdev)**2)
return kernelg, kerneld



ndi.convolve1d(data, kernel, axis=0, mode = "constant")
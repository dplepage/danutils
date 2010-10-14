from numpytools import *
from debugprint import *
from mappedarrayset import *


from scipy.signal import convolve2d

def dan_conv(a,b):
    '''Does what convolve2d purports to do
    
    scipy.signal.convolve2d(a, b, mode='same', old_behavior=False) is supposed
    to always return a result the size of a. Without old_behavior=False, it
    uses an older, deprecated behavior where it switches a and b if b is 
    larger than a, so that the returned array will always be the size of 
    whichever is larger.
    
    Due to a bug in the current scipy implementation, the old_behavior flag
    has no effect, so that the above call, when b is larger than a, will 
    return an array the same shape as b.
    
    This function is a quick and dirty hack to solve this problem.
    dan_conv(a,b) is equivalent to the expected behavior of
    scipy.signal.convolve2d(a, b, mode='same', old_behavior=False)
    
    This was written as of scipy 0.8.0; it is expected that as of 0.9 this bug
    will be rectified, and this function will become obsolete.
    '''
    result = convolve2d(a,b,mode='full',old_behavior=False)
    extra_h, extra_w = b.shape
    h, w = result.shape
    pad_lh = extra_h/2
    pad_uh = h - (extra_h - pad_lh - 1)
    pad_lw = extra_w/2
    pad_uw = w - (extra_w - pad_lw - 1)
    result = result[pad_lh:pad_uh, pad_lw:pad_uw].copy()
    return result

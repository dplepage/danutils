import numpy
from danutils.misc import terminal_size

def normalize(v):
    """Return v/|v|
    
    >>> import numpy
    >>> v = numpy.array([3,4])
    >>> print normalize(v)
    [ 0.6  0.8]
    """
    return v/numpy.sqrt((v*v).sum())

def center(v):
    """Center and whiten v
    
    >>> import numpy
    >>> v = numpy.array([4,8,1])
    >>> print center(v)
    """
    return (v-v.min())/(v.max()-v.min())

def axisnorm(v,axis=-1):
    shape = list(v.shape)
    shape[axis] = 1
    factor = (v*v).sum(axis=axis).reshape(shape)
    factor = factor**.5
    return v/factor

def setprintsize():
    cols, rows = terminal_size()
    numpy.set_printoptions(linewidth = cols)

def setup_sci():
    cols, rows = terminal_size()
    numpy.set_printoptions(precision=3, linewidth = cols, threshold=32*32)
    

if __name__ == '__main__':
    import doctest
    doctest.testmod()

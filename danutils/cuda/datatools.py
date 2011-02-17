'''
A collection of helper functions to move data to and from the GPU.
'''

def ndarrayToTexref3d(data,texref):
    """Copy a 3D numpy array to texture memory and bind it to texref """
    # I'm not sure why order should be F, not C, but this seems to work!
    if data.dtype == bool:
        wasbool=True
        data = scipy.asarray(data,dtype=scipy.uint8,order='F')
    else:
        wasbool=False
        data = scipy.asarray(data,dtype=scipy.float32,order='F')
    h, w, d = data.shape
    h,w = w,h
    descr = cuda.ArrayDescriptor3D()
    descr.height = h
    descr.width = w
    descr.depth = d
    descr.format = cuda.dtype_to_array_format(data.dtype)
    descr.num_channels = 1
    descr.flags = 0
    ary = cuda.Array(descr)

    copy = cuda.Memcpy3D()
    copy.set_src_host(data)
    copy.set_dst_array(ary)
    copy.width_in_bytes = copy.src_pitch = data.strides[1]
    copy.height = copy.src_height = h
    copy.depth = d
    copy()

    texref.set_flags(cuda.TRSF_READ_AS_INTEGER)
    texref.set_filter_mode(cuda.filter_mode.POINT)
    texref.set_format(descr.format,1)
    texref.set_array(ary)
    
def ndarrayToTexref1d(data,texref):
    texref.set_flags(cuda.TRSF_READ_AS_INTEGER)
    texref.set_filter_mode(cuda.filter_mode.POINT)
    texref.set_format(cuda.array_format.FLOAT,4)
    data = scipy.hstack([data, scipy.zeros_like(data[:,0:1])])
    data = scipy.asarray(data,dtype=scipy.float32,order='C').reshape(1,-1,4)
    texref.set_array(cuda.make_multichannel_2d_array(data,order='C'))

def ndarrayToTexref2d(data,texref,nchannels = 4):
    if data.ndim == 2:
        data = data.reshape(data.shape+(1,))
    if data.ndim != 3 or data.shape[-1] > nchannels:
        raise ValueError(
            "Data of shape %s cannot be converted to 2D %i-channel array."%(
                data.shape, nchannels))
    extradims = nchannels - data.shape[-1]
    data = scipy.dstack([data]+ [scipy.zeros_like(data[:,:,0])] * extradims)
    if data.dtype == bool:
        data = scipy.asarray(data,dtype=scipy.uint8,order='C')
    else:
        data = scipy.asarray(data,dtype=scipy.float32,order='C')
    texref.set_flags(cuda.TRSF_READ_AS_INTEGER)
    texref.set_filter_mode(cuda.filter_mode.POINT)
    texref.set_format(cuda.dtype_to_array_format(data.dtype),nchannels)
    if nchannels > 1:
        texref.set_array(cuda.make_multichannel_2d_array(data,order='C'))
    else:
        data=data.reshape(data.shape[:2])
        texref.set_array(cuda.matrix_to_array(data,order='C'))

def fetch(darray, shape=None, target=None):
    '''Copy darray from the GPU into target, or into an array of size shape.
    
    Exactly one of target and shape should be specified.
    '''
    if target is None:
        if shape is None:
            raise TypeError('fetch() requires either a target or a shape')
        target = scipy.empty(shape, dtype='float32')
    else:
        if shape is not None:
            raise TypeError('fetch() cannot take both a target and a shape')
    cuda.memcpy_dtoh(target,darray)
    return target

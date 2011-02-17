import pycuda.compiler as comp
import pycuda.driver as cuda


class CudaFunc(object):
    """
    Wrapper around a CUDA function object.
    
    Optionally serializes calls into a stream.
    The class member timedata is a dictionary of name:(inv,time) pairs where
    inv is the total number of times the function called name has been invoked, 
    and time is the total time of all invocations; timedata will be empty
    until a CudaFunc initialized with time=True has been run.
    """
    timedata = {}
    _default_stream = None
    _default_time = False
    
    @classmethod
    def setDefaultStream(cls, stream=None):
        """Set the default stream used for new CudaFunc objects."""
        cls._default_stream = stream
    
    @classmethod
    def setDefaultTiming(cls, time=False):
        """If true, new CudaFunc objects will have time=True unles specified otherwise."""
        cls._default_time = False
    
    def __init__(self, module, name, types, block = (512, 1, 1), texrefs = [], 
                 sync = False, stream = None, time = None):
        """Create a new CudaFunc and prepare it for calling.

        Positional arguments:
        module -- the pycuda.compiler.Module where the function is defined
        name -- the name of the function
        types -- an iterable of type characters understood by the struct module
        Keyword arguments:
        block -- a 3-element thread block dimension (default (512, 1, 1))
        texrefs -- a list of texture references (default [])
        sync -- if True, ignore the stream argument and run synchronously
        stream -- a stream to serialize calls, or None to use the default
                  determined by CudaFunc.setDefaultStream (default None)
        time -- if True, add timing results for this function to the
                CudaFunc.timedata dictionary
        """

        super(CudaFunc, self).__init__()
        self.module = module
        self.name = name
        self.types = types
        self.block = block
        self._func = self.module.get_function(self.name)
        if texrefs:
            self._func.prepare(types, block = block, texrefs = texrefs)
        else:
            self._func.prepare(types, block = block)
        if sync:
            self.stream = None
        else:
            if stream is None:
                self.stream = self._default_stream
            else:
                self.stream = stream
        self.time = time
        if time is None:
            self.time = self._default_time
        if sync or self.stream is None:
            if self.time:
                self.f = self.time_call_sync
            else:
                self.f = self.call_sync
        else:
            if self.time:
                self.f = self.time_call_async
            else:
                self.f = self.call_async
    
    @classmethod
    def log_call(cls, name, ms):
        inv,time = cls.timedata.get(name,(0,0))
        cls.timedata[name] = (inv+1, time+ms)
    
    def call_sync(self, griddim, *args):
        self._func.prepared_call(griddim, *args)
    
    def time_call_sync(self, griddim, *args):
        a = self._func.prepared_timed_call(griddim, *args)
        self.log_call(self.name, a())
    
    def call_async(self, griddim, *args):
        self._func.prepared_async_call(griddim, self.stream, *args)

    def time_call_async(self, griddim, *args):
        c1, c2 = cuda.Event(), cuda.Event()
        c1.record(self.stream)
        self._func.prepared_async_call(griddim, self.stream, *args)
        c2.record(self.stream)
        c2.synchronize()
        ms = c2.time_since(c1)
        self.log_call(self.name, ms)
    
    def __call__(self,*args):
        self.f(*args)

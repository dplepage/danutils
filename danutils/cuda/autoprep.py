import pycuda.compiler as compiler
import pycuda.driver as driver



class AutoPrepFunc(object):
    def __init__(self, module, name, sig=None, block = None, texrefs = None,
                 stream = None, timelog = None, grid = None):
        """

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
        super(AutoPrepFunc, self).__init__()
        self.module = module
        self.name = name
        self.types = types
        self.block = block
        self.grid = grid
        self._func = self.module.get_function(self.name)
        if texrefs:
            self._func.prepare(types, block = block, texrefs = texrefs)
        else:
            self._func.prepare(types, block = block)
        self.stream = stream
        self.timelog = timelog
        if self.stream is None:
            if self.timelog is not None:
                self.f = self.time_call_sync
            else:
                self.f = self.call_sync
        else:
            if self.timelog is not None:
                self.f = self.time_call_async
            else:
                self.f = self.call_async
    
    def call_sync(self, griddim, *args):
        self._func.prepared_call(griddim, *args)
    
    def time_call_sync(self, griddim, *args):
        a = self._func.prepared_timed_call(griddim, *args)
        self.timelog.log(self.name, a())
    
    def call_async(self, griddim, *args):
        self._func.prepared_async_call(griddim, self.stream, *args)

    def time_call_async(self, griddim, *args):
        c1, c2 = driver.Event(), driver.Event()
        c1.record(self.stream)
        self._func.prepared_async_call(griddim, self.stream, *args)
        c2.record(self.stream)
        c2.synchronize()
        ms = c2.time_since(c1)
        self.timelog.log(self.name, ms)
    
    def __call__(self,*args):
        if self.grid:
            self.f(grid,*args)
        else: # Assume 1st arg will be the grid
            self.f(*args)

class _Registry(object):
    """Registry object that builds collection of functions and textures"""
    def __init__(self):
        super(_registry, self).__init__()
        self.funcs = {}
        self.texs = {}
    
    def function(self, name, sig, block, grid=None, texs = None):
        self.funcinfo[name] = (sig, block, grid, texs)
        return ''
    
    def texture(self, name, type, dims):
        nargs = len(dims)
        macro_args = ', '.join(['a%i'%i for i in range(nargs)])
        macro = "#define %sI(%s) tex3D(%s,%s)"%(name, macro_args, name, macro_args)
        decl = "texture <%s, %s> %s;"%(type, nargs, dims)
        self.texs[name] = (type, dims)
        return decl+'\n'+macro

class AutoPrepModule(object):
    """Module with auto-preparing functions"""
    def __init__(self, source, vars={}, sync=False, stream=None, time=False):
        super(AutoPrepModule, self).__init__()
        self.source = source
        self.vars = vars
        self.sync = sync
        if self.sync:
            self.stream = None
        else:
            self.stream = stream if stream else driver.Stream()
        self.time = time
        self.funcs = {}
        self.texrefs = {}
        reg = _Registry()
        self.rendered_source = self.source.render(vars, reg = reg)
        self.mod = compiler.SourceModule(self.rendered_source)
        self._prep_texrefs(reg)
        self._prep_functions(reg)
    
    def _prep_texrefs(self,reg):
        for ref in reg.texrefs.keys():
            self.texrefs[ref] = self.mod.get_texref(ref)
    
    def _prep_functions(self):
        for name, (sig, block, grid, texnames) in self.funcinfo.iteritems():
            texrefs = [self.texrefs[ref] for ref in texnames]
            self.funcs[name] = AutoPrepFunc(self.mod, name, sig, block, texrefs,
                                            self.sync, self.stream, self.time)
    
    def get_function(self,name):
        return self.funcs[name]
    
    def get_texref(self,name):
        return self.texrefs[name]
    
    @property
    def module(self):
        return self.mod
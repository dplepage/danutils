from contextlib import contextmanager
from itertools import groupby, izip, count
import warnings, os

def deprecated(func):
    """This is a decorator which can be used to mark functions
    as deprecated. It will result in a warning being emitted
    when the function is used."""
    def newFunc(*args, **kwargs):
        warnings.warn("Call to deprecated function %s." % func.__name__,
                      category=DeprecationWarning,stacklevel=2)
        return func(*args, **kwargs)
    newFunc.__name__ = func.__name__
    newFunc.__doc__ = func.__doc__
    newFunc.__dict__.update(func.__dict__)
    return newFunc
    
def cachedproperty(f):
    '''Property that caches the value of an input function'''
    name = f.__name__
    def getter(self):
        try:
            return self.__dict__[name]
        except KeyError:
            res = self.__dict__[name] = f(self)
            return res
    return property(getter)

@contextmanager
def replacing(ob,**kwargs):
    """
    Context Manager that replaces parameters of an object
    and restores them upon completion.
    example:
    
    >>> class f: x = 15
    ... 
    >>> with replacing(f,x=12):
    ...     print f.x
    12
    >>> print f.x
    15
    """
    old = dict([(k,ob.__dict__[k]) for k in kwargs.keys()])
    ob.__dict__.update(kwargs)
    yield ob
    ob.__dict__.update(old)

def batchby(iterable, size):
    c = count()
    for k, g in groupby(iterable, lambda _:c.next()//size):
        yield g

def ioctl_GWINSZ(fd):                  #### TABULATION FUNCTIONS
     try:                                ### Discover terminal width
         import fcntl, termios, struct
         cr = struct.unpack('hh',
                            fcntl.ioctl(fd, termios.TIOCGWINSZ, '1234'))
     except:
         return None
     return cr

def terminal_size():
     ### decide on *some* terminal size
     # try open fds
     cr = ioctl_GWINSZ(0) or ioctl_GWINSZ(1) or ioctl_GWINSZ(2)
     if not cr:
         # ...then ctty
         try:
             fd = os.open(os.ctermid(), os.O_RDONLY)
             cr = ioctl_GWINSZ(fd)
             os.close(fd)
         except:
             pass
     if not cr:
         # env vars or finally defaults
         try:
             cr = (env['LINES'], env['COLUMNS'])
         except:
             cr = (25, 80)
     # reverse rows, cols
     return int(cr[1]), int(cr[0])

class memoize:
  def __init__(self, function):
    self.function = function
    self.memoized = {}

  def __call__(self, *args, **kwargs):
    try:
      return self.memoized[(args,`kwargs`)]
    except KeyError:
      self.memoized[(args,`kwargs`)] = self.function(*args,**kwargs)
      return self.memoized[(args,`kwargs`)]

class dummy:
    def __init__(self, **kwargs):
        for key,val in kwargs.iteritems():
            setattr(self, key, val)

class WipingMainException(Exception): pass

class Clear(object):
  def __init__(self, depth=1):
    import sys
    name = sys._getframe(depth).f_globals["__name__"]
    if name == '__main__':
      raise WipingMainException(
        "Importing "+__name__+" from your main module will erase"
        " __main__ from sys.modules. This is almost certainly not"
        " something you want to do.")
    sys.modules[name] = self
    self.name = name
  
  def wipe(self):
    import sys
    if self.name in sys.modules.keys():
      del sys.modules[self.name]
  
  def __getattr__(self, *args, **kwargs):
    self.wipe()
    return self

Clear(2)
Clear(1)

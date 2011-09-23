class Clear(object):
  def __init__(self, depth=1):
    import sys
    name = sys._getframe(depth).f_globals["__name__"]
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

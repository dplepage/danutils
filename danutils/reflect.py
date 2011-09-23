import inspect
import code
import sys

def interact_here(depth=1):
  frame = sys._getframe(depth)
  d = frame.f_globals.copy()
  d.update(frame.f_locals)
  code.interact(local=d)


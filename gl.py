"""
Useful functions for coding with OpenGL
"""

from contextlib import contextmanager
from OpenGL.GL import glBegin, glEnd

@contextmanager
def ending(arg):
    """Context manager ensuring glEnd"""
    glBegin(arg)
    yield
    glEnd()


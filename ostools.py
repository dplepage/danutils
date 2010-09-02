#!/usr/bin/python
# -*- encoding: utf-8 -*-
from __future__ import with_statement # In case of 2.5
import os
import os.path
import glob

def ensurefile(pth):
    """Make every directory needed to contain the specified file path.
    
    >>> import tempfile
    >>> td = os.path.realpath(tempfile.mkdtemp())
    >>> newfile = os.path.join(td,"foo/bar/baz.py")
    >>> assert not os.path.exists(newfile)
    >>> pth, fname = os.path.split(newfile)
    >>> assert not os.path.exists(pth)
    >>> ensurefile(newfile)
    >>> assert not os.path.exists(newfile)
    >>> assert os.path.exists(pth)
    """
    pth,file = os.path.split(pth)
    mkdir(pth)

def mkdir(pth):
    """
    mkdir ensures that the directory given by dirname exists.
    
    If dirname has subdirectories, every parent directory is created as well.
    
    >>> import tempfile
    >>> td = os.path.realpath(tempfile.mkdtemp())
    >>> fullpath = os.path.join(td,"foo","bar","baz")
    >>> assert not os.path.exists(fullpath)
    >>> mkdir(fullpath)
    >>> assert os.path.exists(fullpath)
    >>> mkdir(fullpath)
    >>> assert os.path.exists(fullpath)
    """
    parent,child = os.path.split(pth)
    if parent and not os.path.exists(parent):
        mkdir(parent)
    # We use try-except instead of checking if os.path.exists(pth)
    # in case another process creates the file between when we check
    # and when we try to create.
    try:
        os.mkdir(pth)
    except OSError, e:
        if e.errno != 17: # 17 -> File already exists
            raise
                

class chdir(object):
    """
    A context manager that temporarily changes the working directory.

    >>> import tempfile
    >>> td = os.path.realpath(tempfile.mkdtemp())
    >>> currentdirectory = os.getcwd()
    >>> with chdir(td) as cd:
    ...     assert cd.current == td
    ...     assert os.getcwd() == td
    ...     assert cd.previous == currentdirectory
    ...     assert os.path.normpath(os.path.join(cd.current, cd.relative)) == cd.previous
    ...
    >>> assert os.getcwd() == currentdirectory
    >>> with chdir(td) as cd:
    ...     os.mkdir('foo')
    ...     with chdir('foo') as cd2:
    ...         assert cd2.previous == cd.current
    ...         assert cd2.relative == '..'
    ...         assert os.getcwd() == os.path.join(td, 'foo')
    ...     assert os.getcwd() == td
    ...     assert cd.current == td
    ...     os.rmdir('foo')
    ...
    >>> os.rmdir(td)
    >>> with chdir('.') as cd:
    ...     assert cd.current == currentdirectory
    ...     assert cd.current == cd.previous
    ...     assert cd.relative == '.'
    """

    def __init__(self, directory):  #:notest:
        self._dir = directory
        self._cwd = os.getcwd()
        self._pwd = self._cwd

    def _current(self):
        return self._cwd
    current = property(_current)

    def _previous(self):
        return self._pwd
    previous = property(_previous)
    
    def _relative(self):
        c = self._cwd.split(os.path.sep)
        p = self._pwd.split(os.path.sep)
        l = min(len(c), len(p))
        i = 0
        while i < l and c[i] == p[i]:
            i += 1
        return os.path.normpath(os.path.join(*(['.'] + (['..'] * (len(c) - i)) + p[i:])))
    relative = property(_relative)
    
    def __enter__(self):
        self._pwd = self._cwd
        os.chdir(self._dir)
        self._cwd = os.getcwd()
        return self

    def __exit__(self, *args):
        os.chdir(self._pwd)
        self._cwd = self._pwd


def nextdir(root, pattern='run{0}'):
    '''Generate a new, unique directory name in root based on pattern'''
    candidates = glob.glob(os.path.join(root,"*"))
    # This is not the most efficient way to do this.
    # But it is very flexible!
    for i in count(0):
        if pattern.format(i) not in candidates:
            return pattern.format(i)
    


if __name__ == '__main__':
    import doctest
    doctest.testmod()
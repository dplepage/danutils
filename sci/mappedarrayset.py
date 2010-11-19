import os

import numpy as np
import yaml

class MappedArraySet(object):
    """Wraps a collection of named arrays stored as memory-mapped files."""
    def __init__(self, dirname, readonly=False):
        '''Create or load a MappedArraySet in a given directory.
        
        This creates the directory (if it does not already exist), and either
        reads an existing manifest.yml file if one exists, or creates a new, 
        empty manifest in the directory. If the manifest already exists, any 
        entries for which the files no longer exist will be removed.
        
        If readonly is set, no modifications can be made to the manifest or the 
        arrays.
        '''
        super(MappedArraySet, self).__init__()
        try:
            os.makedirs(dirname)
        except OSError:
            pass
        self.dirname = dirname
        self.readonly = readonly
        if os.path.exists(self._getf('manifest.yml')):
            self.manifest = yaml.load(open(self._getf('manifest.yml')).read())
            if not readonly:
                for (name, (fname, dtype, shape)) in self.manifest.items():
                    if not os.path.exists(self._getf(fname)):
                        del self.manifest[name]
        else:
            self.manifest = {}
        self._cache = {}
    
    def store(self):
        '''Update the manifest file'''
        if self.readonly:
            raise IOError("Attempt to write to readonly MappedArraySet")
        open(self._getf('manifest.yml'), 'w').write(yaml.dump(self.manifest))
    
    def flush(self):
        '''Force all open arrays to write back'''
        if self.readonly:
            raise IOError("Attempt to write to readonly MappedArraySet")
        self.store()
        for arr in self._cache.values():
            arr.flush()
    
    def __delitem__(self, name):
        '''Destroy an array (deleting the corresponding file)'''
        if self.readonly:
            raise IOError("Attempt to write to readonly MappedArraySet")
        if not self.hasArray(name):
            raise KeyError(name)
        (fname, dtype, shape) = self.manifest[name]
        if name in self._cache.keys():
            del self._cache[name]
        del self.manifest[name]
        os.remove(self._getf(fname))
        self.store()

    def _getf(self,fname):
        return os.path.join(self.dirname, fname)
    
    def __getitem__(self, name):
        '''Return a memmap of the named array'''
        if not self.hasArray(name):
            raise KeyError(name)
        if name in self._cache.keys():
            return self._cache[name]
        (fname, dtype, shape) = self.manifest[name]
        mode = 'r+'
        if self.readonly:
            mode = 'r'
        mm = np.memmap(self._getf(fname), mode=mode, dtype=dtype, shape=shape)
        self._cache[name] = mm
        return mm
    
    @staticmethod
    def toFileName(arrname):
        '''Escape an array name to make it a sensible filename'''
        swaps = [
        ('_','__'),
        (' ','_s'),
        ('.','_d')]
        for a,b in swaps:
            arrname = arrname.replace(a,b)
        return arrname + '.dat'
    
    def __setitem__(self, name, arr):
        if self.readonly:
            raise IOError("Attempt to write to readonly MappedArraySet")
        if self.hasArray(name):
            del self[name]
        self.addArray(name, arr)
    
    def hasArray(self, name):
        return name in self.manifest.keys()
    
    def addArray(self, name, array):
        if self.readonly:
            raise IOError("Attempt to write to readonly MappedArraySet")
        fname = self.toFileName(name)
        self.manifest[name] = (fname, array.dtype.name, array.shape)
        array.tofile(self._getf(fname))
        self.store()
        return self[name]
    
    def newArray(self, name, shape, dtype='float32', overwrite=False):
        if self.readonly:
            raise IOError("Attempt to write to readonly MappedArraySet")
        if self.hasArray(name):
            if overwrite:
                del self[name]
            else:
                raise KeyError("Duplicate key: %s"%name)
        fname = self.toFileName(name)
        mm = np.memmap(self._getf(fname), mode='w+', dtype=dtype, shape=shape)
        self.manifest[name] = (fname, dtype, shape)
        self._cache[name] = mm
        self.store()
        return mm

    def getArray(self, name, shape, dtype='float32'):
        if self.hasArray(name):
            return self[name]
        return self.newArray(name, shape, dtype)
        fname = self.toFileName(name)
        mm = np.memmap(self._getf(fname), mode='w+', dtype=dtype, shape=shape)
        self.manifest[name] = (fname, dtype, shape)
        self._cache[name] = mm
        self.store()
        return mm

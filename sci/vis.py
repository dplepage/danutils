import numpy as np
import pylab as p

class ImShower(object):
    def __init__(self):
        self.items = []
    
    def imshow(self,im, title=''):
        self.items.append([im,title])
    
    def render(self):
        nims = len(self.items)
        nrows = np.floor(nims**.5)
        ncols = np.ceil(nims*1.0/nrows)
        for i,(im,title) in enumerate(self.items):
            p.subplot(nrows, ncols, i+1)
            p.imshow(im, interpolation='nearest')
            p.title(title)

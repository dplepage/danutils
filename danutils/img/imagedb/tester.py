import os

import numpy as np

here = os.path.split(__file__)[0]
metal = os.path.join(here, 'metalrect.tga')

if __name__ == '__main__':
    from danutils.img import imread
    from danutils.img.imagedb import imagedb
    m = imread(metal)
    imagedb(m)
    for i in [3,4,5]:
        dtype = 'int'+`2**i`
        imagedb((m*2**(2**i)).astype(dtype),'{0} image'.format(dtype))
    for dtype in ['float32','float64']:
        imagedb((m).astype(dtype),'{0} image'.format(dtype))
    
    x = np.arange(512*512).reshape((512,512))/(512.0*512)
    imagedb(x)
    imagedb(np.dstack([x,x*0+.5,x.T]))
from __future__ import division 
from itertools import chain, repeat

import Image
import numpy as np

from danutils.misc import batchby

def imread(fname, mode='RGBA'):
    return np.asarray(Image.open(fname).convert(mode))/255.0

def floatToImg(M):
    """
    Take a floating point image array with values between 0 and 1 and convert it
    to a uint8 PIL Image
    """
    if len(M.shape) == 2 or M.shape[2] == 1:
        mode = 'L'
    elif M.shape[2] == 3:
        mode = 'RGB'
    else:
        mode = 'RGBA'
    return Image.fromarray(np.uint8(M.clip(0,1)*255),mode=mode)

f2i = floatToImg

def tileIms(cols,*ims):
    print "cols:",cols
    print "ims:",len(ims)
    ims = list(chain(ims,repeat(np.zeros_like(ims[0]),cols-len(ims)%cols)))
    print "new ims:",len(ims)
    l = [np.hstack([im for im in batch]) for batch in batchby(ims,cols)]
    for i in l:
        print i.shape
    return floatToImg(np.vstack(l))
    

def grayToHSVImg(M):
    """
    Take a floating point 1-channel image and return a PIL image where the one
    channel is hue, and saturation and value are set to 1.
    """
    o = np.ones_like(M)
    return floatToImg(hsv_to_rgb(np.dstack([M,o,o])))
    """                                      /  
                                         (__) 
                                         (oo) 
                                   /------\/ 
                                  / |    ||   
                                 *  /\---/\ 
                                    ~~   ~~"""

def vis_gray(M):
    """
    Visualize a grayscale image by mapping the range [0,1) to a blue-green-red
    stretch of HSV color space
    """
    return hsv_to_rgb(np.dstack([(1-M.clip(0,1))*.66,np.ones_like(M),M]))

def rgb_to_hsv(img):
    h,w,d = img.shape
    r, g, b = np.dsplit(img,3)
    maxc = img.max(axis=2).reshape(h,w,1)
    minc = img.min(axis=2).reshape(h,w,1)
    s = (maxc-minc) / maxc
    v = maxc
    imgc = (maxc-img)/(maxc-minc)
    rc, gc, bc = np.dsplit(imgc,3)
    h = np.where(maxc==r, bc-gc,
        np.where(maxc==g, 2.0+rc-bc,
                             4.0+gc-rc))
    h = (h/6.0) % 1.0
    hsv = np.dstack([h,s,v])
    v0 = np.dstack([np.zeros_like(h),np.zeros_like(h),v])
    mask = minc == maxc
    mask = np.dstack([mask,mask,mask])
    return np.where(mask, v0, hsv)

def hsv_to_rgb(hsv):
    h, s, v = np.dsplit(hsv,3)
    i = np.floor(h*6.0)
    f = (h*6.0) - i
    p = v*(1.0 - s)
    q = v*(1.0 - s*f)
    t = v*(1.0 - s*(1.0-f))
    i = np.dstack([i%6,i%6,i%6])
    s = np.dstack([s,s,s])
    where = np.where
    return (
        where(s == 0.0, np.dstack([v, v, v]), 
        where(i == 0,   np.dstack([v, t, p]), 
        where(i == 1,   np.dstack([q, v, p]), 
        where(i == 2,   np.dstack([p, v, t]), 
        where(i == 3,   np.dstack([p, q, v]), 
        where(i == 4,   np.dstack([t, p, v]), 
                        np.dstack([v, p, q]),
        )))))))

def fresize(im,newshape):
    l = []
    h, w = newshape
    for plane in range(im.shape[2]):
        fim = Image.fromarray(im[:,:,plane],mode='F')
        l.append(np.asarray(fim.resize((w,h), Image.BICUBIC)))
    return np.dstack(l)

def togray(im):
    if im.ndim == 2 or im.shape[2] == 1:
        return np.atleast_3d(im)[:,:,0]
    return im.mean(axis=-1)

def asciify(im, chars="#Wmg=-. "):
    chars = list(chars)
    nc = len(chars)
    im = togray(im)
    chars = np.array(chars)
    indices = (im*(nc-.00001)).astype(int)
    asciid = chars[indices]
    return '\n'.join(''.join(row) for row in asciid)

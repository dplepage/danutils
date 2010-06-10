import wx
from danutils.img import floatToImg
from danutils.wx import pilToImage

class StaticImage(wx.StaticBitmap):
    def __init__(self, parent, im = None, scale=1, *args, **kwargs):
        wx.StaticBitmap.__init__(self, parent, *args, **kwargs)
        self.scale = scale
        self.setImage(im,scale)
    
    def setImage(self, im, scale=None):
        self.dat = im
        if scale is None:
            scale = self.scale
        self.scale = scale
        if self.dat is None:
            self.actualsize = (0,0)
            return
        pilim = floatToImg(im*scale)
        self.actualsize = pilim.size
        pilim = pilim.resize(self.GetSize())
        self.SetBitmap(wx.BitmapFromImage(pilToImage(pilim)))
    
    def setScale(self, scale=1):
        self.scale = scale
        if self.dat is None:
            return
        pilim = floatToImg(self.dat*scale)
        pilim = pilim.resize(self.GetSize())
        self.SetBitmap(wx.BitmapFromImage(pilToImage(pilim)))

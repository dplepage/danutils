from contextlib import contextmanager
import wx

@contextmanager
def destroying(dlg):
    yield dlg
    dlg.Destroy()

def pilToImage(pil):
    image = wx.EmptyImage(pil.size[0], pil.size[1])
    image.SetData(pil.convert('RGB').tostring())
    return image

def testControlApp(title,control,*args,**kwargs):
    class MainWindow(wx.Frame): 
        def __init__(self, parent = None, id = -1, title = title): 
            wx.Frame.__init__( 
                self, parent, id, title, 
                style = wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE) 
            self.control = control(self,*args,**kwargs)
            self.Fit()
            self.Show(True) 
    app = wx.PySimpleApp() 
    frame = MainWindow() 
    app.MainLoop()
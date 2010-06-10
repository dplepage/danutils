from OpenGL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import sys
from danutils.misc import Trackball
import danutils.rainbow as rainbow

try:
    import wx
    from wx import glcanvas
except ImportError:
    raise ImportError, "Required dependency wx.glcanvas not present"
try:
    from OpenGL.GL import *
except ImportError:
    raise ImportError, "Required dependency OpenGL not present"

class MyCanvasBase(wx.glcanvas.GLCanvas):
    def __init__(self, parent,**kwargs):
        wx.glcanvas.GLCanvas.__init__(self, parent, -1, **kwargs)
        self.init = False
        # initial mouse position
        self.lastx = self.x = 0
        self.lasty = self.y = 0
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
        self.Bind(wx.EVT_MOTION, self.OnMouseMotion)

    def OnEraseBackground(self, event):
        pass # Do nothing, to avoid flashing on MSW.

    def OnSize(self, event):
        size = self.GetClientSize()
        if self.GetContext():
          self.SetCurrent()
          glViewport(0, 0, size.width, size.height)
        event.Skip()

    def OnPaint(self, event):
        dc = wx.PaintDC(self)
        self.SetCurrent()
        if not self.init:
          self.InitGL()
          self.init = True
        self.OnDraw()

    def OnMouseDown(self, evt):
        self.CaptureMouse()
        self.lastx,self.lasty = self.x, self.y = evt.GetPosition()

    def OnMouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def OnMouseMotion(self, evt):
        if evt.Dragging() and evt.LeftIsDown():
            self.lastx, self.lasty = self.x, self.y
            self.x, self.y = evt.GetPosition()
            self.Refresh(False)

class TrackballCanvas(MyCanvasBase):
    def __init__(self,parent,theta=45,phi=60,drawfunc=None,**kwargs):
        MyCanvasBase.__init__(self,parent,**kwargs)
        if drawfunc:
            self.draw = drawfunc
        self.trackball = Trackball(theta,phi)
        self.dist = -5
    
    def InitGL(self):
        self.setupGL()
        glFlush()

    def OnMouseMotion(self,evt):
        MyCanvasBase.OnMouseMotion(self,evt)
        if evt.LeftIsDown():
            w,h= map(float,self.GetSize())
            x = (self.lastx*2.0 - w)/float(w)
            y = (self.lasty*2.0 - w)/float(h)
            dx = (self.x-self.lastx)*2.0/float(w)
            dy = -(self.y-self.lasty)*2.0/float(h)
            if evt.CmdDown():
                self.dist += dy
            else:
                self.trackball.drag(x,y,dx,dy)

    def OnDraw(self):
        # set viewing projection
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(60,1,.01,1000)
        
        # position viewer
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        glTranslatef(0.0, 0.0, self.dist)
        glMultMatrixf(self.trackball.matrix)
        
        # clear color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT)
        glClear(GL_DEPTH_BUFFER_BIT)
        
        self.draw()
        glFlush() 
        self.SwapBuffers()

    def draw(self):
        raise NotImplementedError()
    
    def setupGL(self):
        pass

if __name__ == '__main__':
    def drawCube():
        # draw six faces of a cube
        glBegin(GL_QUADS)
        glNormal3f( 0.0, 0.0, 1.0)
        glVertex3f( 0.5, 0.5, 0.5)
        glVertex3f(-0.5, 0.5, 0.5)
        glVertex3f(-0.5,-0.5, 0.5)
        glVertex3f( 0.5,-0.5, 0.5)

        glNormal3f( 0.0, 0.0,-1.0)
        glVertex3f(-0.5,-0.5,-0.5)
        glVertex3f(-0.5, 0.5,-0.5)
        glVertex3f( 0.5, 0.5,-0.5)
        glVertex3f( 0.5,-0.5,-0.5)

        glNormal3f( 0.0, 1.0, 0.0)
        glVertex3f( 0.5, 0.5, 0.5)
        glVertex3f( 0.5, 0.5,-0.5)
        glVertex3f(-0.5, 0.5,-0.5)
        glVertex3f(-0.5, 0.5, 0.5)

        glNormal3f( 0.0,-1.0, 0.0)
        glVertex3f(-0.5,-0.5,-0.5)
        glVertex3f( 0.5,-0.5,-0.5)
        glVertex3f( 0.5,-0.5, 0.5)
        glVertex3f(-0.5,-0.5, 0.5)

        glNormal3f( 1.0, 0.0, 0.0)
        glVertex3f( 0.5, 0.5, 0.5)
        glVertex3f( 0.5,-0.5, 0.5)
        glVertex3f( 0.5,-0.5,-0.5)
        glVertex3f( 0.5, 0.5,-0.5)

        glNormal3f(-1.0, 0.0, 0.0)
        glVertex3f(-0.5,-0.5,-0.5)
        glVertex3f(-0.5,-0.5, 0.5)
        glVertex3f(-0.5, 0.5, 0.5)
        glVertex3f(-0.5, 0.5,-0.5)
        glEnd()

    app = wx.PySimpleApp()
    frame = wx.Frame(None, -1, 'GL Window',size=(200,200))
    win = TrackballCanvas(frame,drawfunc=drawCube)
    win.SetFocus()
    frame.Show()
    app.MainLoop()
    app.Destroy()
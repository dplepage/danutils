cdef extern from "stdlib.h":
    void* malloc(int size)
    void free(void* p)
    int sizeof(double)

cdef extern from "math.h":
    double c_pow "pow" (double x,double y)
    double c_sin "sin" (double x)
    double c_cos "cos" (double x)
    double c_tan "tan" (double x)
    double c_sinh "sinh" (double x)
    double c_cosh "cosh" (double x)
    double c_tanh "tanh" (double x)
    double c_log "log" (double x)
    double c_exp "exp" (double x)
    double c_sqrt "sqrt" (double x)
    double c_asin "asin" (double x)
    double c_acos "acos" (double x)
    double c_atan "atan" (double x)

import numpy
from numpy import sin,cos,tan,exp,log,sqrt
from numpy import sinh,cosh,tanh,arcsin,arccos,arctan

cdef extern from "mapwrapper.h":
    ctypedef struct iterator:
        int key()
        double val()
        void set(double d)
        void next()
        int neq "operator!="(iterator other)
    ctypedef struct derivarray:
        double get(int n)
        void set(int n, double d)
        iterator begin()
        iterator end()
        bint contains(int n)
    derivarray* new_derivarray "new derivarray"()
    void del_derivarray "delete" (derivarray *d)
    void addScaleDer(double scale, derivarray* src, derivarray* dst)

cdef class ValDer:
    cdef double v
    cdef int nd
    cdef derivarray* ds
    
    def __cinit__(self,int n):
        self.nd = n
        self.ds = new_derivarray()
    
    def __dealloc__(self):
        del_derivarray(self.ds)

    def __repr__(self):
        return 'valder('+`self.val`+','+`self.der`+')'
        
    def __str__(self):
        return '('+str(self.val)+','+str(self.der)+')'
    
    def __float__(self):
        return float(self.val)
    
    def __cmp__(self, xx):
        cdef ValDer x
        if type(xx)==ValDer:
            x = xx
            return cmp(self.v,x.v)
        else:
            return cmp(self.v,xx)
        
    def __neg__(self):
        cdef ValDer ans
        cdef int i
        ans = ValDer(self.nd)
        ans.v = -self.v
        addScaleDer(-1,self.ds,ans.ds)
        return ans
    
    def __pos__(self):
        return self
    
    def __add__(xx,yy):
        cdef ValDer ans,x,y
        cdef int i
        if type(yy)==numpy.ndarray:
            return NotImplemented   # let ndarray handle it
        elif type(xx)==ValDer:
            x = xx
            if type(yy)==ValDer:        
                y = yy
                if y.nd<>x.nd:
                    raise ValueError , 'mismatched ValDers -- %d,%d'%(x.nd,y.nd)
                ans = ValDer(x.nd)
                ans.v = x.v + y.v
                addScaleDer(1,x.ds,ans.ds)
                addScaleDer(1,y.ds,ans.ds)
                return ans
            else:
                ans = ValDer(x.nd)
                ans.v = x.v + yy
                addScaleDer(1,x.ds,ans.ds)
                return ans
        elif type(yy)==ValDer:
            y = yy
            ans = ValDer(y.nd)
            ans.v = y.v + xx
            addScaleDer(1,y.ds,ans.ds)
            return ans
        else: return NotImplemented
    
    def __sub__(xx,yy):
        cdef ValDer ans,x,y
        cdef int i
        if type(yy)==numpy.ndarray:
            return NotImplemented   # let ndarray handle it
        elif type(xx)==ValDer:
            x = xx
            if type(yy)==ValDer:        
                y = yy
                if y.nd<>x.nd:
                    raise ValueError , 'mismatched ValDers -- %d,%d'%(x.nd,y.nd)
                ans = ValDer(x.nd)
                ans.v = x.v - y.v
                addScaleDer(1,x.ds,ans.ds)
                addScaleDer(-1,y.ds,ans.ds)
                return ans
            else:
                ans = ValDer(x.nd)
                ans.v = x.v - yy
                addScaleDer(1,x.ds,ans.ds)
                return ans
        elif type(yy)==ValDer:
            y = yy
            ans = ValDer(y.nd)
            ans.v = xx - y.v
            addScaleDer(-1,y.ds,ans.ds)
            return ans
        else: return NotImplemented
    
    def __mul__(xx,yy):
        cdef ValDer ans,x,y
        cdef double xd,yd
        cdef int i
        if type(yy)==numpy.ndarray:
            return NotImplemented   # let ndarray handle it
        elif type(xx)==ValDer:
            x = xx
            if type(yy)==ValDer:        
                y = yy
                if y.nd<>x.nd:
                    raise ValueError , 'mismatched ValDers -- %d,%d'%(x.nd,y.nd)
                ans = ValDer(x.nd)
                ans.v = x.v * y.v
                addScaleDer(y.v,x.ds,ans.ds)
                addScaleDer(x.v,y.ds,ans.ds)
                return ans
            else:
                yd = yy
                ans = ValDer(x.nd)
                ans.v = x.v * yd
                addScaleDer(yd,x.ds,ans.ds)
                return ans
        elif type(yy)==ValDer:
            y = yy
            xd= xx
            ans = ValDer(y.nd)
            ans.v = xd * y.v
            addScaleDer(xd,y.ds,ans.ds)
            return ans
        else: return NotImplemented
    
    def __div__(xx,yy):
        cdef ValDer ans,x,y
        cdef double xd,yd,iy,iy2,ix,ix2
        cdef int i
        if type(yy)==numpy.ndarray:
            return NotImplemented   # let ndarray handle it
        elif type(xx)==ValDer:
            x = xx
            if type(yy)==ValDer:        
                y = yy
                if y.nd<>x.nd:
                    raise ValueError , 'mismatched derivatives -- %d,%d'%(x.nd,y.nd)
                ans = ValDer(x.nd)
                iy = 1./y.v
                ans.v = x.v * iy
                iy2 = iy*iy
                addScaleDer(iy,x.ds,ans.ds)
                addScaleDer(-x.v*iy2,y.ds,ans.ds)
                return ans
            else:
                iy = 1./yy
                ans = ValDer(x.nd)
                ans.v = x.v * iy
                addScaleDer(iy,x.ds,ans.ds)
                return ans
        elif type(yy)==ValDer:
            y = yy
            xd= xx
            iy= 1./y.v
            ans = ValDer(y.nd)
            ans.v = xd*iy
            iy2 = -iy*iy
            addScaleDer(xd*iy2,y.ds,ans.ds)
            return ans
        else: return NotImplemented
    
    def __truediv__(xx,yy):
        cdef ValDer ans,x,y
        cdef double xd,yd,iy,iy2,ix,ix2
        cdef int i
        if type(yy)==numpy.ndarray:
            return NotImplemented   # let ndarray handle it
        elif type(xx)==ValDer:
            x = xx
            if type(yy)==ValDer:        
                y = yy
                if y.nd<>x.nd:
                    raise ValueError , 'mismatched derivatives -- %d,%d'%(x.nd,y.nd)
                ans = ValDer(x.nd)
                iy = 1./y.v
                ans.v = x.v * iy
                iy2 = iy*iy
                addScaleDer(iy,x.ds,ans.ds)
                addScaleDer(-x.v*iy2,y.ds,ans.ds)
                return ans
            else:
                iy = 1./yy
                ans = ValDer(x.nd)
                ans.v = x.v * iy
                addScaleDer(iy,x.ds,ans.ds)
                return ans
        elif type(yy)==ValDer:
            y = yy
            xd= xx
            iy= 1./y.v
            ans = ValDer(y.nd)
            ans.v = xd*iy
            iy2 = -iy*iy
            addScaleDer(xd*iy2,y.ds,ans.ds)
            return ans
        else: return NotImplemented
    
    def __pow__(xx,yy,zz):
        cdef ValDer ans,x,y
        cdef double xd,yd,f1,f2
        cdef int i
        if type(yy)==numpy.ndarray:
            return NotImplemented   # let ndarray handle it
        elif type(xx)==ValDer:
            x = xx
            if type(yy)==ValDer:        
                y = yy
                if y.nd<>x.nd:
                    raise ValueError , 'mismatched ValDers -- %d,%d'%(x.nd,y.nd)
                ans = ValDer(x.nd)
                ans.v = c_pow(x.v,y.v)
                f1 = c_pow(x.v,y.v-1.)*y.v
                f2 = ans.v*c_log(x.v)
                addScaleDer(f1,x.ds,ans.ds)
                addScaleDer(f2,y.ds,ans.ds)
                return ans
            else:
                yd = yy
                ans = ValDer(x.nd)
                ans.v = c_pow(x.v,yd)
                f1 = c_pow(x.v,yd-1)*yd
                addScaleDer(f1,x.ds,ans.ds)
                return ans
        elif type(yy)==ValDer:
            y = yy
            xd= xx
            ans = ValDer(y.nd)
            ans.v = c_pow(xd,y.v)
            f1 = ans.v*c_log(xd)
            addScaleDer(f1,y.ds,ans.ds)
            return ans
        else: return NotImplemented
    
    def sin(self):
        cdef ValDer ans
        cdef int i
        cdef double cv
        ans = ValDer(self.nd)
        ans.v = c_sin(self.v)
        cv = c_cos(self.v)
        addScaleDer(cv,self.ds,ans.ds)
        return ans
    
    def cos(self):
        cdef ValDer ans
        cdef int i
        cdef double msv
        ans = ValDer(self.nd)
        ans.v = c_cos(self.v)
        msv = -c_sin(self.v)
        addScaleDer(msv,self.ds,ans.ds)
        return ans
    
    def tan(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_tan(self.v)
        f = 1+ans.v**2
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def arcsin(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_asin(self.v)
        f = 1./(1-self.v**2)**0.5
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def asin(self): return self.arcsin()
    
    def arccos(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_acos(self.v)
        f = -1./(1-self.v**2)**0.5
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def acos(self): return self.arccos()
    
    def arctan(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_atan(self.v)
        f = 1./(1+self.v**2)
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def atan(self): return self.arctan()
    
    def sinh(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_sinh(self.v)
        f = c_cosh(self.v)
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def cosh(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_cosh(self.v)
        f = c_sinh(self.v)
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def tanh(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_tanh(self.v)
        f = 1/(c_cosh(self.v)**2)
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def exp(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_exp(self.v)
        f = ans.v
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def log(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_log(self.v)
        f = 1./self.v
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    def sqrt(self):
        cdef ValDer ans
        cdef int i
        cdef double f
        ans = ValDer(self.nd)
        ans.v = c_sqrt(self.v)
        f = 0.5/ans.v
        addScaleDer(f,self.ds,ans.ds)
        return ans
    
    property val:
        def __get__(self):
            return self.v

    property der:
        def __get__(self):
            d = numpy.zeros(self.nd,float)
            cdef iterator i = self.ds.begin()
            while i.neq(self.ds.end()):
                d[i.key()] = i.val()
                i.next()
            return d


def valder(double v,d):
    """ valder(v,d) creates a ValDer with x.val=v and x.der=d
    
    The value v should be a number; the derivative d can
    be any sort of sequence of numbers (eg, a numpy.array
    or an ordinary list). d is converted into a numpy.array 
    and then flattened.
    """
    cdef nd,i
    cdef ValDer vd
    d = numpy.asarray(d).flatten()
    nd = len(d)
    vd = ValDer(nd)
    vd.v = v
    for i from 0<=i<nd:
        vd.ds.set(i,d[i])
    return vd

def valder_var(vv):
    """ valder_var(val_array) -> numpy.array(ValDers)
    
    Given an array (or list) val_array of numbers,
    valder_var(val_array) creates an array of ValDers
    representing independent variables with respect to 
    which derivatives will be evaluated. The shape of the 
    array of ValDers is the same as that of val_array,
    and the values of the independent variables are those
    specified in val_array. The order of the derivatives
    in each ValDer is the order in which the variables 
    would appear in val_array were it flattened by numpy.
    """
    cdef int nv,i
    cdef ValDer vd
    va = numpy.asarray(vv)
    v = va.flat
    nv = len(v)
    ans = nv*[None]
    for i from 0<=i<nv:
        vd = ValDer(nv)
        vd.v = v[i]
        vd.ds.set(i,1.0)
        ans[i] = vd
    ans = numpy.array(ans)
    ans.shape = va.shape
    return ans

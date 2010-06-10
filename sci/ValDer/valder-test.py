""" exercises ValDer
    
Created by G. Peter Lepage on 2008-02-12.
Copyright (c) 2008 G. Peter Lepage (Cornell University). All rights reserved.
"""

from numpy import array,arange,cos,set_printoptions
from ValDer import *

set_printoptions(precision=4,suppress=True) # numpy strings

def pr(x):  
    if type(x)==ValDer:
        print x.val,x.der
    else:
        for xx in x:
            pr(xx)
def optester(z):
    x,y = valder_var([3.,4.])
    print 30*'=',type(z)
    xx = z+x+z+y
    print '+',xx
    xx = z*x*z*y
    print '*',xx
    xx = z-x-z-y
    print '-',xx
    xx = z/x/z/y
    print '/',xx
    print

optester(100.)
optester(arange(1.,5.))

def timetester(n):
    print 30*'=','time test'
    x,y = valder_var(arange(3.,33.))[:2]
    x = arange(1.,23.*16)*x+y   # 23 time steps, 4x4 matrix
    for i in range(n):
        a = cos(x+ x*x + 2*x*x*x + x*x*x*x + i)

    print a[0]
    print a[-1]
    try:
        print len(a),a.itemsize
    except: pass
    print

timetester(200)



def oldtests():
    def fmt(a,b):
        return '('+str(a)+','+str(array(b))+')'
    # array = numpy.array
    print 30*'=','old tests'
    x,y = valder_var(array([2.,5.]))
    print 'x,y',x,y
    print 'x',x.val,x.der
    print 'add,radd',x+y+(2+x)+(x+2),fmt(15.,[3.,1.])
    print 'sub,rsub',x-y+(2-x)+(x-2),fmt(-3.,[1.,-1.])
    print 'mul,rmul',x*y+y*x+(7.*x)+(y*7.),fmt(69.,[17.,11.])
    print 'div,rdiv\n   ',5*x/y+2*y/x+(2./x)+(y/5.),'\n   ',fmt(9.,[-2.,.80])
    print 'pow,rpow\n   ',x**y+x**2+2**y,'\n   ',fmt(68., [84., 44.36141956])
    print 'exp\n   ',exp(x),'\n   ',fmt(7.38905609893,[7.389056099, 0.])
    print 'log\n   ',log(x),'\n   ',fmt(.69314718056,[.5000000000, 0.])
    print 'sqrt\n   ',sqrt(x),'\n   ',fmt(1.41421356237, [.3535533905, 0.])
    print 'sin\n   ',sin(x),'\n   ',fmt(.909297426826, [-.4161468365, 0.])
    print 'cos\n   ',cos(x),'\n   ',fmt(-.416146836547, [-.9092974268, 0.])
    print 'tan\n   ',tan(x),'\n   ',fmt(-2.18503986326, [5.774399203, 0.])
    print 'arcsin',arcsin(sin(x-1)),x-1
    print 'arccos',arccos(cos(x-1)),x-1
    print 'arctan',arctan(tan(x-1)),(x-1)
    print 'sinh\n   ',sinh(x),'\n   ',fmt(3.62686040785, [3.762195691, 0.])
    print 'cosh\n   ',cosh(x),'\n   ',fmt(3.76219569108, [3.626860408, 0.])
    print 'tanh\n   ',tanh(x),'\n   ',fmt(.964027580076, [.706508248e-1, 0.])
    print 'misc',sqrt(exp(log(y))**2),y
    print 'misc',exp(log(y)),y
    print 'misc',sqrt(sin(y)**2+cos(y)**2),1.
    print 'misc',tan(x)*cos(x)/sin(x),1.
    print 'misc',sqrt(-sinh(y)**2+cosh(y)**2),1.
    print 'misc',tanh(x)*cosh(x)/sinh(x),1.
    
    xy = valder_var(array([[2.,5.],[3.,5.]]))
    x,y = xy
    print 'array x',x
    print 'array y',y
    print 'dx**2/dx - 2x',(x[0]**2).der[0]-(2*x[0]).val,0.0
    print 'dx**2/dx - 2x',(x[1]**2).der[1]-(2*x[1]).val,0.0
    y = sin(y)
    print 'dsin(y)/dx',y[0].der[0],0.0
    print 'dsin(y)/dy**2+sin(y)**2',y[0].val**2+y[0].der[2]**2,1
    print 'dsin(y)/dy**2+sin(y)**2',y[1].val**2+y[1].der[3]**2,1
    print 'x,y',xy
    
    print

oldtests()

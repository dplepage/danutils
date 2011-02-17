import ValDer as NewValDer

import sys
sys.path = ['/Users/dplepage/Downloads/lsqfit/']+sys.path
del sys.modules['ValDer']
import ValDer as OldValDer

import time
from numpy import *
asin = arcsin
atan = arctan
acos = arccos

def test(mod):
	x,y = mod.valder_var([.5,-.3])
	results = []
	results.append(("x      ",x      ))
	results.append(("-x     ",-x     ))
	results.append(("x+2    ",x+2    ))
	results.append(("x-2    ",x-2    ))
	results.append(("x/2    ",x/2    ))
	results.append(("x*2    ",x*2    ))
	results.append(("x**2   ",x**2   ))
	results.append(("2+x    ",2+x    ))
	results.append(("2-x    ",2-x    ))
	results.append(("2/x    ",2/x    ))
	results.append(("2*x    ",2*x    ))
	results.append(("2**x   ",2**x   ))
	results.append(("x<10   ",x<10   ))
	results.append(("x>10   ",x>10   ))
	results.append(("x<100  ",x<100  ))
	results.append(("x>100  ",x>100  ))
	results.append(("sin (x)",sin (x)))
	results.append(("cos (x)",cos (x)))
	results.append(("tan (x)",tan (x)))
	results.append(("asin(x)",asin(x)))
	results.append(("acos(x)",acos(x)))
	results.append(("atan(x)",atan(x)))
	results.append(("sinh(x)",sinh(x)))
	results.append(("cosh(x)",cosh(x)))
	results.append(("tanh(x)",tanh(x)))
	results.append(("exp (x)",exp (x)))
	results.append(("log (x)",log (x)))
	results.append(("sqrt(x)",sqrt(x)))
	results.append(("x+y    ",x+y    ))
	results.append(("x-y    ",x-y    ))
	results.append(("x*y    ",x*y    ))
	results.append(("x/y    ",x/y    ))
	results.append(("x**y   ",x**y   ))
	results.append(("x<y    ",x<y    ))
	results.append(("x>y    ",x>y    ))
	results.append(("y<x    ",y<x    ))
	results.append(("y>x    ",y>x    ))
	results.append(("sin(exp(sqrt((x+100)**(y/x))))",sin(y*exp(sqrt((x+100)**(-y/x))))    ))
	return results


r1 = test(OldValDer)
r2 = test(NewValDer)

errors = []
for a,b in zip(r1,r2):
	s = a[0]
	a = a[1]
	b = b[1]
	if type(a) == OldValDer.ValDer:
		v1,d1 = a.val, a.der
		v2,d2 = b.val, b.der
		if v1 != v2 or (d1 != d2).any():
			errors.append("%s:\n\t%s\n\t%s"%(s,a,b))
	else:
		if a != b:
			errors.append("%s:\n\t%s\n\t%s"%(s,a,b))

if not errors:
	print "All tests passed."
else:
	print "Some tests failed:"
	print '\n'.join(errors)


def timetest1(mod,r):
	"""
	A very sparse computation
	"""
	t0 = time.time()
	x = mod.valder_var(r)
	x = x*x + x**x + x/x
	x = sin(exp(x))
	t1 = time.time()
	return t1-t0,x[0]

def timetest2(mod,r):
	"""
	A computation that is sparse until the last step
	"""
	t0 = time.time()
	x = mod.valder_var(r)
	x = x*x + x**x + x/x
	x = sin(exp(x))
	x = dot(x,x)
	t1 = time.time()
	return t1-t0,x

def timetest3(mod,r):
	"""
	A dense computation
	"""
	t0 = time.time()
	x = mod.valder_var(r)
	x = dot(x,x)
	x = x*x + x**x + x/x
	x = sin(exp(x))
	t1 = time.time()
	return t1-t0,x


def timetests():
	for i,f in enumerate([timetest1,timetest2,timetest3]):
		print "Time test %i (n=1000):"%i
		r = random.random(1000)
		told,vold = f(OldValDer,r)
		tnew,vnew = f(NewValDer,r)
		print " Old time:  %.5f"%told
		print " New time:  %.5f"%tnew
	for i,f in enumerate([timetest1,timetest2,timetest3]):
		print "Time test %i (n=10000):"%i
		# Note that if we included OldValDer, calling
		# valder_var alone would run us out of memory
		r = random.random(10000)
		tnew,vnew = f(NewValDer,r)
		print " New time:  %.5f"%tnew

print "="*50
timetests()

# print "All test results:"
# for a,b in zip(r1,r2):
# 	s = a[0]
# 	a = a[1]
# 	b = b[1]
# 	print "%s:\n\t%s\n\t%s"%(s,a,b)

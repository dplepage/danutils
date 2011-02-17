import livedebugger
import sys
import os
livedebugger.listen()
print "Running %s with pid %i"%(sys.argv[0],os.getpid())
print "To interrupt for live debugging, run:"
print "livedebugger.py %i"%(os.getpid())

from itertools import count
from time import sleep


i = 0
delay = 2
while 1:
	i += 1
	print "Set i to %i; sleeping for %is"%(i,delay)
	sleep(delay)
	print "Slept for %is; awoke with i=%i"%(delay,i)
	if i > 100:
		print "i > 100, exiting"
		sys.exit(0)
	else:
		print "i <= 100, continuing"

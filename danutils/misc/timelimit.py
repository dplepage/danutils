#!/usr/bin/env python

'''This module exposes function  timelimited and two
   classes  TimeLimited and  TimeLimitExpired.

   Function  timelimited can be used to invoke any
   callable object with a time limit.

   Class  TimeLimited wraps any callable object into
   a time limited callable with the same signature.

   No signals or timers are affected and any errors are
   propagated as usual.  Decorators and with statements
   are avoided for backward compatibility.

   Tested with Python 2.2.3, 2.3.7, 2.4.5, 2.5.2, 2.6.2
   or 3.0.1 on CentOS 4.7, MacOS X 10.4.11 Tiger (Intel)
   and 10.3.9 Panther (PPC), Solaris 10 and Windows XP.

   Note, for Python 2.5 and below, replace 'as' with
   a comma in the 3 except lines marked #XXX below.

   The core of function  timelimited is copied from
   <http://code.activestate.com/recipes/473878/>.
'''
__all__ = ['timelimited', 'TimeLimited', 'TimeLimitExpired']

from threading import Thread


class TimeLimitExpired(Exception):
    '''Exception raised when time limit expires.
    '''
    pass


def timelimited(timeout, function, *args, **kwds):
    '''Invoke the given function with the positional and
       keyword arguments.

       The function result is returned if the function
       finishes within the given time limit, otherwise
       a TimeLimitExpired error is raised.

       The timeout value is in seconds and has the same
       resolution as the standard time.time function.  A
       timeout value of None invokes the given function
       without imposing any time limit.

       A TypeError is raised if  function is not callable,
       a ValueError is raised for negative  timeout values
       and any errors occurring inside the function are
       passed along as-is.
    '''
    class _Interruptable(Thread):
        _error_  = TimeLimitExpired  # assume timeout
        _result_ = None

        def run(self):
            try:
                self._result_ = function(*args, **kwds)
                self._error_ = None
            except Exception, e:
                self._error_ = e

    if not hasattr(function, '__call__'):
        raise TypeError('function not callable: %s' % repr(function))

    if timeout is None:  # shortcut
        return function(*args, **kwds)

    if timeout < 0:
        raise ValueError('timeout invalid: %s' % repr(timeout))

    t = _Interruptable()
    t.start()
    t.join(timeout)

    if t._error_ is None:
        return t._result_
    if t._error_ is TimeLimitExpired:
        raise TimeLimitExpired('timeout %r for %s' % (timeout, repr(function)))
    else:
        raise t._error_


class TimeLimited(object):
    '''Create a time limited version of any callable.

       Fro example, to limit function f to t seconds,
       first create a time limited version of f.

         from timelimited import *

         f_t = TimeLimited(t, f)

      Then instead of invoking f(...), use f_t like

         try:
             r = f_t(...)
         except TimeLimitExpired:
             r = ...  # timed out

    '''
    def __init__(self, function, timeout=0):
        '''See function  timelimited for a description
           of the  function and  timeout arguments.
        '''
        self._function = function
        self._timeout  = timeout

    def __call__(self, *args, **kwds):
        '''See function  timelimited for a description
           of the behavior.
        '''
        return timelimited(self._timeout, self._function, *args, **kwds)

    def __str__(self):
        return '<%s of %r, timeout=%s>' % (repr(self)[1:-1], self._function, self._timeout)

    def _timeout_get(self):
        return self._timeout
    def _timeout_set(self, timeout):
        self._timeout = timeout
    timeout = property(_timeout_get, _timeout_set)
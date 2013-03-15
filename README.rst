====================================================
 danutils - random functions that do who knows what
====================================================

My name is Daniel Lepage (http://www.dplepage.com), and you probably don't need
this module. It's full of random hacks that I've accumulated and needed
somewhere to stick. Many of them probably no longer work. Some that do may be
stupid ways of doing things, or obsoleted by real libraries.

Some parts are also copied directly from blogs or whatnot; such code is marked
accordingly.

I'm only putting this online so that I can use it when I need to work with some
old thing I developed that relies on some random function from here. Feel free
to use it, but do so at your own risk.

``danutils.do.interact``
========================

Honestly, the only part of this library that I ever use on a regular basis is
the module ``danutils.do.interact``, which, upon import, harvests the locals and
globals at that point, calls ``code.interact`` with that namespace, and then
deletes itself from the module registry so that it can be imported again. I use
it when debugging::

  def foo(x):
    from danutils.do.interact import doit
    return x*x

If you run the above, you'll suddenly find yourself at a command line at that
point where you imported interact.

Some other things, at least the ones that I remember what they do, are:

``danutils.cuda``
=================

Some automation tools for building modules with pyCuda. I'm pretty sure various
other libraries have made this ENTIRELY OBSOLETE.

``danutils.img``
================

Looks like a bunch of image manipulation functions that are probably already
provided by some library or other. Also includes a python wrapper around
`Christopher de Coro's image debugger`_

_: http://www.cs.princeton.edu/~cdecoro/imagedb/

``danutils.lib``
================

Automatically compile really simple c projects and then import them via ctypes.

``danutils.pandatools``
=======================

From one time when I was playing around with the Panda3D_ game engine.

_: http://www.panda3d.org/

``danutils.sci``
================

A few tools I used back when I did a lot of numpy/scipy work. These days you're
probably better off with IPython's notebooks, and tools like PyTables or Blaze.

``danutils.wx``
===============

Some wx widgets I built, also when I did a lot of numpy/scipy stuff.

``danutils.misc``
=================

Could be pretty much anything in here.

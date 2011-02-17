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

/*
A wrapper around unordered_map
because Cython doesn't play well with operator[]
*/

#ifndef MAPWRAP_H
#define MAPWRAP_H
#include <map>
#include <tr1/unordered_map>
using std::tr1::unordered_map;

typedef unordered_map<int,double> imap;

class iterator {
private:
	imap::iterator it;
public:
	iterator() : it(NULL) {}
	iterator(imap::iterator it) : it(it) {}
	int key() {return it->first;}
	double val() {return it->second;}
	void set(double d) {it->second=d;}
	void next() {it++;}
	bool operator!=(const iterator& other) {
		return it != other.it;
	}
};

class derivarray {
public:
	derivarray() {}
	~derivarray() {}
	
	int contains(int i) {return mymap.count(i);}
	double get(int i) {
		if(contains(i))
			return mymap[i];
		return 0;
	}
	void set(int i,double d) {
		mymap[i] = d;
	}
	iterator begin() {return iterator(mymap.begin());}
	iterator end() {return iterator(mymap.end());}
private:
	imap mymap;
};

void addScaleDer(double scale, derivarray* src, derivarray* dst){
	for(iterator it = src->begin(); it != src->end(); it.next())
		dst->set(it.key(),scale*it.val()+dst->get(it.key()));
}

#endif

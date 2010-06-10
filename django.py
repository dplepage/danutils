def sdesc(desc):
    def wrap(f):
        f.short_description = desc
        return f
    return wrap
    
def asList(name,desc=None):
    if desc is None:
        desc = name
    @sdesc(desc)
    def tmp(ob):
        return ", ".join([unicode(a) for a in getattr(ob,name).all()]) or "No %s"%desc
    return tmp


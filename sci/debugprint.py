import numpy as np

def alignpair(*arrs, **kwargs):
    show_errs = kwargs.get('show_errs',True)
    # Convert list of arrays to list of lists of strings
    arrlists = [a.__str__().splitlines() for a in arrs]
    # Compute length of longest line in any array
    max_len = max(max(len(line) for line in arrlist) for arrlist in arrlists)
    for arrlist in arrlists:
        arrlist[:] = ["{0:{1}}".format(line, max_len) for line in arrlist]
    # Format string for printing a line of arrays
    fmt = ("%%-%is\t"%max_len)*len(arrs)
    out = []
    for lines_as_lists in zip(*arrlists):
        lines_as_lists = map(list, lines_as_lists)
        if show_errs:
            for i in range(max_len):
                items = [line[i] for line in lines_as_lists]
                if any(x != items[0] for x in items):
                    for line in lines_as_lists:
                        line[i] = "\x1b[7m%s\x1b[0m"%line[i]
        lines_as_strings = map(''.join, lines_as_lists)
        out.append(fmt%tuple(lines_as_strings))
    return '\n'.join(out)

def debugCmp(*arrs, **kwargs):
    shape = kwargs.get('shape')
    if shape:
        arrs = [arr.reshape(shape) for arr in arrs]
    if all(np.allclose(arrs[0],a) for a in arrs[1:]): return True
    for arr in arrs[1:]:
        assert arrs[0].shape == arr.shape, (
            "Shape mismatch: {0} vs {1}".format(arrs[0].shape, arr.shape))
    nd = arrs[0].ndim
    if nd <= 2:
        print alignpair(*arrs)
    else:
        for i in range(arrs[0].shape[0]):
            debugCmp(*[a[i,:] for a in arrs])
    return False
    
def assertCmp(a1, a2, shape = None):
    if shape:
        a1 = a1.reshape(shape)
        a2 = a2.reshape(shape)
    assert a1.shape == a2.shape, (
        "Shape mismatch: {0} vs {1}".format(a1.shape, a2.shape))
    nd = a1.ndim
    if np.allclose(a1,a2): return
    if nd <= 2:
        print alignpair(a1,a2)
    else:
        for i in range(a1.shape[0]):
            print alignpair(a1[i,:],a2[i,:])
    assert False
    


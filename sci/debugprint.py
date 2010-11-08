import numpy as np

def alignpair(a1,a2):
    s1 = a1.__str__().splitlines()
    s2 = a2.__str__().splitlines()
    l = max(len(l) for l in s1)
    fmt = "%%-%is\t%%s"%l
    a = []
    for l1,l2 in zip(s1, s2):
        l1,l2 = list(l1), list(l2)
        for i in range(len(l1)):
            if i < len(l2):
                if l1[i] != l2[i]:
                    l1[i] = "\x1b[7m%s\x1b[0m"%l1[i]
                    l2[i] = "\x1b[7m%s\x1b[0m"%l2[i]
        l1, l2 = ''.join(l1),''.join(l2)
        a.append(fmt%(l1,l2))
    return '\n'.join(a)

def debugCmp(a1, a2, shape = None):
    if shape:
        a1 = a1.reshape(shape)
        a2 = a2.reshape(shape)
    if np.allclose(a1,a2): return True
    assert a1.shape == a2.shape, (
        "Shape mismatch: {0} vs {1}".format(a1.shape, a2.shape))
    nd = a1.ndim
    if nd <= 2:
        print alignpair(a1,a2)
    else:
        for i in range(a1.shape[0]):
            debugCmp(a1[i,:],a2[i,:])
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
    


try:
    from itertools import product
except ImportError:
    def product(*sets):
        wheels = map(iter, sets) # wheels like in an odometer
        digits = [it.next() for it in wheels]
        while True:
            yield digits[:]
            for i in range(len(digits)-1, -1, -1):
                try:
                    digits[i] = wheels[i].next()
                    break
                except StopIteration:
                    wheels[i] = iter(sets[i])
                    digits[i] = wheels[i].next()
            else:
                break
    
def crossrange(*args):
    '''
    Example:
    >>> list(utils.crossrange(5,3))
    [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2), (2, 0), (2, 1), (2, 2), 
    (3, 0), (3, 1), (3, 2), (4, 0), (4, 1), (4, 2)]
    '''
    return product(*[xrange(arg) for arg in args])
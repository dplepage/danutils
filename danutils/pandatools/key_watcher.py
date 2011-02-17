from direct.showbase.DirectObject import DirectObject

class KeyWatcher(DirectObject):
    """
    A KeyWatcher monitors every key added via the watchKey function and knows
    whether each is currently up or down
    """

    def __init__(self):
        DirectObject.__init__(self)
        self.keyMap = {}

    def watchKey(self,key,alias=None):
        alias = alias or key
        if alias in self.keyMap.keys():
            return
        self.keyMap[alias] = 0
        self.accept(key,self.setKey,[alias,1])
        self.accept(key+"-up",self.setKey,[alias,0])

    def setKey(self,key,value):
       self.keyMap[key] = value

    def isWatching(self,key):
       return key in self.keyMap.keys()

    def getKey(self,key):
       return self.keyMap.get(key,0)

arrow_watcher = None
def getArrowWatcher():
    global arrow_watcher
    if arrow_watcher == None:
        arrow_watcher = KeyWatcher()
        arrow_watcher.watchKey("arrow_up","up")
        arrow_watcher.watchKey("arrow_down","down")
        arrow_watcher.watchKey("arrow_left","left")
        arrow_watcher.watchKey("arrow_right","right")
    return arrow_watcher

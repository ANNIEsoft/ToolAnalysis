#### my python value Generator
import Store

def Initialise():
    return 1

def Finalise():
    return 1

def Execute():
    print'using python to change values'
    Store.SetInt('a',6)
    Store.SetDouble('b',8.0)
    Store.SetString('c','hello')
    return 1


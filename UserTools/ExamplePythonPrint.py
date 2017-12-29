#####my python print script
import Store

def Initialise():
    return 1

def Finalise():
    return 1

def Execute():
    a=Store.GetInt('a')
    b=Store.GetDouble('b')
    c=Store.GetString('c')
    print a
    print b
    print c
    return 1

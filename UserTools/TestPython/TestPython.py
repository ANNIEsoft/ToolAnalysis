# python Tool script
# ------------------
from Tool import *

class TestPython(Tool):
    
    # declare member variables here
    demovariable = 'TestString'
    
    def Initialise(self):
        
        print("verbosity is: ",self.m_verbosity)
        self.m_log.Log(__file__+" Initialising", self.v_debug, self.m_verbosity)
        print("config variables passed: ", end=' ')
        self.m_variables.Print()
        
        return 1
    
    def Execute(self):
        self.m_log.Log(__file__+" Executing", self.v_debug, self.m_verbosity)
        
        # set python variables
        somefloat = 3.334
        somebool = True
        somestring = std.string("teststing")
        
        # construct a c++ class - a simple ASCII Store
        astore = cppyy.gbl.Store()
        
        # call some methods
        astore.Set("myfloat",somefloat)
        astore.Set("mybool",somebool)
        astore.Set("mystring",somestring)
        #print("showing constructed Store contents: ")
        #astore.Print()
        
        # another c++ class - a BoostStore
        abstore = cppyy.gbl.BoostStore()
        print("constructed: ",type(abstore))
        # construct the BoostStore on the heap - i.e. prevent python from removing it at the
        # end of this function scope.
        abstore.__python_owns__ = False
        
        # The BoostStore is more flexible than the Store class in that it can hold any data type
        # such as, for example, a Store class instance!
        abstore.Set("myStore",astore)
        # or a std vector of doubles. Note that when declaring stl containers in python
        # we need to enclose the contained type name in single quotes!
        vect_o_dubs = std.vector['double'](range(1,10))
        abstore.Set("myDoubles",vect_o_dubs)
        
        # put the BoostStore into the DataModel - m_data.Stores is a map of string to BoostStore*.
        print("Data model stores length: ", self.m_data.Stores.size())
        self.m_data.Stores["myBStore"] = abstore
        print("Data model stores length: ", self.m_data.Stores.size())
        
        return 1
    
    def Finalise(self):
        
        self.m_log.Log(__file__+" Finalising", self.v_debug, self.m_verbosity)
        
        # when c++ functions accept arguments by reference or pointer we need
        # to wrap them in a class. If the argument type is already a class
        # we don't need to do anything, but basic types need a ctype wrapper
        somefloat_ref = ctypes.c_float()
        somebool_ref = ctypes.c_bool()
        somestring = std.string()
        
        # Retrieve the BoostStore* we made before
        bbstore = self.m_data.Stores.at("myBStore")
    
        # check it lists our Store as present
        bbstore.Print()
        ok = bbstore.Has("myStore")
        print("BoostStore has entry myStore: ",ok)
        print("type of myStore entry is :",bbstore.Type("myStore"))
        
        print("get Store from BoostStore")
        mystore = cppyy.gbl.Store()
        ok = bbstore.Get("myStore",mystore)
    
        print("ok is ",ok)
        print("mystore is ",type(mystore))
    
        print("print contents")
        mystore.Print()
        
        # retrieving stl containers such as std::vector, std::string, std::map etc
        # is done in the same was as for classes, so we don't need a ctypes reference
        v_dubs = std.vector['double']()
        abstore.Get("myDoubles",v_dubs)
    
        # let's unpack our Store class to check it has the right contents
        # first reinit our local variables to defaults
        print("reinit locals")
        somefloat_ref.value = -1
        somebool_ref.value = False
        somestring = std.string("")
    
        # get the contents of the Store
        print("retrieve from Store")
        mystore.Get("myfloat",somefloat_ref)
        mystore.Get("mybool",somebool_ref)
        mystore.Get("mystring",somestring)
    
        # print to check
        print("float is ",somefloat_ref.value,
              "bool is ", somebool_ref.value,
              "string is",somestring)
    
        # this is a suitable way to 'delete' heap objects
        bbstore.__python_owns__ = True
        self.m_data.Stores.erase("myBStore")
        
        return 1

#################
#  Boilerplate  #
# Do Not Change #
#################

thistool = TestPython()

def SetToolChainVars(m_data_in, m_variables_in, m_log_in):
    return thistool.SetToolChainVars(m_data_in, m_variables_in, m_log_in)

def Initialise():
    print("calling initialise")
    return thistool.Initialise()

def Execute():
    return thistool.Execute()

def Finalise():
    return thistool.Finalise()

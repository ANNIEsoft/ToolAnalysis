ToolDAQFrameworkPath=ToolDAQ/ToolDAQFramework

CPPFLAGS= -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable

PythonLib= `python-config --libs` -L $(PYTHON_LIB)/../src/Python-2.7.11
PythonInclude=`python-config --cflags`

ZMQLib= -L ToolDAQ/zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I ToolDAQ/zeromq-4.0.7/include/ 

BoostLib= -L $(BOOST_LIB) -lboost_date_time -lboost_serialization -lboost_iostreams 
BoostInclude= -I $(BOOST_INC)

RootInclude=  -I $(ROOT_INC)
RootLib= -L $(ROOT_INC)/../lib `root-config --libs` -lMinuit

DataModelInclude = $(RootInclude)
DataModelLib = $(RootLib)

MyToolsInclude =  $(RootInclude) $(PythonInclude)
MyToolsLib = $(RootLib) $(PythonLib)


all: lib/libMyTools.so lib/libToolChain.so lib/libStore.so include/Tool.h  lib/libServiceDiscovery.so lib/libDataModel.so lib/libLogging.so

	g++ $(CPPFLAGS) -std=c++1y -g src/main.cpp -o Analyse -I include -L lib -lStore -lMyTools -lToolChain -lDataModel -lLogging -lServiceDiscovery -lpthread $(DataModelInclude) $(MyToolsInclude)  $(MyToolsLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)


lib/libStore.so:

	cp $(ToolDAQFrameworkPath)/src/Store/*.h include/
	g++ $(CPPFLAGS) -std=c++1y -g -fPIC -shared  -I include $(ToolDAQFrameworkPath)/src/Store/*.cpp -o lib/libStore.so $(BoostLib) $(BoostInclude)


include/Tool.h:

	cp $(ToolDAQFrameworkPath)/src/Tool/Tool.h include/


lib/libToolChain.so: lib/libStore.so include/Tool.h lib/libDataModel.so lib/libMyTools.so lib/libServiceDiscovery.so lib/libLogging.so

	cp $(ToolDAQFrameworkPath)/src/ToolChain/*.h include/
	g++ $(CPPFLAGS) -g -fPIC -shared $(ToolDAQFrameworkPath)/src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lMyTools -lServiceDiscovery -lLogging -o lib/libToolChain.so $(DataModelInclude) $(ZMQLib) $(ZMQInclude) $(MyToolsInclude)  $(BoostLib) $(BoostInclude)


clean: 
	rm -f include/*.h include/*.hh include/*.C include/*.cxx
	rm -f lib/*.so
	rm -f Analyse

lib/libDataModel.so: lib/libStore.so lib/libLogging.so

	cp -L DataModel/* include/
	g++ $(CPPFLAGS) -std=c++1y -g -fPIC -shared DataModel/*.C DataModel/*.cpp -I include -L lib -lStore  -lLogging  -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude) $(UserLib)

lib/libMyTools.so: lib/libStore.so include/Tool.h lib/libDataModel.so lib/libLogging.so

	cp UserTools/*.h include/
	cp UserTools/Factory/*.h include/
	g++ $(CPPFLAGS) -std=c++1y -g -fPIC -shared  UserTools/Factory/Factory.cpp -I include -L lib -lStore -lDataModel -lLogging -o lib/libMyTools.so $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude) $(UserLib)

lib/libServiceDiscovery.so: lib/libStore.so
	cp $(ToolDAQFrameworkPath)/src/ServiceDiscovery/ServiceDiscovery.h include/
	g++ $(CPPFLAGS) -shared -fPIC -I include $(ToolDAQFrameworkPath)/src/ServiceDiscovery/ServiceDiscovery.cpp -o lib/libServiceDiscovery.so -L lib/ -lStore  $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libLogging.so: lib/libStore.so
	cp $(ToolDAQFrameworkPath)/src/Logging/Logging.h include/
	g++ $(CPPFLAGS) -shared -fPIC -I include $(ToolDAQFrameworkPath)/src/Logging/Logging.cpp -o lib/libLogging.so -L lib/ -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

update:
	cd $(ToolDAQFrameworkPath)
	git pull
	cd ../..
	git pull

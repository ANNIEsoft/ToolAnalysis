
ZMQLib= -L ../zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I ../zeromq-4.0.7/include/ 

BoostLib= -L /usr/local/lib -lboost_date_time
BoostInclude= -I /usr/local/include/

DataModelInclude =
DataModelLib =

MyToolsInclude =
MyToolsLib =

all: lib/libMyTools.so lib/libToolChain.so lib/libStore.so include/Tool.h  lib/libDataModel.so RemoteControl NodeDaemon

	g++ src/main.cpp -o main -I include -L lib -lStore -lMyTools -lToolChain -lDataModel -lpthread $(DataModelInclude) $(MyToolsInclude) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)


lib/libStore.so:

	cp src/Store/Store.h include/
	g++ -shared -fPIC -I inlcude src/Store/Store.cpp -o lib/libStore.so


include/Tool.h:

	cp src/Tool/Tool.h include/


lib/libToolChain.so: lib/libStore.so include/Tool.h lib/libDataModel.so lib/libMyTools.so

	cp src/ToolChain/*.h include/
	g++ -fPIC -shared src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lMyTools -o lib/libToolChain.so $(DataModelInclude) $(MyToolsInclude) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)



clean: 
	rm -f include/*.h
	rm -f lib/*.so
	rm -f main
	rm -f RemoteControl
	rm -f NodeDaemon

lib/libDataModel.so: lib/libStore.so

	cp DataModel/*.h include/
	g++ -fPIC -shared DataModel/DataModel.cpp -I include -L lib -lStore -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)



lib/libMyTools.so: lib/libStore.so include/Tool.h lib/libDataModel.so

	cp UserTools/*.h include/
	cp UserTools/Factory/*.h include/
	g++  -shared -fPIC UserTools/Factory/Factory.cpp -I include -L lib -lStore -lDataModel -o lib/libMyTools.so $(MyToolsInclude)$(DataModelInclude) $(MyToolsLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

RemoteControl: lib/libStore.so lib/libToolChain.so lib/libDataModel.so

	g++ src/RemoteControl/RemoteControl.cpp -o RemoteControl -I include -L lib -lStore -lToolChain -lDataModel -lMyTools $(DataModelInclude)  $(MyToolsInclude)  $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

NodeDaemon: 
	g++ src/NodeDaemon/NodeDaemon.cpp -o NodeDaemon -I ./include/ -L ./lib/ -lToolChain -lStore 

ToolDAQPath=ToolDAQ

CPPFLAGS= -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable -Wl,--no-as-needed

CC=g++ -std=c++1y -g -fPIC -shared $(CPPFLAGS)

ZMQLib= -L $(ToolDAQPath)/zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I $(ToolDAQPath)/zeromq-4.0.7/include/ 

BoostLib= -L $(ToolDAQPath)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization  -lboost_iostreams 
BoostInclude= -I $(ToolDAQPath)/boost_1_66_0/install/include

RootInclude=  -I $(ToolDAQPath)/root/include
 
WCSimLib= -L ToolDAQ/WCSimLib -lWCSimRoot
WCSimInclude= -I ToolDAQ/WCSimLib/include

MrdTrackLib= -L ToolDAQ/MrdTrackLib/src -lFindMrdTracks
MrdTrackInclude= -I ToolDAQ/MrdTrackLib/include

RawViewerLib= -L UserTools/PlotWaveforms -lRawViewer

RootLib=   -L $(ToolDAQPath)/root/lib `root-config --glibs` -lMinuit -lGeom -lEve #-lGL  -lGLEW -lGLU

DataModelInclude = $(RootInclude)
DataModelLib = $(RootLib)

MyToolsInclude =  $(RootInclude) `python-config --cflags` $(MrdTrackInclude) $(WCSimInclude)
MyToolsLib = -lcurl $(RootLib) `python-config --libs` $(MrdTrackLib) $(WCSimLib) $(RawViewerLib)

all: lib/libStore.so lib/libLogging.so lib/libDataModel.so include/Tool.h lib/libMyTools.so lib/libServiceDiscovery.so lib/libToolChain.so Analyse

Analyse: src/main.cpp | lib/libMyTools.so lib/libStore.so lib/libLogging.so lib/libToolChain.so lib/libDataModel.so lib/libServiceDiscovery.so

	g++ -std=c++1y -g -fPIC $(CPPFLAGS) src/main.cpp -o Analyse -I include -L lib -lStore -lMyTools -lToolChain -lDataModel -lLogging -lServiceDiscovery -lpthread $(DataModelInclude) $(MyToolsInclude)  $(MyToolsLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)


lib/libStore.so: $(ToolDAQPath)/ToolDAQFramework/src/Store/*

	cp $(ToolDAQPath)/ToolDAQFramework/src/Store/*.h include/
	$(CC)  -I include $(ToolDAQPath)/ToolDAQFramework/src/Store/*.cpp -o lib/libStore.so $(BoostLib) $(BoostInclude)


include/Tool.h: $(ToolDAQPath)/ToolDAQFramework/src/Tool/Tool.h

	cp $(ToolDAQPath)/ToolDAQFramework/src/Tool/Tool.h include/


lib/libToolChain.so: $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/* | lib/libLogging.so lib/libStore.so lib/libMyTools.so lib/libServiceDiscovery.so lib/libLogging.so lib/libDataModel.so

	cp $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/*.h include/
	cp UserTools/Factory/*.h include/
	$(CC) $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lServiceDiscovery -lLogging -o lib/libToolChain.so $(DataModelInclude) $(ZMQLib) $(ZMQInclude) $(MyToolsInclude)  $(BoostLib) $(BoostInclude)


clean: 
	rm -f include/*.h
	rm -f lib/*.so
	rm -f Analyse

lib/libDataModel.so: DataModel/* lib/libLogging.so | lib/libStore.so

	cp DataModel/*.h include/
	$(CC) DataModel/*.C DataModel/*.cpp -I include -L lib -lStore  -lLogging  -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)

lib/libMyTools.so: UserTools/*/* UserTools/* | include/Tool.h lib/libDataModel.so lib/libLogging.so lib/libStore.so include/Tool.h lib/libToolChain.so

	cp UserTools/*/*.h include/
	cp UserTools/Factory/*.h include/
	$(CC)  UserTools/Factory/Factory.cpp -I include -L lib -lStore -lDataModel -lLogging -o lib/libMyTools.so $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

lib/libServiceDiscovery.so: $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/* | lib/libStore.so
	cp $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/ServiceDiscovery.h include/
	$(CC) -I include $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/ServiceDiscovery.cpp -o lib/libServiceDiscovery.so -L lib/ -lStore  $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libLogging.so: $(ToolDAQPath)/ToolDAQFramework/src/Logging/* | lib/libStore.so
	cp $(ToolDAQPath)/ToolDAQFramework/src/Logging/Logging.h include/
	$(CC) -I include $(ToolDAQPath)/ToolDAQFramework/src/Logging/Logging.cpp -o lib/libLogging.so -L lib/ -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

update:
	cd $(ToolDAQPath)/ToolDAQFramework; git pull
	cd $(ToolDAQPath)/zeromq-4.0.7; git pull
	cd $(ToolDAQPath)/MrdTrackLib; git checkout . ; git pull; make
	cd $(ToolDAQPath)/WCSimLib; git checkout . ; git pull; make
	git pull


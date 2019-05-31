ToolDAQPath=ToolDAQ
ZMQLib= -L $(ToolDAQPath)/zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I $(ToolDAQPath)/zeromq-4.0.7/include/ 

BoostLib= -L $(ToolDAQPath)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I $(ToolDAQPath)/boost_1_66_0/install/include

DataModelInclude = 
DataModelLib = 

MyToolsInclude =
MyToolsLib = 

all: lib/libStore.so lib/libLogging.so lib/libDataModel.so include/Tool.h lib/libMyTools.so lib/libServiceDiscovery.so lib/libToolChain.so main RemoteControl  NodeDaemon

main: src/main.cpp | lib/libMyTools.so lib/libStore.so lib/libLogging.so lib/libToolChain.so lib/libDataModel.so lib/libServiceDiscovery.so
	@echo -e "\n*************** Making " $@ "****************"
	g++ -g src/main.cpp -o main -I include -L lib -lStore -lMyTools -lToolChain -lDataModel -lLogging -lServiceDiscovery -lpthread $(DataModelInclude) $(DataModelib) $(MyToolsInclude)  $(MyToolsLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)


lib/libStore.so: $(ToolDAQPath)/ToolDAQFramework/src/Store/*
	cd $(ToolDAQPath)/ToolDAQFramework && make lib/libStore.so
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/Store/*.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/lib/libStore.so lib/
	#g++ -g -fPIC -shared  -I include $(ToolDAQPath)/ToolDAQFramework/src/Store/*.cpp -o lib/libStore.so $(BoostLib) $(BoostInclude)


include/Tool.h:  $(ToolDAQPath)/ToolDAQFramework/src/Tool/Tool.h
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/Tool/Tool.h include/
	cp UserTools/*.h include/
	cp UserTools/*/*.h include/
	cp DataModel/*.h include/


lib/libToolChain.so: $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/* | lib/libLogging.so lib/libStore.so lib/libMyTools.so lib/libServiceDiscovery.so lib/libLogging.so lib/libDataModel.so
	@echo -e "\n*************** Making " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/UserTools/Factory/*.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/*.h include/
	g++ -g -fPIC -shared $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lServiceDiscovery -lLogging -lMyTools -o lib/libToolChain.so $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(MyToolsInclude)  $(BoostLib) $(BoostInclude)


clean: 
	@echo -e "\n*************** Cleaning up ****************"
	rm -f include/*.h
	rm -f lib/*.so
	rm -f main
	rm -f RemoteControl
	rm -f NodeDaemon
	rm -f UserTools/*/*.o
	rm -f DataModel/*.o

lib/libDataModel.so: DataModel/* lib/libLogging.so lib/libStore.so $(patsubst DataModel/%.cpp, DataModel/%.o, $(wildcard DataModel/*.cpp))
	@echo -e "\n*************** Making " $@ "****************"
	cp DataModel/*.h include/
	#g++ -g -fPIC -shared DataModel/*.cpp -I include -L lib -lStore  -lLogging  -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)
	g++  -fPIC -shared DataModel/*.o -I include -L lib -lStore -lLogging -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

lib/libMyTools.so: UserTools/*/* UserTools/* include/Tool.h  lib/libLogging.so lib/libStore.so  $(patsubst UserTools/%.cpp, UserTools/%.o, $(wildcard UserTools/*/*.cpp)) |lib/libDataModel.so
	@echo -e "\n*************** Making " $@ "****************"
	cp UserTools/*/*.h include/
	cp UserTools/*.h include/
	#g++ -g -fPIC -shared  UserTools/Factory/Factory.cpp -I include -L lib -lStore -lDataModel -lLogging -o lib/libMyTools.so $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)
	g++   -shared -fPIC UserTools/*/*.o -I include -L lib -lStore -lDataModel -lLogging -o lib/libMyTools.so $(MyToolsInclude) $(DataModelInclude) $(MyToolsLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

RemoteControl:
	cd $(ToolDAQPath)/ToolDAQFramework/ && make RemoteControl
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/RemoteControl ./

NodeDaemon: 
	cd $(ToolDAQPath)/ToolDAQFramework/ && make NodeDaemon
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/NodeDaemon ./

lib/libServiceDiscovery.so: $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/* | lib/libStore.so
	cd $(ToolDAQPath)/ToolDAQFramework && make lib/libServiceDiscovery.so
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/ServiceDiscovery.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/lib/libServiceDiscovery.so lib/
	#g++ -shared -fPIC -I include $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/ServiceDiscovery.cpp -o lib/libServiceDiscovery.so -L lib/ -lStore  $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libLogging.so:  $(ToolDAQPath)/ToolDAQFramework/src/Logging/* | lib/libStore.so
	cd $(ToolDAQPath)/ToolDAQFramework && make lib/libLogging.so
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/Logging/Logging.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/lib/libLogging.so lib/
	#g++ -shared -fPIC -I include $(ToolDAQPath)/ToolDAQFramework/src/Logging/Logging.cpp -o lib/libLogging.so -L lib/ -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

update:
	@echo -e "\n*************** Updating ****************"
	cd $(ToolDAQPath)/ToolDAQFramework; git pull
	cd $(ToolDAQPath)/zeromq-4.0.7; git pull
	git pull


UserTools/%.o: UserTools/%.cpp lib/libStore.so include/Tool.h lib/libLogging.so lib/libDataModel.so
	@echo -e "\n*************** Making " $@ "****************"
	cp $(shell dirname $<)/*.h include
	-g++ -c -fPIC -o $@ $< -I include -L lib -lStore -lDataModel -lLogging $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

target: remove $(patsubst %.cpp, %.o, $(wildcard UserTools/$(TOOL)/*.cpp))

remove:
	echo "removing"
	-rm UserTools/$(TOOL)/*.o

DataModel/%.o: DataModel/%.cpp lib/libLogging.so lib/libStore.so
	@echo -e "\n*************** Making " $@ "****************"
	cp $(shell dirname $<)/*.h include
	-g++ -c -fPIC -o $@ $< -I include -L lib -lStore -lLogging  $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

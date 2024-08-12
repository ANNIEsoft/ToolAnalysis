ToolDAQPath=${PWD}/ToolDAQ

GIT_VERSION := "$(shell git describe --dirty --always)"

CPPFLAGS= -DVERSION=\"$(GIT_VERSION)\" -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable -Werror=return-type -Wl,--no-as-needed

CC=g++ -std=c++1y -g -fPIC -shared $(CPPFLAGS)
CCC= g++ -std=c++1y -g -fPIC  $(CPPFLAGS)


ZMQLib= -L $(ToolDAQPath)/zeromq-4.0.7/lib -lzmq 
ZMQInclude= -isystem$(ToolDAQPath)/zeromq-4.0.7/include/

BoostLib= -L $(ToolDAQPath)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization  -lboost_iostreams -lboost_system -lboost_filesystem -lboost_regex
BoostInclude= -isystem$(ToolDAQPath)/boost_1_66_0/install/include
 
WCSimLib= -L $(ToolDAQPath)/WCSimLib -lWCSimRoot
WCSimInclude= -I $(ToolDAQPath)/WCSimLib/include

GenieIncludeDir := $(shell genie-config --topsrcdir)
GenieInclude= -isystem$(GenieIncludeDir)/Framework -isystem$(GenieIncludeDir) `gsl-config --cflags` -isystem$(GENIE_REWEIGHT)/src
GenieLibs= `genie-config --libs` -lxml2 `gsl-config --libs` -L$(GENIE_REWEIGHT)/lib -lGRwClc -lGRwFwk -lGRwIO
PythiaLibs= -L $(ToolDAQPath)/Pythia6Support/v6_424/lib -lPythia6
Log4CppLibs= -L $(ToolDAQPath)/log4cpp/lib -llog4cpp
Log4CppInclude= -isystem$(ToolDAQPath)/log4cpp/include

RATEventLib= -L $(ToolDAQPath)/RATEventLib/lib -lRATEvent
RATEventInclude= -I $(ToolDAQPath)/RATEventLib/include

MrdTrackLib= -L $(ToolDAQPath)/MrdTrackLib/src -lFindMrdTracks
MrdTrackInclude= -I $(ToolDAQPath)/MrdTrackLib/include

CLHEPInc= -isystem${CLHEP_DIR}/include
CLHEPLib= -L ${CLHEP_DIR}/lib -lCLHEP-2.4.0.2

RootLib=  -L `root-config --libdir --glibs` -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -lm -ldl -rdynamic -m64 -lGui -lGenVector -lMinuit -lGeom -lEG -lEGPythia6 -lEve #-lGL -lGLEW -lGLU
RootInclude= -isystem`root-config --incdir`

ALL_INCLUDE_DIRS=$(ZMQInclude) $(BoostInclude) $(WCSimInclude) $(GenieInclude) $(Log4CppInclude) $(RATEventInclude) $(MRDTrackInclude) -I../include
# we need to build a ROOT dictionary for the DataModel for python integration to work.
# we're using `-isystem` instead of `-I` for dependencies here to suppress the myriad of warnings
# coming from code we cannot fix, but rootcling ignores `-isystem`, so fails to find the headers.
# we have two solutions:
# 1. replace all -isystem instances with -I via:
ALL_INCLUDE_DIRS2=$(subst -isystem,-I,$(ALL_INCLUDE_DIRS))
# and pass this to rootcling. or...
# 2. add those paths to ROOT_INCLUDE_PATH, which rootcling will search.

RawViewerLib= -L UserTools/PlotWaveforms -lRawViewer


DataModelInclude = $(RootInclude)
DataModelLib = $(RootLib)
HEADERS=$(shell cd DataModel && ls *.h)

MyToolsInclude =  $(RootInclude) $(MrdTrackInclude) $(WCSimInclude) $(RATEventInclude) $(CLHEPInc) $(Log4CppInclude) $(GenieInclude)
MyToolsInclude += `python3-config --cflags` -Wno-sign-compare
MyToolsLib = -lcurl $(RootLib) $(MrdTrackLib) $(WCSimLib) $(RATEventLib) $(RawViewerLib) $(CLHEPLib) $(Log4CppLibs) $(GenieLibs) $(PythiaLibs)
MyToolsLib += `python3-config --ldflags --embed`


all: lib/libStore.so lib/libLogging.so lib/libDataModel.so include/Tool.h lib/libMyTools.so lib/libServiceDiscovery.so lib/libToolChain.so Analyse

Analyse: src/main.cpp | lib/libMyTools.so lib/libStore.so lib/libLogging.so lib/libToolChain.so lib/libDataModel.so lib/libServiceDiscovery.so
	@echo -e "\n*************** Making " $@ "****************"
	g++ -std=c++1y -g -fPIC $(CPPFLAGS) src/main.cpp -o Analyse -I include -L lib -lStore -lMyTools -lToolChain -lDataModel -lLogging -lServiceDiscovery -lpthread $(DataModelInclude) $(DataModelLib) $(MyToolsInclude)  $(MyToolsLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)


lib/libStore.so: $(ToolDAQPath)/ToolDAQFramework/src/Store/*
	cd $(ToolDAQPath)/ToolDAQFramework && make lib/libStore.so
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/Store/*.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/lib/libStore.so lib/        
	#$(CC)  -I include $(ToolDAQPath)/ToolDAQFramework/src/Store/*.cpp -o lib/libStore.so $(BoostLib) $(BoostInclude)


include/Tool.h: $(ToolDAQPath)/ToolDAQFramework/src/Tool/Tool.h
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/Tool/Tool.h include/
	cp UserTools/*.h include/
	cp UserTools/*/*.h include/
	cp DataModel/*.h include/


lib/libToolChain.so: $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/* | lib/libLogging.so lib/libStore.so lib/libServiceDiscovery.so lib/libLogging.so lib/libDataModel.so
	@echo -e "/n*************** Making " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/UserTools/Factory/*.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/*.h include/
	$(CC) $(ToolDAQPath)/ToolDAQFramework/src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lServiceDiscovery -lLogging -o lib/libToolChain.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(MyToolsInclude)  $(BoostLib) $(BoostInclude)


clean: 
	@echo -e "\n*************** Cleaning up ****************"
	rm -f include/*.h
	rm -f lib/*.so
	rm -f Analyse
	find UserTools/* -type f -name '*.o' -follow -writable -delete
	rm -f DataModel/*.o
	rm -f DataModel/DataModel_Linkdef.hh
	rm -f DataModel/DataModel_RootDict*
	rm -f lib/*.pcm
	rm -f DataModel/libDataModel.rootmap

lib/libDataModel.so: DataModel/* lib/libLogging.so lib/libStore.so $(patsubst DataModel/%.cpp, DataModel/%.o, $(wildcard DataModel/*.cpp)) DataModel/DataModel_RootDict.cpp
	@echo -e "\n*************** Making " $@ "****************"
	cp -f DataModel/*.h include/
	$(CC) DataModel/*.o -I include -L lib -lStore  -lLogging  -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)

DataModel/DataModel_Linkdef.hh: linkdefpreamble.txt linkdefincludeheader.txt linkdefpostamble.txt $(wildcard DataModel/*.h)
	@cat linkdefpreamble.txt > $@
	@for file in $(HEADERS); do \
	  echo -n $$(cat linkdefincludeheader.txt) >> $@; \
	  echo " \"$${file}\";" >> $@; \
	done
	@cat linkdefpostamble.txt >> $@

DataModel/DataModel_RootDict.cpp: DataModel/DataModel_Linkdef.hh | include/Tool.h lib/libStore.so
	# the dictionary sourcefiles need to be built within the directory they're to reside in
	# otherwise the paths encoded into the dictionary don't match those when the object is built.
	cd DataModel && \
	rootcling -f DataModel_RootDict.cpp -c -p -rmf libDataModel.rootmap $(RMLLIBS) $(ALL_INCLUDE_DIRS2) $(HEADERS) DataModel_Linkdef.hh
	#rootcint -f $@ -c -p -fPIC `root-config --cflags` -I ./ $(HEADERS) $^
	@rm -f lib/libDataModel.rootmap lib/DataModel_RootDict_rdict.pcm
	cp -f DataModel/libDataModel.rootmap lib/
	cp -f DataModel/DataModel_RootDict_rdict.pcm lib/

lib/libMyTools.so: UserTools/*/* UserTools/* include/Tool.h lib/libLogging.so lib/libStore.so $(patsubst UserTools/%.cpp, UserTools/%.o, $(wildcard UserTools/*/*.cpp)) |lib/libDataModel.so lib/libToolChain.so lib/libRawViewer.so 
	@echo -e "\n*************** Making " $@ "****************"
	cp -f UserTools/*/*.h include/
	cp -f UserTools/*.h include/
	#$(CC)  UserTools/Factory/Factory.cpp -I include -L lib -lStore -lDataModel -lLogging -o lib/libMyTools.so $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)
	$(CC) UserTools/*/*.o -I include -L lib -lStore -lDataModel -lLogging -o lib/libMyTools.so $(MyToolsInclude) $(DataModelInclude) $(MyToolsLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

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
	#$(CC) -I include $(ToolDAQPath)/ToolDAQFramework/src/ServiceDiscovery/ServiceDiscovery.cpp -o lib/libServiceDiscovery.so -L lib/ -lStore  $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libLogging.so: $(ToolDAQPath)/ToolDAQFramework/src/Logging/* | lib/libStore.so
	cd $(ToolDAQPath)/ToolDAQFramework && make lib/libLogging.so
	@echo -e "\n*************** Copying " $@ "****************"
	cp $(ToolDAQPath)/ToolDAQFramework/src/Logging/Logging.h include/
	cp $(ToolDAQPath)/ToolDAQFramework/lib/libLogging.so lib/
	#$(CC) -I include $(ToolDAQPath)/ToolDAQFramework/src/Logging/Logging.cpp -o lib/libLogging.so -L lib/ -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libRawViewer.so: UserTools/PlotWaveforms/RawViewer.h UserTools/PlotWaveforms/RawViewer.cc UserTools/PlotWaveforms/viewer_linkdef.hh UserTools/recoANNIE/Constants.h DataModel/ANNIEconstants.h UserTools/recoANNIE/RawReader.h UserTools/recoANNIE/RawReader.cc UserTools/recoANNIE/RawReadout.h UserTools/recoANNIE/RawReadout.cc UserTools/recoANNIE/RawChannel.h UserTools/recoANNIE/RawChannel.cc UserTools/recoANNIE/RawCard.h UserTools/recoANNIE/RawCard.cc UserTools/recoANNIE/RawTrigData.h UserTools/recoANNIE/RawTrigData.cc
	@echo -e "\n*************** Making " $@ "****************"
	cd UserTools/PlotWaveforms && . ./setup_builder.sh && make clean && make
	cp UserTools/PlotWaveforms/libRawViewer.so UserTools/PlotWaveforms/dict_rdict.pcm lib/

update:
	@echo -e "\n*************** Updating ****************"
	cd $(ToolDAQPath)/ToolDAQFramework; git pull
	cd $(ToolDAQPath)/zeromq-4.0.7; git pull
	cd $(ToolDAQPath)/MrdTrackLib; git checkout . ; git pull; make -f Makefile.FNAL
	cd $(ToolDAQPath)/WCSimLib; git checkout . ; git pull; make
	cd $(ToolDAQPath)/RATEventLib; git checkout . ; git pull; make
	git pull

UserTools/%.o: UserTools/%.cpp lib/libStore.so include/Tool.h lib/libLogging.so lib/libDataModel.so lib/libToolChain.so
	@echo -e "\n*************** Making " $@ "****************"
	cp $(shell dirname $<)/*.h include
	-$(CCC) -c -o $@ $< -I include -L lib -lStore -lDataModel -lLogging $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

target: remove $(patsubst %.cpp, %.o, $(wildcard UserTools/$(TOOL)/*.cpp))

remove:
	echo "removing"
	-rm UserTools/$(TOOL)/*.o

DataModel/%.o: DataModel/%.cpp lib/libLogging.so lib/libStore.so
	@echo -e "\n*************** Making " $@ "****************"
	cp $(shell dirname $<)/*.h include
	-$(CCC) -c -o $@ $< -I include -L lib -lStore -lLogging  $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

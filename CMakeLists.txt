cmake_minimum_required (VERSION 2.6)
project (ToolDAQ)

include_directories ("${PROJECT_SOURCE_DIR}/include/" "${PROJECT_SOURCE_DIR}/ToolDAQ/boost_1_66_0/install/include/" "${PROJECT_SOURCE_DIR}/ToolDAQ/zeromq-4.0.7/include/" "${PROJECT_SOURCE_DIR}/ToolDAQ/root/include" "/usr/include/python2.7" "${PROJECT_SOURCE_DIR}/ToolDAQ/WCSimLib/include" "${PROJECT_SOURCE_DIR}/ToolDAQ/MrdTrackLib/include")
link_directories("${PROJECT_SOURCE_DIR}/lib" "${PROJECT_SOURCE_DIR}/ToolDAQ/boost_1_66_0/install/lib" "${PROJECT_SOURCE_DIR}/ToolDAQ/zeromq-4.0.7/lib" "${PROJECT_SOURCE_DIR}/ToolDAQ/root/lib" "${PROJECT_SOURCE_DIR}/ToolDAQ/MrdTrackLib/src" "${PROJECT_SOURCE_DIR}/ToolDAQ/WCSimLib")


MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${PROJECT_SOURCE_DIR}/UserTools/*.h)
   #FILE(COPY ${new_list} DESTINATION ${PROJECT_BINARY_DIR}/include)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

HEADER_DIRECTORIES(header_list)
include_directories(${header_list})
include_directories("${PROJECT_SOURCE_DIR}/DataModel/")


set(CMAKE_CXX_FLAGS "-std=c++1y -g -fPIC -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

file(GLOB_RECURSE STORESRC RELATIVE ${CMAKE_SOURCE_DIR} "ToolDAQ/ToolDAQFramework/src/Store/*.cpp")
add_library(Store SHARED ${STORESRC})

file(GLOB_RECURSE LOGGINGSRC RELATIVE ${CMAKE_SOURCE_DIR} "ToolDAQ/ToolDAQFramework/src/Logging/*.cpp")
add_library(Logging SHARED ${LOGGINGSRC})

file(GLOB_RECURSE SERVICEDISCOVERYSRC RELATIVE ${CMAKE_SOURCE_DIR} "ToolDAQ/ToolDAQFramework/src/ServiceDiscovery/*.cpp")
add_library(ServiceDiscovery SHARED ${SERVICEDISCOVERYSRC})

file(GLOB_RECURSE DATAMODELSRC RELATIVE ${CMAKE_SOURCE_DIR} "DataModel/*.cpp")
add_library(DataModel SHARED ${DATAMODELSRC})

file(GLOB_RECURSE MYTOOLSSRC RELATIVE ${CMAKE_SOURCE_DIR} "UserTools/*.cpp")
add_library(MyTools SHARED ${MYTOOLSSRC})

file(GLOB_RECURSE TOOLCHAINSRC RELATIVE ${CMAKE_SOURCE_DIR} "ToolDAQ/ToolDAQFramework/src/ToolChain/*.cpp")
add_library(ToolChain SHARED ${TOOLCHAINSRC})

add_executable (Analyse ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries (Analyse Store Logging ToolChain ServiceDiscovery MyTools DataModel zmq boost_date_time boost_serialization boost_iostreams boost_system GenVector Core Cint RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore Thread pthread m dl util python2.7 WCSimRoot FindMrdTracks curl)

#add_executable ( NodeDaemon src/NodeDaemon/NodeDaemon.cpp)
#target_link_libraries (NodeDaemon Store ServiceDiscovery zmq boost_date_time boost_serialization boost_iostreams)

#add_executable ( RemoteControl src/RemoteControl/RemoteControl.cpp)
#target_link_libraries (RemoteControl Store ServiceDiscovery zmq boost_date_time boost_serialization boost_iostreams)

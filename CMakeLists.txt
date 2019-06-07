cmake_minimum_required (VERSION 2.6)

project (ToolDAQApplicaiton)

set(TOOLDAQ_PATH "${PROJECT_SOURCE_DIR}/ToolDAQ")

#include(${TOOLDAQ_PATH}/ToolDAQFramework/CMakeLists.include)

set(ZMQ_INC "${TOOLDAQ_PATH}/zeromq-4.0.7/include/")
set(ZMQ_LIB_PATH "${TOOLDAQ_PATH}/zeromq-4.0.7/lib")
set(ZMQ_LIBS zmq)

set(BOOST_INC "${TOOLDAQ_PATH}/boost_1_66_0/install/include/")
set(BOOST_LIB_PATH "${TOOLDAQ_PATH}/boost_1_66_0/install/lib")
set(BOOST_LIBS boost_date_time boost_serialization boost_iostreams)

set(DATAMODEL_INC "")
set(DATAMODEL_LIB_PATH "")
set(DATAMODEL_LIBS "")

set(MYTOOLS_INC "")
set(MYTOOLS_LIB_PATH "")
set(MYTOOLS_LIBS "")

#add_subdirectory(${TOOLDAQ_PATH}/ToolDAQFramework/ ./ToolDAQ/ToolDAQFramework/)

if(NOT(${PROJECT_SOURCE_DIR} STREQUAL  ${PROJECT_BINARY_DIR}))
message("Not Building in source directory: Copying files")
FILE(COPY ${PROJECT_SOURCE_DIR}/configfiles DESTINATION ${PROJECT_BINARY_DIR}/)
FILE(COPY ${PROJECT_SOURCE_DIR}/UserTools DESTINATION ${PROJECT_BINARY_DIR}/)
FILE(COPY ${PROJECT_SOURCE_DIR}/DataModel DESTINATION ${PROJECT_BINARY_DIR}/)
FILE(COPY ${PROJECT_SOURCE_DIR}/Setup.sh DESTINATION ${PROJECT_BINARY_DIR}/)
endif()

include_directories(${PROJECT_BINARY_DIR}/Datamodel ${BOOST_INC} ${ZMQ_INC} ${DATAMODEL_INC} ${MYTOOLS_INC} ${TOOLDAQ_PATH}/ToolDAQFramework/include)
link_directories("${PROJECT_BINARY_DIR}/lib" ${BOOST_LIB_PATH} ${ZMQ_LIB_PATH} ${DATAMODEL_LIB_PATH} ${MYTOOLS_LIB_PATH} ${TOOLDAQ_PATH}/ToolDAQFramework/lib)

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${PROJECT_BINARY_DIR}/UserTools/*.h)
    FILE(COPY ${new_list} DESTINATION ${PROJECT_BINARY_DIR}/include)
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

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

file(GLOB_RECURSE DATAMODEL_SRC RELATIVE ${CMAKE_BINARY_DIR} "DataModel/*.cpp")
add_library(DataModel SHARED ${DATAMODEL_SRC})

file(GLOB_RECURSE MYTOOLS_SRC RELATIVE ${CMAKE_BINARY_DIR} "UserTools/*.cpp")
add_library(MyTools SHARED ${MYTOOLS_SRC})

add_executable (main ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries (main Store Logging ToolChain ServiceDiscovery MyTools DataModel ${ZMQ_LIBS} ${BOOST_LIBS} ${DATAMODEL_LIBS} ${MYTOOLS_LIBS})

add_executable ( NodeDaemon ${TOOLDAQ_PATH}/ToolDAQFramework/src/NodeDaemon/NodeDaemon.cpp)
target_link_libraries (NodeDaemon Store ServiceDiscovery ${ZMQ_LIBS} ${BOOST_LIBS})

add_executable ( RemoteControl ${TOOLDAQ_PATH}/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp)
target_link_libraries (RemoteControl Store ServiceDiscovery ${ZMQ_LIBS} ${BOOST_LIBS})

#!/bin/bash

#Application path location of applicaiton

ToolDAQapp=`pwd`

source ${ToolDAQapp}/ToolDAQ/root/bin/thisroot.sh

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib:${ToolDAQapp}/ToolDAQ/root/lib:${ToolDAQapp}/ToolDAQ/WCSimLib/:${ToolDAQapp}/ToolDAQ/RATEventLib/lib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/RATEventLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:$ROOT_INCLUDE_PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

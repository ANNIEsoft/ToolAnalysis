#!/bin/bash

#Application path location of applicaiton

ToolDAQapp=`pwd`

source ${ToolDAQapp}/ToolDAQ/root/bin/thisroot.sh

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib:${ToolDAQapp}/ToolDAQ/root/lib:${ToolDAQapp}/ToolDAQ/WCSimLib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:${TOOLANALYSIS_ROOT}/UserTools/PlotWaveforms:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${TOOLANALYSIS_ROOT}/ToolDAQ/WCSimLib/include/:$ROOT_INCLUDE_PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done


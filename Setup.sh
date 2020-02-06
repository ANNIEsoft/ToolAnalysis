#!/bin/bash

#Application path location of applicaiton

ToolDAQapp=`pwd`

source ${ToolDAQapp}/ToolDAQ/root-6.06.08/install/bin/thisroot.sh


export LD_LIBRARY_PATH=`pwd`/lib:${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib:${ToolDAQapp}/ToolDAQ/root/lib:${ToolDAQapp}/ToolDAQ/WCSimLib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:${ToolDAQapp}/ToolDAQ/RATEventLib/lib/:${ToolDAQapp}/UserTools/PlotWaveforms:${ToolDAQapp}/ToolDAQ/log4cpp/lib:${ToolDAQapp}/ToolDAQ/Pythia6Support/v6_424/lib:${ToolDAQapp}/ToolDAQ/Generator-R-3_00_04/lib:$LD_LIBRARY_PATH

export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:${ToolDAQapp}/ToolDAQ/RATEventLib/include/:$ROOT_INCLUDE_PATH

export PYTHIA6_DIR=/ToolAnalysis/ToolDAQ/Pythia6Support/v6_424/
export GENIE=/ToolAnalysis/ToolDAQ/Generator-R-3_00_04/

export PATH=$GENIE/bin:$PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

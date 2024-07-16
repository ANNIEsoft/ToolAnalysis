#!/bin/bash

#Application path location of applicaiton

ToolDAQapp=`pwd`

export LIBGL_ALWAYS_INDIRECT=1

source ${ToolDAQapp}/ToolDAQ/root-6.24.06/install/bin/thisroot.sh

export CLHEP_DIR=${ToolDAQapp}/ToolDAQ/2.4.0.2/CLHEP_install

export LD_LIBRARY_PATH=`pwd`/lib:${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib:${ToolDAQapp}/ToolDAQ/root/lib:${ToolDAQapp}/ToolDAQ/WCSimLib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:${ToolDAQapp}/ToolDAQ/RATEventLib/lib/:${ToolDAQapp}/UserTools/PlotWaveforms:${ToolDAQapp}/ToolDAQ/log4cpp/lib:${ToolDAQapp}/ToolDAQ/Pythia6Support/v6_424/lib:${CLHEP_DIR}/lib:${ToolDAQapp}/ToolDAQ/LHAPDF-6.3.0/install/lib:${ToolDAQapp}/ToolDAQ/GENIE-v3-master/lib:${ToolDAQapp}/ToolDAQ/Reweight-3_00_04_ub3/lib:$LD_LIBRARY_PATH

export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/include:${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:${ToolDAQapp}/ToolDAQ/RATEventLib/include/:${ToolDAQapp}/UserTools/PlotWaveforms:$ROOT_INCLUDE_PATH

export PYTHIA6_DIR=${ToolDAQapp}/ToolDAQ/Pythia6Support/v6_424/
export LHAPATH=${ToolDAQapp}/ToolDAQ/LHAPDF-6.3.0/install/share/LHAPDF/
export GENIE=${ToolDAQapp}/ToolDAQ/GENIE-v3-master
export GENIE_REWEIGHT=${ToolDAQapp}/ToolDAQ/Reweight-3_00_04_ub3/

export PATH=${ToolDAQapp}/ToolDAQ/LHAPDF-6.3.0/install/bin:$GENIE/bin:$GENIE_REWEIGHT/bin:$PATH

export PATH=/ToolAnalysis/ToolDAQ/fsplit:$PATH
export TF_CPP_MIN_LOG_LEVEL=2

export FW_SEARCH_PATH=${ToolDAQapp}/UserTools/ReweightFlux

for folder in `ls -d ${PWD}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

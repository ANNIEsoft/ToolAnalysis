#!/bin/bash

if [ ! -d ToolDAQ ];
then
    echo "ToolDAQ symlink does not exist.  Making symlink"
    ln -s /ToolAnalysis/ToolDAQ ToolDAQ
fi

#Check that ToolDAQ symlink exists
DIRTYPE=$(ls -la ToolDAQ | head -c 1)
if [ "$DIRTYPE" != "l" ];
then
    echo "ToolDAQ is not a symlink.  Delete ./ToolDAQ directory if you're on the cluster and re-setup to have symlink made."
fi

#Application path location of applicaiton

ToolDAQapp=`pwd`

# for newer containers
export LIBGL_ALWAYS_INDIRECT=1

source ${ToolDAQapp}/ToolDAQ/root-6.08.06/install/bin/thisroot.sh


export LD_LIBRARY_PATH=`pwd`/lib:${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib:${ToolDAQapp}/ToolDAQ/root/lib:${ToolDAQapp}/ToolDAQ/WCSimLib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:${ToolDAQapp}/ToolDAQ/RATEventLib/lib/:${ToolDAQapp}/UserTools/PlotWaveforms:${ToolDAQapp}/ToolDAQ/log4cpp/lib:${ToolDAQapp}/ToolDAQ/Pythia6Support/v6_424/lib:${ToolDAQapp}/ToolDAQ/Generator-R-3_00_04/lib:$LD_LIBRARY_PATH

export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:${ToolDAQapp}/ToolDAQ/RATEventLib/include/:${ToolDAQapp}/UserTools/PlotWaveforms:$ROOT_INCLUDE_PATH

export PYTHIA6_DIR=${ToolDAQapp}/ToolDAQ/Pythia6Support/v6_424/
export GENIE=${ToolDAQapp}/ToolDAQ/Generator-R-3_00_04/

export PATH=$GENIE/bin:$PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done


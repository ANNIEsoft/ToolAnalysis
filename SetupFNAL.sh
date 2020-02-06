#!/bin/bash

# Saves the directory where this script is located to the ToolDAQapp
# variable. This method isn't foolproof. See
# https://stackoverflow.com/a/246128/4081973 if you need something more robust
# for edge cases (e.g., you're calling the script using symlinks).
ToolDAQapp="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source /cvmfs/annie.opensciencegrid.org/setup_annie.sh --local
#setup -f Linux64bit+2.6-2.12 -q e17:prof boost v1_66_0a  # need a newer boost for reading MRD data files
#setup -f Linux64bit+2.6-2.12 -q e15:prof root v6_12_06a  #
export PRODUCTS=${PRODUCTS}:/grid/fermiapp/products/larsoft
setup -f Linux64bit+2.6-2.12 git v2_14_1  # newer git version, available from larsoft. Nice features like git diff --word-diff

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/ToolDAQFramework/lib/:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/WCSimLib:${ToolDAQapp}/ToolDAQ/RATEventLib/lib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/RATEventLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:$ROOT_INCLUDE_PATH
## we need to use local boost install to get the correct version without C++ ABI issues
export BOOST_LIB=${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib
export BOOST_INC=${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/include

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/ToolDAQFramework/lib/:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/WCSimLib:${ToolDAQapp}/ToolDAQ/RATEventLib/lib/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:${ToolDAQapp}/ToolDAQ/boost_1_66_0/install/lib:${ToolDAQapp}/UserTools/PlotWaveforms:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/RATEventLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:$ROOT_INCLUDE_PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

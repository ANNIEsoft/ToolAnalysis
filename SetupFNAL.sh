#!/bin/bash

# Saves the directory where this script is located to the ToolDAQapp
# variable. This method isn't foolproof. See
# https://stackoverflow.com/a/246128/4081973 if you need something more robust
# for edge cases (e.g., you're calling the script using symlinks).
ToolDAQapp="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source /cvmfs/annie.opensciencegrid.org/setup_annie.sh --local
export PRODUCTS=${PRODUCTS}:/grid/fermiapp/products/larsoft
setup -f Linux64bit+2.6-2.12 git v2_14_1  # newer git version, available from larsoft. Nice features like git diff --word-diff

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/ToolDAQFramework/lib/:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/WCSimLib:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:${ToolDAQapp}/UserTools/PlotWaveforms:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:$ROOT_INCLUDE_PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

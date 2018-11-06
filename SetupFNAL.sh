#!/bin/bash

# Saves the directory where this script is located to the TOOLANALYSIS_ROOT
# variable. This method isn't foolproof. See
# https://stackoverflow.com/a/246128/4081973 if you need something more robust
# for edge cases (e.g., you're calling the script using symlinks).
TOOLANALYSIS_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source /cvmfs/annie.opensciencegrid.org/setup_annie.sh --local

export LD_LIBRARY_PATH=${TOOLANALYSIS_ROOT}/lib:${TOOLANALYSIS_ROOT}/ToolDAQ/ToolDAQFramework/lib/:${TOOLANALYSIS_ROOT}/ToolDAQ/zeromq-4.0.7/lib:${TOOLANALYSIS_ROOT}/ToolDAQ/WCSimLib:${TOOLANALYSIS_ROOT}/ToolDAQ/MrdTrackLib/src:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${TOOLANALYSIS_ROOT}/ToolDAQ/WCSimLib/include/:${TOOLANALYSIS_ROOT}/ToolDAQ/MrdTrackLib/include:$ROOT_INCLUDE_PATH

for folder in `ls -d ${TOOLANALYSIS_ROOT}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

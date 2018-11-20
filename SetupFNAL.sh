#!/bin/bash

# Saves the directory where this script is located to the ToolDAQapp
# variable. This method isn't foolproof. See
# https://stackoverflow.com/a/246128/4081973 if you need something more robust
# for edge cases (e.g., you're calling the script using symlinks).
ToolDAQapp="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source /cvmfs/annie.opensciencegrid.org/setup_annie.sh --local

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/ToolDAQFramework/lib/:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/WCSimLib:${ToolDAQapp}/ToolDAQ/MrdTrackLib/src:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${ToolDAQapp}/ToolDAQ/WCSimLib/include/:${ToolDAQapp}/ToolDAQ/MrdTrackLib/include:$ROOT_INCLUDE_PATH

for folder in `ls -d ${ToolDAQapp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

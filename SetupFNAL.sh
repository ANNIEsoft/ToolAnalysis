#!/bin/bash

#Application path location of applicaiton

ToolAnalysisApp=`pwd`

source /grid/fermiapp/products/common/etc/setup
export PRODUCTS=${PRODUCTS}:/grid/fermiapp/products/larsoft
setup -f Linux64bit+2.6-2.12 -q debug:e10:nu root v6_06_08
source /grid/fermiapp/products/larsoft/root/v6_06_08/Linux64bit+2.6-2.12-e10-nu-debug/bin/thisroot.sh
export ROOT_PATH=/grid/fermiapp/products/larsoft/root/v6_06_08/Linux64bit+2.6-2.12-e10-nu-debug/cmake
setup -f Linux64bit+2.6-2.12 python v2_7_11

setup boost v1_57_0a -q debug:e9

export LD_LIBRARY_PATH=${ToolAnalysisApp}/lib:${ToolAnalysisApp}/ToolDAQ/ToolDAQFramework/lib/:${ToolAnalysisApp}/ToolDAQ/zeromq-4.0.7/lib:${ToolAnalysisApp}/ToolDAQ/LibWCSim/:${ToolAnalysisApp}/ToolDAQ/LibFindMrdTracks/src:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=${ToolAnalysisApp}/ToolDAQ/LibWCSim/include/:$ROOT_INCLUDE_PATH

for folder in `ls -d ${ToolAnalysisApp}/UserTools/*/ `
do
    export PYTHONPATH=$folder:${PYTHONPATH}
done

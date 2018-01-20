#!/bin/bash

#Application path location of applicaiton

export ToolAnalysisApp="/annie/app/users/moflaher/ToolAnalysis/ToolAnalysis"

setup boost v1_57_0a -q debug:e9
export LD_LIBRARY_PATH=${ToolAnalysisApp}/lib:${ToolAnalysisApp}/ToolDAQ/ToolDAQFramework/lib/:${ToolAnalysisApp}/ToolDAQ/zeromq-4.0.7/lib:$LD_LIBRARY_PATH
export PYTHONPATH=${ToolAnalysisApp}/UserTools/:${PYTHONPATH}

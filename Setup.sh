#!/bin/bash

#Application path location of applicaiton

ToolDAQapp=`pwd`

export LD_LIBRARY_PATH=${ToolDAQapp}/lib:${ToolDAQapp}/ToolDAQ/zeromq-4.0.7/lib:${ToolDAQapp}/ToolDAQ/boost_1_60_0/install/lib:$LD_LIBRARY_PATH
export PYTHONPATH=${ToolDAQapp}/UserTools/:${PYTHONPATH}
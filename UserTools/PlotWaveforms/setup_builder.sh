#!/bin/bash
# Get the path to the directory that holds this setup script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

ROOT_INCLUDE_PATH=$ROOT_INCLUDE_PATH:$SCRIPT_DIR/../../include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${SCRIPT_DIR}

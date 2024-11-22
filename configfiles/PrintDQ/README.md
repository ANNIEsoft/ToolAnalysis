# PrintDQ

***********************
# Description
***********************

The `PrintDQ` toolchain runs the clustering tools (MRD + PMT) over the ProcessedData files created by the event building toolchain to output run quality statistics. For more information, check out the README for the tool: https://github.com/ANNIEsoft/ToolAnalysis/tree/Application/UserTools/PrintDQ. This tool can be used for assessing the quality of Processed runs to help identify issues with the detector.

************************
# Usage
************************

- Populate the `my_inputs.txt` file with all part files from a Processed runs. Running the script `sh create_my_inputs.sh <run_number>` will automatically populate the input file with all Processed Data part files for that run.
- Run the toolchain via: `./Analyse ./configfiles/PrintDQ/ToolChain`
- Run statistics will be outputted via `std::out` once the toolchain completes.


************************
# Additional information
************************

- The current version of the `PrintDQ` tool is intended to be run over 1 run at a time. As the clustering tools may take some time to compile, the processing time of this toolchain may take several minutes (for a ~100 part file run) to ~1 hour (~thousands of part files) depending on how many part files exist.

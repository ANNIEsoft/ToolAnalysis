# Configure files

***********************
#Description
**********************

Configure files are simple text files for passing variables to the Tools.

Text files are read by the Store class (src/Store) and automatically asigned to an internal map for the relavent Tool to use.


************************
#Useage
************************

Any line starting with a "#" will be ignored by the Store, as will blank lines.

Variables should be stored one per line as follows:


Name Value #Comments 


Note: Only one value is permitted per name and they are stored in a string stream and templated cast back to the type given.


# Notes on this configuration for ToolAnalysis

The PhaseIIDoE configuration utilizes tools to read from the reduced PhaseIIDoE
Files and run the Extended Vertex Fitter in ToolAnalysis.  Note that, due to
the files' simplicity (the files are essentially ntuples), the LoadWCSim and
DigitBuilder tools are combined into the DigitBuilderDoE tool.

Also note that, as seems to have been done in the scripts used to read the
DoE proposal data, a Muon is not required to stop in the MRD, but only
has to have passed into it for some portion of travel.

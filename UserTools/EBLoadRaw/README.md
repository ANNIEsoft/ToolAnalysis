# EBLoadRaw

EBLoadRaw tool is a part of Event Building version 2 tool chain.
For reference slides, see:
https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5633
It basically follows the logic of LoadRawData, but reorganized to a different format and abort the BuileType option. If you need to load PMT and CTC data flow, just choose LoadCTC = true and LoadPMT = true, and use false for others. 

## Data

need a list of raw part file as input, set the loaded entry to CStore.
Check Load*Data functions for each data flow.

## Configuration

**InputFile** is the name of txt file which has the raw part file list in it.
**ReadTriggerOverlap** will control load the overlap file or not. It's necessary for beam runs, not really necessary for source runs because lost a few events is not unacceptable for source run.

**LoadCTC, LoadPMT, LoadMRD, LoadLAPPD** These tells the tool to load the data flow or not. Usually true for all of them.



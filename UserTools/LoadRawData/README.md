# LoadRawData
LoadRawData

This tool is used to load BoostStores from an ANNIE Raw Data file, 
access data, and save the data to the CStore for tools to use downstream.


## Data

Describe any data formats LoadRawData creates, destroys, changes, or analyzes. E.G.

Entries placed into the CStore:
FileProcessingComplete (bool)
NewRawDataEntryAccessed (bool)
NewRawDataFileAccessed (bool)
PauseTankDecoding (bool)
PauseMRDDecoding (bool)
PauseCTCDecoding (bool)

Values are updated each loop and are used by tools downstream.  Can be used for 
error handling logic.

Entries accessed from the CStore:


## Configuration

Describe any configuration variables for LoadRawData.

```
verbosity (int)
Set the level of print out. 0-completely quiet.  5- very verbose.

BuildType (string)
Adjust the build mode.  Options are:
"SingleFile" - only build one file
"FileList" - build a list of files into a single processed file
"Processing" - Not implemented yet, but will operate in a continuous mode

Mode (string)
Controls which RawData are loaded into the CStore.
Tank - Only load PMT data entries
MRD - Only load MRD data entries
TankAndMRD - load both PMT and MRD data entries
TankAndMRDAndCTC - load PMT, MRD, and CTC data entries

InputFile (string)
Input file that either points to a single raw data file (BuildType SingleFile) or
a file that has a list of raw data files (BuildType FileList).

DummyRunInfo (bool)
If 1, run information is filled with -1 values.  Used to bypass reading any
RunInformation if the file has no run information.

```

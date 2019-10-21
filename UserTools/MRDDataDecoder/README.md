# MRDDataDecoder

MRDDataDecoder

The MRDDataEncoder is a tool used to open MRD files and place TDC Data into a map
used by ANNIEEvent to sync data streams.  


#####Maps used in the tool#####

  Input: Raw data files.

  Output: FinishedMRDHits [map] is saved to CStore.


## Configuration

Describe any configuration variables for MRDDataDecoder.

verbosity (int)
    Integer that controls the level of log output shown when running.

Mode (string)
    Controls whether tool runs assuming a single file is input or if the tool
    will be run continously and searching for files in the data stream.  
    Current options are Continuous or SingleFile.

InputFile (string)
    String defining the path to a raw data file to process.  Only used if 
    Mode is set to FileList.


```
  Example of what you may want for a default config file:

  verbosity 2
  InputFile /path/to/VMEDataFile 
  LockStep 0
  CardDataEntriesPerExecute 5
```

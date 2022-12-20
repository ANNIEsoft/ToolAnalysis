# GetLAPPDEvents

The `GetLAPPDEvents` tool checks how many events with LAPPD raw data are present in ANNIE raw data BoostStore files. At the same time, it also counts the number of events for all other data streams (VME, MRD, Trigger) and writes all of them into an output file.

There are two distinct output files:
* one which contains the event numbers for all subsystem data streams
* and one which only contains the names of the raw data files which contain LAPPD data.

## Configuration

`GetLAPPDEvents` has the following configuration variables:

```
InputFile InputFile #file containing the list of input files (1 raw data file per line)
OutputFile OutputFile.csv #will be comma-separated and contains the number of VME,MRD,Trigger,LAPPD events per file
OutputFileLAPPD OutputFileLAPPD #this file will include a list of raw data files which contain at least 1 LAPPD event
```



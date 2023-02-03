# FilterLAPPDEvents

FilterLAPPDEvents

## Data


**ANNIEEvent**
* Takes the whole `ANNIEEvent` store and saves it if it contains LAPPD data, depending on the necessary `DesiredCuts` config variable.

**LAPPDStatistics<runnumbers>.csv** 
* Uses data from this input to determine whether a certain LAPPD event: occurs within the beamgate, contains an MRD track, or registered a hit in the FMV.


## Configuration

Describe any configuration variables for FilterLAPPDEvents.

```
DesiredCuts (string)
Indicates which types of LAPPD events you want the tool to save. It can either be LAPPDEvents, LAPPDEventsBeamgateMRDTrack, or LAPPDEventsBeamgateMRDTrackNoVeto

verbosity (int)
verbosity level of the Tool. Either 0 for no printouts, 1 for some printouts.

FilteredFilesBasename (string)
The basename of the filtered files. This will be appended to the SavePath.

SavePath (string)
Where to save the filtered files.

csvInputFile (string)
The LAPPDStatistics .csv file containing LAPPD event data.

RangeOfRuns (string)
The range of runs you will be filtering events through. This is appended to the SavePath and FilteredFilesBasename.
```

# saveLAPPDInfo

The `saveLAPPDInfo` is a demonstration tool how LAPPD data can be accessed and saved to an output file alongside reconstruction information from the other subsystems (tank PMTs, the MRD, and the trigger system). The associated toolchain can be found at `configfiles/LAPPD-ANNIEEvent/checkLAPPDStatus`.

The tool saves the following variables into an output csv file:

* run number
* subrun number
* part number
* event number
* global event number
* trigger time
* event time (LAPPD)
* Global LAPPD event offset
* Time since the beginning of the beamgate
* in beam window
* mrd track
* no veto    

## Configuration

The `saveLAPPDInfo` tool has the following configuration variables:

```
OutputFile outputfile.csv
```

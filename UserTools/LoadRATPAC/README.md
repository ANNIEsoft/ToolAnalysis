# LoadRATPAC

LoadRATPAC

This tool utilizes the RATEventLib libraries to read the RAT-PAC RATDS structure
into the ToolAnalysis store.  The RATEventLib library, available at:

https://github.com/pershint/RATEventLib

must first be compiled in the ToolDAQ directory before using this tool.

## Data

Describe any data formats LoadRATPAC creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks


## Configuration

Describe any configuration variables for LoadRATPAC.

```
loadTracks bool
Toggles whether to load all track information into the ANNIEEvent store.  Doing so
gives a full picture of the event structure, but makes the store bulky in size.

InputFile string
Give the path to the ratpac file to open

param2 value2
```

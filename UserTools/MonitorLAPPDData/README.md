# MonitorLAPPDData

MonitorLAPPDData

## Data

Describe any data formats MonitorLAPPDData creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks


Data persistance for handling certain plots (i.e. PPS Event Counting) is handled using ROOT files, which are written to the `PathMonitoring` directory while processing each run file, by the `WriteToFile` function, so that we can handle all of the sub-runs incrementally, and plot them in a single plot.

## Configuration

Describe any configuration variables for MonitorLAPPDData.

```
param1 value1
param2 value2
```

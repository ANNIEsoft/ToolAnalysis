# LAPPDnnls

LAPPDnnls

## Data

Describe any data formats LAPPDnnls creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the peaks by 
* doing a non-negative least squares analysis based on the waveform
* template found in pulsecharacteristics.root . 


## Configuration

Describe any configuration variables for LAPPDnnls.

```
param1 value1
param2 value2
```

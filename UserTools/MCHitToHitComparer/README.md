# MCHitToHitComparer

MCHitToHitComparer

## Data

Describe any data formats MCHitToHitComparer creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks


## Configuration

The MCHitToHitComparer tool is a tool meant for comparing time/charge information in
the MCHits and Hits maps found within the ANNIEEvent store.  Currently, this tool
only prints out the time/charge information in each store; however, the tool will
eventually be expanded to display more sophisticated checks, such as time and
charge distributions.


```
verbosity bool
```

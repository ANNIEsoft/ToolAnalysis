# ADCCalibrator

ADCCalibrator

## Data

Describe any data formats ADCCalibrator creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks

## Configuration

Describe any configuration variables for ADCCalibrator.

```
verbose
  An integer code representing the level of logging to perform

NumBaselineSamples
  The number of samples to use for each measurement of the ADC baseline

NumSubMinibuffers
  The number of sub-minibuffers to use when measuring the ADC baseline for
  non-Hefty mode data
```

# PhaseIIADCHitFinder

PhaseIIADCHitFinder

## Data

Describe any data formats PhaseIIADCHitFinder creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks

## Configuration

Describe any configuration variables for PhaseIIADCHitFinder.

PulseFindingApproach: string that defines whether pulses are searched for via
a threshold approach ("threshold") or with the NNLS algorithm ("NNLS").

ADCThresholdDB: Absolute path to a CSV file where each line is the pair:
channel_key, threshold

DefaultADCThreshold: Defines the default threshold to be used for any PMT
that does not have a channel_key, threshold value defined in the ADCThresholdDB
file.

DefaultThresholdType: Marks whether the given threshold values in the DB value are
relative to the calibrated baseline (relative), or absolute ADC counts (absolute).

PulseWindowStart: Start of pulse window relative to when adc trigger threshold
was crossed.  Only used when

```
PulseFindingApproach string
ADCThresholdDB string
DefaultADCThreshold unsigned short

DefaultThresholdType string

PulseWindowStart int
PulseWindowEnd int
```

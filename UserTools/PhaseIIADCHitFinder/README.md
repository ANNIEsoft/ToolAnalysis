# PhaseIIADCHitFinder

PhaseIIADCHitFinder

## Data

Describe any data formats PhaseIIADCHitFinder creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks

## Configuration

***Describe any configuration variables for PhaseIIADCHitFinder.***

PulseFindingApproach: string that defines whether pulses are searched for via
a threshold approach ("threshold") or with the NNLS algorithm ("NNLS").

ADCThresholdDB: Absolute path to a CSV file where each line is the pair:
channel_key, threshold.  Thresholds define the ADC threshold for each PMT used
when pulse-finding.

DefaultADCThreshold: Defines the default threshold to be used for any PMT
that does not have a channel_key, threshold value defined in the ADCThresholdDB
file.

DefaultThresholdType: Marks whether the given threshold values in the DB value are
relative to the calibrated baseline ("relative"), or absolute ADC counts ("absolute").

PulseWindowType: If using "threshold" on pulse finding approach, this toggle defines
how the pulse windows in a waveform are found.  Either fixed window ("fixed") or
the pulse windows are defined by crossing and un-crossing threshold ("dynamic").

PulseWindowStart: Start of pulse window relative to when adc trigger threshold
was crossed.  Only used when PulseFindingApproach==threshold and
PulseWindowType==fixed.  Unit is ADC samples.

PulseWindowEnd: End of pulse window relative to when adc trigger threshold
was crossed.  Only used when PulseFindingApproach==threshold and
PulseWindowType==fixed.  Unit is ADC samples.

***Types that are fed into each configuration file***

PulseFindingApproach string
ADCThresholdDB string
DefaultADCThreshold unsigned short

DefaultThresholdType string

PulseWindowType string
PulseWindowStart int
PulseWindowEnd int

```
```

# ADCCalibrator

The PhaseIIADCCalibrator tool is used to convert raw PMT waveforms into calibrated
waveforms.  The process for producing a calibrated waveform can be summed up to:

  - A single raw PMT waveform is first loaded. The beginning of the raw waveform
    is split into several sub-waveforms of a configurable amount of ADC samples. 
  - For each sub-waveform, the mean and variance are calculated.
  - An F-distribution test is computed for the variances of all sub-waveforms, comparing
    each neighboring sub-waveform.  This test has a null hypothesis of the 
    sub-waveforms having equal variance. 
    The details are in Steven Gardiner's thesis, section 9.1. 
  - The baseline mean and variance are calculated from the means and variances
    passing the F-test.
  - The raw waveform then has the baseline mean subtracted, and is converted
    to volts using the ADC_TO_VOLT variable in ANNIEconstants.h.  This voltage
    waveform is saved as the calibrated waveform.

Eventually, if data acquisition is moved to a "hefty mode" style acquisition, then
the first two points are replaced with the following:
  - A single PMT's set of minibuffers is loaded.  The beginning of each minibuffer
    is collected (number of samples is configurable with NumBaselineSamples).
  - For each minibuffer start, the mean and variance are calculated.

This option is not currently implemented, but could quickly be using the
approach taken in the ADCCalibrator tool (written for Phase I).

## Data

Describe any data formats PhaseIIADCCalibrator creates, destroys, changes, or analyzes. E.G.

The PhaseIIADCCalibrator tool reads the RawADCData map found in the ANNIEEvent store
and produces a map of channel keys to calibrated waveforms.  This is ultimately
stored in the CalibratedADCData map of the ANNIEEvent store.


## Configuration

Describe any configuration variables for ADCCalibrator.

```
verbosity int
  An integer code representing the level of logging to perform

NumBaselineSamples int
  The number of samples to split each sub-waveform into

NumSubWaveforms int
  Number of sub-waveforms to grab from the beginning of raw waveforms

```

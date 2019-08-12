# ADCCalibrator

The PhaseIIADCCalibrator tool is used to convert raw PMT waveforms into calibrated
waveforms.  The process for producing a calibrated waveform can be summed up to:

  - A single raw PMT waveform is first loaded.  A single waveform can be made of
    several minibuffers, that each have an array of ADC samples.
  - For each minibuffer, the first NumBaselineSamples is taken, and the mean and
    varianceare calculated.
  - An F-distribution test is computed for the variances of all minibuffers, comparing
    each neighboring minibuffer.  This test has a null hypothesis of the minibuffers
    having equal variance. The details are in Steven Gardiner's thesis, section
    9.1. 
  - The baseline mean and variance are calculated from the means and variances
    passing the F-test.
  - The raw waveform then has the baseline mean subtracted, and is converted
    to volts using the ADC_TO_VOLT variable in ANNIEconstants.h.  This voltage
    waveform is saved as the calibrated waveform.

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
  The number of samples to use for each measurement of the ADC baseline
```

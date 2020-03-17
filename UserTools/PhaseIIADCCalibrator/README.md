# ADCCalibrator

The PhaseIIADCCalibrator tool is used to convert raw PMT waveforms into calibrated
waveforms.  This option is not currently implemented, but could quickly be using the
approach taken in the ADCCalibrator tool (written for Phase I).  In each 
waveform, the baseline and variance in the baseline are estimated.  
The raw waveform then has the baseline mean subtracted, and is converted
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

BaselineEstimationType string 
  Define what algorithm to use when estimating the baseline (options: ze3ra or simple).
  "simple": Takes the mean and standard deviation of all samples in the first 
  NumBaselineSamples in the sub-waveform.
  
  "ze3ra": Uses the phase I baseline estimation algorithm from Phase I in each 
           sub-waveform.
  The process for producing a calibrated waveform can be summed up to:
  
    - The beginning of the raw waveform
      is split into several sub-waveforms of a configurable amount of ADC samples. 
    - For each sub-waveform, the mean and variance are calculated.  These are 
      configured with the NumSubWaveforms and NumBaselineSamples configurables.
    - An F-distribution test is computed for the variances of all sub-waveforms, comparing
      each neighboring sub-waveform.  This test has a null hypothesis of the 
      sub-waveforms having equal variance. 
      The details are in Steven Gardiner's thesis, section 9.1. 
    - The baseline mean and variance are calculated from the means and variances
      passing the F-test.
  
  Eventually, if data acquisition is moved to a "hefty mode" style acquisition, then
  the first two points are replaced with the following:
    - A single PMT's set of minibuffers is loaded.  The beginning of each minibuffer
      is collected (number of samples is configurable with NumBaselineSamples).
    - For each minibuffer start, the mean and variance are calculated.



NumBaselineSamples int
  The number of samples to split each sub-waveform into

NumSubWaveforms int
  Number of sub-waveforms to grab from the beginning of raw waveforms

MakeCalLEDWaveforms int
  If true, PhaseIIADCCalibrator takes raw waveforms and produces smaller 
  raw and calibrated waveforms using the ADC windows defined in the 
  WindowIntegrationDB file.  These are stored into the ANNIEEvent booststore
  in "CalibratedLEDADCData" and "RAWLEDADCData".

WindowIntegrationDB string
  File path to a text file with lines all of the line structure:
  channel_key,window_min,window_max.  A raw and calibrated waveform will
  be produced for each window range specified for each channel_key.  Multiple
  windows can be specified for each channel.

```
```

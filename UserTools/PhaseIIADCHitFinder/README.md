# PhaseIIADCHitFinder

PhaseIIADCHitFinder

## Data

Describe any data formats PhaseIIADCHitFinder creates, destroys, changes, or analyzes. E.G.

## Configuration

***Describe any configuration variables for PhaseIIADCHitFinder.***

UseLEDWaveforms [int]: Specifies whether hits and pulses are found using the 
       raw waveforms from the DAQ, or the LED waveform windows produced from running 
       PhaseIIADCCalibrator with MakeLEDWaveforms set to 1.  
       1=Use LED window waveforms, 
       0 = Use full waveforms.

###### PULSE FINDING TECHNIQUES #########

PulseFindingApproach [string]: String that defines what algorithm is used to find pulses.
Possible options:

  "threshold": Search for an ADC sample to cross some defined threshold.  Threshold 
is manipulable using DefaultADCThreshold and DefaultThresholdType config variables.

  "fixed_window": Fixed windows defined in the WindowIntegrationDB text file are
                  treated entirely as pulses.

  "full_window": Every waveform is integrated completely and background-subtracted
                 to form a single pulse object.

  "full_window_maxpeak": The maximum peak anywhere in the window is taken as the pulse.  
                 the pulse is integrated to either side of the max until dropping to 
                 < 10% of the max peak amplitude, then background-subtracted.

  "signal_window_maxpeak": The maximum peak anywhere beyond the baseline estimation window
                  is taken as the pulse.  
                 the pulse is integrated to either side of the max until dropping to 
                 < 10% of the max peak amplitude, then background-subtracted.
  
  "NNLS": Uses the NNLS algorithm that will be applied to LAPPD hit reconstruction.
          Not yet implemented.

###### "threshold" setting configurables ########

DefaultADCThreshold [int]: Defines the default threshold to be used for any PMT
      that does not have a channel_key, threshold value defined in the ADCThresholdDB
      file.

DefaultThresholdType [string]: Marks whether the given threshold values in the DB value are
      relative to the calibrated baseline ("relative"), or absolute ADC counts ("absolute").

PulseWindowType [string]: If using "threshold" on pulse finding approach, this toggle defines
      how the pulse windows in a waveform are found.  Either fixed window ("fixed") or
      the pulse windows are defined by crossing and un-crossing threshold ("dynamic").

PulseWindowStart [int]: Start of pulse window relative to when adc trigger threshold
      was crossed.  Only used when PulseFindingApproach==threshold and
      PulseWindowType==fixed.  Unit is ADC samples.

PulseWindowEnd [int]: End of pulse window relative to when adc trigger threshold
      was crossed.  Only used when PulseFindingApproach==threshold and
      PulseWindowType==fixed.  Unit is ADC samples.

ADCThresholdDB [string]: Absolute path to a CSV file where each line is the pair
      channel_key (int), threshold (int).  For any channel_key,threshold pair defined in the 
      config file, these thresholds will be used in place of the default ADC threshold.  
      Thresholds define the ADC threshold for each PMT used when pulse-finding.

###### "fixed_windows" setting configurables ######

WindowIntegrationDB [string]: Absolute path to a CSV file where each line has the format:
        channel_key,window_min,window_max
      A channel can be given multiple integration windows.  Windows are in ADC samples.
      A single pulse will be calculated for each integration window defined.

```
```

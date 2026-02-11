# PhaseIIADCHitFinder

Hit finding tool for the tank PMTs. The configurable settings and types of pulse integration are listed below, but the main feature of the tool is to identify pulses via:
- With a threshold-style approach (default - used in the EventBuilder and in the MC waveform workflow)
- Search for the maximum peak within the entire buffer (used for Gains calibration with LED data)
- Within a fixed, pre-defined window (used for assessing the pedestal / (dark) noise rates or other relevant analyses)

## Data

After identifying a pulse and obtaining the integrated charge [nQ], the pulse timing [ns], and other features of the identified "hit", the tool will populate the ADCPulse class (DataModel/ADCPulse), storing:
- `int` TubeID (PMT channel ID)
- `double` start_time (Returns the start time (ns) of the pulse relative to the start of its minibuffer)
- `double` peak_time (Returns the peak time (ns) of the pulse relative to the start of its minibuffer)
- `double` baseline (Returns the approximate baseline (ADC) used to calibrate the pulse)
- `double` sigma_baseline (Returns the approximate error on the baseline (ADC) used to calibrate the pulse)
- `unsigned long` area (Returns the area (ADC * samples) of the uncalibrated pulse)
- `unsigned short` raw_amplitude (Returns the amplitude (ADC) of the uncalibrated pulse)
- `double` calibrated_amplitude (eturns the amplitude (V) of the calibrated (baseline-subtracted) pulse)
- `double` charge (Returns the charge (nC) of the calibrated (baseline-subtracted) pulse)
- `const std::vector<double>&` trace_x (Returns the x [ns] points of the "found" pulse (baseline-subtracted and relative to pulse start point))
- `const std::vector<double>&` trace_y (Returns the y [ADC] points of the "found" pulse (baseline-subtracted and relative to pulse start point))

into: `std::map<unsigned long, std::vector< std::vector<ADCPulse>>` RecoADCHits

It will then store the hit charge [nQ] and the hit time [ns] to the store via: `std::map<unsigned long,std::vector<Hit>>*` Hits

The tool will set additional objects to the store:
- ADCThreshold (Threshold used to identify pulses)
- InProgressHits, InProgressRecoADCHits, InProgressHitsAux, InProgressRecoADCHitsAux, NewHitsData (for book-keeping in the event building mode)

Lastly, for the auxillary channels, it will store the ADCPulse and Hit instead to:
- `std::map<unsigned long, std::vector< std::vector<ADCPulse>>` RecoADCAuxHits
- `std::map<unsigned long,std::vector<Hit>>*` AuxHits

## Configuration

***Describe any configuration variables for PhaseIIADCHitFinder.***

UseLEDWaveforms [int]: Specifies whether hits and pulses are found using the 
       raw waveforms from the DAQ, or the LED waveform windows produced from running 
       PhaseIIADCCalibrator with MakeLEDWaveforms set to 1.  
       1=Use LED window waveforms, 
       0 = Use full waveforms.

MCWaveforms [int]: Whether the `PMTWaveformSim` MC waveform generator is being used for MC Hits.

EventBuilding [int]: Whether this tool is being used in the event building toolchain.

###### PULSE FINDING TECHNIQUES #########

PulseFindingApproach [string]: String that defines what algorithm is used to find pulses.
Possible options:

  - "threshold": (DEFAULT for event building and MC waveforms) Search for an ADC sample to cross some defined threshold.  Threshold 
is manipulable using DefaultADCThreshold and DefaultThresholdType config variables.

 - "fixed_window": Fixed windows defined in the WindowIntegrationDB text file are
                  treated entirely as pulses.

 - "full_window": Every waveform is integrated completely and background-subtracted
                 to form a single pulse object.

 - "full_window_maxpeak": The maximum peak anywhere in the window is taken as the pulse.  
                 the pulse is integrated to either side of the max until dropping to 
                 < 10% of the max peak amplitude, then background-subtracted. This is 
                 used in the LED analysis (use with find_pulses_bywindow)

 - "signal_window_maxpeak": The maximum peak anywhere beyond the baseline estimation window
                  is taken as the pulse.  
                 the pulse is integrated to either side of the max until dropping to 
                 < 10% of the max peak amplitude, then background-subtracted.
  
 - "NNLS": Uses the NNLS algorithm that will be applied to LAPPD hit reconstruction.
          Not yet implemented.

###### "threshold" setting configurables ########

- DefaultADCThreshold [int]: Defines the default threshold to be used for any PMT
      that does not have a channel_key, threshold value defined in the ADCThresholdDB
      file.

- DefaultThresholdType [string]: Marks whether the given threshold values in the DB value are
      relative to the calibrated baseline ("relative"), or absolute ADC counts ("absolute").

- PulseWindowType [string]: If using "threshold" on pulse finding approach, this toggle defines
      how the pulse windows in a waveform are found.  There are three options: fixed window ("fixed"),
      dynamic window where the pulse windows are defined by crossing and un-crossing threshold ("dynamic"),
      and ("Fixed_2023_Gains") which implements the same integration window used in the 2023 Gains calibration 
      where the pulse windows are defined by crossing and un-crossing the baseline.

- PulseWindowStart [int]: Start of pulse window relative to when adc trigger threshold
      was crossed.  Only used when PulseFindingApproach==threshold and
      PulseWindowType==fixed.  Unit is ADC samples.

- PulseWindowEnd [int]: End of pulse window relative to when adc trigger threshold
      was crossed.  Only used when PulseFindingApproach==threshold and
      PulseWindowType==fixed.  Unit is ADC samples.

- ADCThresholdDB [string]: Absolute path to a CSV file where each line is the pair
      channel_key (int), threshold (int).  For any channel_key,threshold pair defined in the 
      config file, these thresholds will be used in place of the default ADC threshold.  
      Thresholds define the ADC threshold for each PMT used when pulse-finding.

###### "fixed_windows" setting configurables ######

WindowIntegrationDB [string]: Absolute path to a CSV file where each line has the format:
        channel_key,window_min,window_max
      A channel can be given multiple integration windows.  Windows are in ADC samples.
      A single pulse will be calculated for each integration window defined.


## Example of working configurations
##### EventBuilding #####
```
verbosity 0

UseLEDWaveforms 0

PulseFindingApproach threshold
PulseWindowType Fixed_2023_Gains
DefaultADCThreshold 7
DefaultThresholdType relative

EventBuilding 1
MCWaveforms 0
```

##### PrintADCData (obtaining raw traces for pulse analysis) #####
```
verbosity 0

UseLEDWaveforms 0

PulseFindingApproach threshold
PulseWindowType Fixed_2023_Gains
DefaultADCThreshold 7
DefaultThresholdType relative

EventBuilding 0
MCWaveforms 0
```

##### MC #####
```
verbosity 0

UseLEDWaveforms 0

PulseFindingApproach threshold
PulseWindowType Fixed_2023_Gains
DefaultADCThreshold 7
DefaultThresholdType relative

EventBuilding 0
MCWaveforms 1
```

##### Gains #####
```
verbosity 0

UseLEDWaveforms 1

PulseFindingApproach full_window_maxpeak

EventBuilding 0
MCWaveforms 0
```

## Tools needed to run this tool successfully
All working configurations must include at minimum the following tools:
```
LoadGeometry
PhaseIIADCCalibrator
PhaseIIADCHitFinder
```

This is executed either:
1. Over RAWData as part of the event building procedure
2. Over ProcessedData that contains the raw waveforms instead of the extracted hits information (no hit finding was ran during the event building to purposely give you the raw waveforms)
3. On MC waveforms generated by the `PMTWaveformSim` tool to give you data-like MC hits

In all three cases additional tools are needed.

For 1. (RAWData for event building):
```
LoadGeometry
LoadRawData (EventBuilder or DataDecoder) or EBLoadRaw (EventBuilderV2)
PMTDataDecoder
TriggerDataDecoder
PhaseIIADCCalibrator
PhaseIIADCHitFinder
```

For 2. (ProcessedData with raw waveforms):
```
LoadGeometry
LoadANNIEEvent
PhaseIIADCCalibrator
PhaseIIADCHitFinder
```

For 3. (MC waveforms):
```
LoadGeometry
LoadWCSim
PMTWaveformSim
PhaseIIADCHitFinder
```


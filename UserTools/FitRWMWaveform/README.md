# FitRWMWaveform

FitRWMWaveform tool takes the auxilliary channel waveforms saved in the Processed Data and peforms rising edge fitting to determine the rising edge time. 

Currently the two auxillary waveforms are the Booster RF and the Booster RWM signals (https://github.com/ANNIEsoft/ToolAnalysis/blob/ff86893aa905652621594f8ac093be45f3bd13b5/configfiles/LoadGeometry/AuxChannels.csv#L5).

Motivation for this tool and details of the waveform shape and fitting can be found here: https://annie-docdb.fnal.gov/cgi-bin/sso/ProcessDocumentAdd 

## Data

The tool will populate the following objects (all `double` datatype):

### RWM fits
* **RWMRisingStart**: Time where the RWM trace crosses predefined threshold
* **RWMRisingEnd**: Time where RWM trace crosses below predefined threshold (at the end)
* **RWMHalfRising**: Time where the RWM rising edge crosses 50% of its maximum above baseline.
* **RWMFHWM**: FWHM of the RWM waveform (sorry for the typo).
* **RWMFirstPeak**: After the rising edge of the RWM trace, there is often a sharp peak. This the time of this first peak.
  
### BRF fit
* **BRFFirstPeak**: For the BRF waveform, fit of the first BRF pulse. 
* **BRFAveragePeak** The average (or baseline) y-value (in ADC) of the signal across the entire window.
* **BRFFirstPeakFit** Fit for the half-rising time of the first BRF pulse.


## Configuration

Configuration variables for FitRWMWaveform.

```
verbosityFitRWMWaveform 0             # verbosity level
printToRootFile 1                     # 1 to print the RWM and BRF waveforms to an output root file, 0 for no printing
maxPrintNumber 100                    # maximum number of waveforms to save to the output root file
output_filename RWMBRFWaveforms.root  # name of the output root file
```

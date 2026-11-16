# FitRWMWaveform

FitRWMWaveform tool takes the auxilliary channel waveforms saved in the Processed Data and peforms rising edge fitting to determine the rising edge time. 

Currently the three auxillary waveforms are fitted: the Booster RF (BRF), the Booster RWM and the Booster Extraction signal (BES), all listed at [AuxChannels.csv](https://github.com/ANNIEsoft/ToolAnalysis/blob/Application/configfiles/LoadGeometry/AuxChannels.csv)

Motivation for this tool and details of the waveform shape and fitting can be found in the docdb.
Link to be added later.

## Data

The tool will populate the following objects (all `double` datatype):

### RWM fits
* **RWMRisingStart** (ns): Sample just before the first sharp rise (> 400 ADC and a jump of > 50 ADC from the previous sample). Steps of 2 ns.
* **RWMRisingEnd** (ns): Highest sample within 60 samples after RWMRisingStart, i.e. the top of the rising edge.
* **RWMHalfRising** (ps): Time where the RWM rising edge crosses 50% between baseline and plateau, interpolated between samples.
* **RWMFHWM** (ps): FWHM of the RWM waveform (sorry for the typo), from the interpolated 50% rising and falling times.
* **RWMFirstPeak** (ns): First sample above 1600 ADC.

### BRF fit
* **BRFFirstPeak** (ns): The highest sample in the first 10 samples. Steps of 2 ns.
* **BRFAveragePeak** (ns): Average time of the highest sample within each 10-sample region.
* **BRFFirstPeakFit** (ps): 50% time of the first full rising edge of the BRF signal.

### BES fit
* **BESstartTime** (ps): 50% time on the leading edge of the negative BES pulse (searched in samples 250-450).
* **BESendTime** (ps): 50% time on the leading edge of the positive BES pulse (searched from sample 800 to the end).
* **BESwidth** (ps): Width of the BES signal (BESendTime - BESstartTime).

## Configuration

Configuration variables for FitRWMWaveform.

```
verbosityFitRWMWaveform 0             # verbosity level
printToRootFile 1                     # 1 to print the RWM, BRF and BES waveforms to an output root file, 0 for no printing
maxPrintNumber 100                    # maximum number of waveforms to save to the output root file
output_filename RWMBRFWaveforms.root  # name of the output root file
```

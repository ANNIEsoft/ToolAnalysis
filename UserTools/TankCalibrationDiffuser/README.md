# TankCalibrationDiffuser

The `TankCalibrationDiffuser` tool analyzes the PMT responses after laser light has been distributed uniformly across the whole detector with a diffusion ball. The time and charge response of all PMTs is evaluated 

## Data

The TankCalibrationDiffuser tool needs access to the hit information within ANNIE events:

**MCHits** `map<unsigned long, vector<Hit>>`
* Takes this data from the `ANNIEEvent` store and evaluates whether observed and expected times and charges correspond to one another.

## Configuration

The following variables can be configured for the TankCalibrationDiffuser tool:

```
# TankCalibrationDiffuser Config File

HitStore MCHits #Specify whether to make plots wit hits in Hits or MCHits store in ANNIEEvent
OutputFile SimulationCalibrationTest #Output root prefix name for the current run
DiffuserX 0.      	#x-position of the diffuser ball
DiffuserY 0.      	#y-position of the diffuser ball
DiffuserZ 0.      	#z-position of the diffuser ball
ToleranceCharge 0.5	#tolerance of fit single p.e. value for being classified as a bad PMT
ToleranceTime 0.5	#tolerance of mean time value [ns] for being classified as a bad PMT
FitMethod Gaus2Exp	#fit function for charge, options: Gaus2Exp (2 times gaus + exp), Gaus2 (2 times gaus), Gaus (single gaus)
TApplication 0		#0/1, depending on whether plots should be shown interactively or not

verbose 1         #verbosity of the application

```

## OutputFiles

The tool produces two output files:
* a root file `<OutputFile>_PMTStability_Run<RunNumber>.root` which contains the charge & time histograms for all the PMTs and overview plots of the fitted charge and time distributions
* a txt file `<OutputFile>_Run<RunNumber>_pmts_laser_calibration.txt`, which contains a PMT-by-PMT summary of the calibration run information. The columns are (from left to right)
  * PMT Detkey
  * Fitted charge - mean
  * Fitted charge - sigma
  * Fitted time - mean
  * Fitted time - sigma
  * Time deviation (observed / expected hit time)
* The last line of the .txt-file contains the average fit information averaged over all PMTs (assigned to detectorkey 10000)

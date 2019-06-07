# TankCalibrationDiffuser

The `TankCalibrationDiffuser` tool analyzes the PMT responses after laser light has been distributed uniformly across the whole detector with a diffusion ball. The time and charge response of all PMTs is evaluated 

## Data

The TankCalibrationDiffuser tool needs access to the hit information within ANNIE events:

**MCHits** `map<unsigned long, vector<Hit>>`
* Takes this data from the `ANNIEEvent` store and evaluates whether observed and expected times and charges correspond to one another.

## Configuration

The following variables can be configured for the TankCalibrationDiffuser tool:

```
OutputFile /ANNIECode/ToolAnalysis/histogrammed_pmt_charge.root #Output root file for the current calibration run
StabilityFile /ANNIECode/ToolAnalysis/stability_pmtcharge.root  #Output root file that displays the stability over multiple runs
GeometryFile configfiles/TankCalibrationDiffuser/geofile_128PMTs.txt    #Geometry file specifying the different radii of the PMTs (taken from file geofile.txt in WCSim installation directory for the respective installation)
DiffuserX 0.      #x-position of the diffuser ball
DiffuserY 0.      #y-position of the diffuser ball
DiffuserZ 0.      #z-position of the diffuser ball
ToleranceCharge 0.5     #tolerance of fit single p.e. value for being classified as a bad PMT
ToleranceTime 0.5       #tolerance of mean time value [ns] for being classified as a bad PMT
TApplication 0          #0/1, depending on whether plots should be shown interactively or not

verbose 1         #verbosity of the application

```

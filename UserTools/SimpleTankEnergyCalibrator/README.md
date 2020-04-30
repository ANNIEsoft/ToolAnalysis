# SimpleTankEnergyCalibrator

SimpleTankEnergyCalibrator

## Data

This tool uses through-going muons to estimate the total photoelectrons observed per 
MeV deposition in the ANNIE tank.  Events which have front muon veto activity and
a single through-going track are selected, and the total number of photoelectrons
observed in the beam acquisition window is summed.


## Configuration

Describe any configuration variables for SimpleTankEnergyCalibrator.

```
TankBeamWindowStart int
TankBeamWindowEnd int
TankNHitThreshold int
MinPenetrationDepth int
MaxAngle int
MaxEntryPointRadius int

```

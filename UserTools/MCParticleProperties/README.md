# MCParticleProperties

MCParticleProperties loops through the `MCParticles` object in the `ANNIEEvent` store and calculates different properties of the particles based on MC 

## Data

The `MCParticleProperties` tool loops over `MCParticles` and adds the following features for each `MCParticle` object:

* StartsInFiducialVolume (`bool`)
* TrackAngleX (`double`)
* TrackAngleY (`double`)
* TrackAngleFromBeam (`double`)
* ProjectedHitMrd (`bool`)
* EntersMrd (`bool`)
* MrdEntryPoint (`Position`)
* ExitsMrd (`bool`)
* MrdExitPoint (`Position`)
* PenetratesMrd (`bool`)
* MrdPenetration (`double`)
* NumMrdLayersPenetrated (`int`)
* TrackLengthInMrd (`double`)
* MrdEnergyLoss (`double`)
* EntersTank (`bool`)
* TankEntryPoint (`Position`)
* ExitsTank (`bool`)
* TankExitPoint (`Position`)
* TrackLengthInTank (`double`)

## Configuration

Describe any configuration variables for MCParticleProperties.

```
verbosity 1

```

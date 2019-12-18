# MCRecoEventLoader

MCRecoEventLoader

The MCRecoEventLoader tool is used to load truth information associated with a
particle vertex into the RecoEvent store.  
This truth information can be used as a seed for the reconstruction
tools, and is ultimately passed to the PhaseIITreeMaker tool and saved in the
final output.

The MCRecoEventLoader tool also loads in the number of pions/kaons that were
parent particles for the event in consideration.

## Data

MCRecoEventLoader laods the following information into the `RecoEvent` store:

* `MCPi0Count`: Number of primary Pi0s
* `MCPiPlusCount`: Number of primary PiPlus
* `MCPiMinusCount`: Number of primary PiMinus
* `MCK0Count`: Number of primary K0s
* `MCKPlusCount`: Number of primary KPlus
* `MCKMinusCount`: Number of primary KMinus
* `NRings`: Number of rings
* `IndexParticlesRing`: The indices of primary particles within the `MCParticles` object in `ANNIEEvent`
* `TrueVertex`: The true start vertex of the selected primary particle
* `TrueStopVertex`: The true stop vertex of the selected primary particle
* `TrueMuonEnergy`: The true energy of the selected primary particle
* `TrueTrackLengthInWater`: The true track length of the selected primary particle in water
* `TrueTrackLengthInMRD`: The true track length of the selected primary particle in the MRD
* `ProjectedMRDHit`: Does the (extended) trajectory of the selected primary particle hit the MRD?

## Configuration

Describe any configuration variables for MCRecoEventLoader.

```
GetPionKaonInfo bool
Decides whether or not true pion/kaon information is loaded into the RecoEvent store.
If the MCPiK flag is to be used in the EventSelector, this must be set to 1!

GetNRings bool
Decides whether the number of true rings should be calculated based on the Cherenkov thresholds of 
the found primary particles

ParticleID int
The given integer is used to determine which parent particle type's vertex
information is loaded into the RecoEvent store.  Default is muon.



```

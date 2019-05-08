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

Describe any data formats MCRecoEventLoader creates, destroys, changes, or analyzes. E.G.



## Configuration

Describe any configuration variables for MCRecoEventLoader.

```
GetPionKaonInfo bool
Decides whether or not true pion/kaon information is loaded into the RecoEvent store.
If the MCPiK flag is to be used in the EventSelector, this must be set to 1!

ParticleID int
The given integer is used to determine which parent particle type's vertex
information is loaded into the RecoEvent store.  Default is muon.

```

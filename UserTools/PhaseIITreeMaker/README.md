# PhaseIITreeMaker

PhaseIITreeMaker

## Data

The PhaseIITreeMaker takes various information from the ANNIEEvent and from other
processing tools (such as the ClusterFinder and ClusterClassifiers tools) and saves
the information into an ntuple for offline analysis.  Configurables defining what
information to save into the 'PhaseIITriggerTree' and 'PhaseIIClusterTree's is 
discussed below.


## Configuration

```
verbose (1 or 0)
Defines the level of verbosity for outputs of PhaseIITreeMaker algorithm.

OutputFile TestFile.ntuple.root
Output filename

TankClusterProcessing 1
Process PMT cluster-level trees.  Each ntuple entry contains all the PMT hits observed
in a cluster (as defined in the ClusterFinder tool) as well as cluster classifiers
(as defined in the ClusterClassifiers tool), along with other general information 
(run/subrun number, nhits, SiPM hits, trigger time).

MRDClusterProcessing 1
Process MRD cluster-level trees.  Each ntuple entry contains all the MRD hits observed
in a cluster (as defined in the TimeClustering tool), along with other general information 
(run/subrun number, nhits, trigger time). 

TriggerProcessing 1
Process trigger-level trees.  Each ntuple entry contains all PMT hits observed
for a given trigger, along with other general information (run/subrun number,
nhits,trigger time,beam parameters).

TankHitInfo_fill 1
Fill in PMT hit information for all hits (Time,Charge,PE,Position).

MRDHitInfo_fill 1
Fill in MRD hit information for all hits (Time,Channel ID).

SiPMPulseInfo_fill 1
Fill in SiPM pulse information (charge/time/SiPM number).

MRDReco_fill 0
Fill in track reconstruction parameters defined by the FindMRDTracks tool.

fillCleanEventsOnly (1 or 0)
Only fill tree with events that pass the event selection defined in the
EventSelector tool.

Reco_fill 0
Fill in final reconstruction parameters estimated using the Tank
Reconstruction algorithms.

MCTruth_fill (1 or 0)
Input will determine if Truth information from files given is saved to the
reco tree.  Will output to tree if 1.

Reweight_fill (1 or 0)
Input will determine if reweights from flux and xsec will be saved to tree.
Will output to tree if 1.

SimpleReco_fill (1 or 0)
Input will determine if info from SimpleReconstruction will be saved to reco
tree. Will output to tree if 1.

muonTruthRecoDiff_fill (1 or 0)
Input determines if the difference in truth and reco information is saved to
the reco tree.  Will output to tree if 1.

RecoDebug_fill (1 or 0)
Input determines if reconstruction variables at each step in the muon event
reconstruction chain are saved to the tree.  Values include seeds from SeedVtxFinder,
fits from PointPosFinder, and FOMs for likelihood fits at each reconstruction step.
Will output to tree if 1.

IsData (1 or 0)
Whether the data we are processing is real (Hits) or MC (MCHits).

HasGenie (1 or 0)
Input determines whether GENIE-level information is loaded by the LoadGENIEEvent tool.

HasBNBtimingMC (1 or 0)
Input determines whether cluster times (MC) are spill-corrected to produce a BNB-realistic timing structure.
Must have HasGenie 1 and TankClusterProcessing 1 enabled.
(as defined in the AssignBunchTimingMC tool).  

```

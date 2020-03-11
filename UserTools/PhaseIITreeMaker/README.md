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
ClusterProcessing 1
Process cluster-level trees.  Each ntuple entry contains all the PMT hits observed
in a cluster (as defined in the ClusterFinder tool) as well as cluster classifiers
(as defined in the ClusterClassifiers tool), along with other general information 
(run/subrun number, nhits, SiPM hits, trigger time).  

TriggerProcessing 1
Process trigger-level trees.  Each ntuple entry contains all PMT hits observed
for a given trigger, along with other general information (run/subrun number,
nhits,trigger time).


HitInfo_fill 1
Fill in hit information for all hits (Time,Charge,PE,Position).


SiPMPulseInfo_fill 1
Fill in SiPM pulse information (charge/time/SiPM number).

fillCleanEventsOnly (1 or 0)
Only fill tree with events that pass the event selection defined in the
EventSelector tool.


Reco_fill 0
Fill in final reconstruction parameters estimated using the Tank
Reconstruction algorithms.


MCTruth_fill (1 or 0)
Input will determine if Truth information from files given is saved to the
reco tree.  Will output to tree if 1.

muonTruthRecoDiff_fill (1 or 0)
Input determines if the difference in truth and reco information is saved to
the reco tree.  Will output to tree if 1.

RecoDebug_fill (1 or 0)
Input determines if reconstruction variables at each step in the muon event
reconstruction chain are saved to the tree.  Values include seeds from SeedVtxFinder,
fits from PointPosFinder, and FOMs for likelihood fits at each reconstruction step.
Will output to tree if 1.

```

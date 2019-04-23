# EventSelector

EventSelector

## Data

The event selector tool flags events in the input data file as passing or failing
various event selection criteria.  Current event selection flags that can be
utilized include:
  - MCPiKCut: Flags events that had a primary muon or pion produced
  - MRDRecoCut Flags muon events that do not stop in the MRD according to reco.
  - MCFVCut: Flags MC muon events that are not generated in the defined Fiducial volume 
  - MCPMTVolCut: Flags MC muon events that are not generated inside the PMT volume 
  - MCMRDCut: Flags MC muon events with muons that do not stop in the MRD
  - PromptTrigOnly: Flags muon events that do not have an MCTriggernum of 0
                    (any store event with MCTriggernum>0 is a delayed trigger)
  - NHit: Flags events that do not have at least some number of hits (default is 4)
  - Identical cuts, but with the reconstructed information, are prefaced with "Reco"

After running the event selector tool, two bitmasks are added to the RecoEvent
store: "EventFlagApplied" and "EventFlagged".  The prior indicates which event
selection criteria were checked on the event, while the latter indicates which
event selection the event actually failed.  The bitmask as of now is given by:

   kFlagNone         = 0x00, //0
   kFlagMCFV         = 0x01, //1
   kFlagMCPMTVol     = 0x02, //2
   kFlagMCMRD        = 0x04, //4
   kFlagMCPiK        = 0x08, //8
   kFlagRecoMRD      = 0x10, //16
   kFlagPromptTrig   = 0x20, //32
   kFlagNHit         = 0x40, //64
   kFlagRecoFV       = 0x80, //128
   kFlagRecoPMTVol   = 0x100, //256

For each event, all cuts defined with the config file are checked.  

The fSaveStatusToStore bool determines if the "EventCutStatus" bool in the store
is updated after running event selection.  If the event
passes all cuts and fSaveStatusToStore==true, EventCutStatus is set to true in 
the RecoEvent store.  If any cut fails, EventCutStatus is set to false.

The EventCutStatus tool is used by downstream tools to determine whether to run
the tool or not.  


## Configuration

Configurables tell EventSelector which cuts to run/check when defining the
master EventCutStatus bit checked by all following tools

```
All bools (1 or 0)
verbosity
MRDRecoCut
MCFVCut
MCPMTVolCut
MCMRDCut
MCPiKCut
NHitCut
PromptTrigOnly
RecoFVCut
RecoPMTVolCut
SaveStatusToStore
```

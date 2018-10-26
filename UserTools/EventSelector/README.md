# EventSelector

EventSelector

## Data

The event selector tool flags events in the input data file as passing or failing
various event selection criteria.  Current event selection flags that can be
utilized include:
  - MRDRecoCut Flags muon events that reconstruct within a defined radius and height
  - MCTruthCut: Flags MC muon events generated within a defined radius and height 

For each event, all cuts defined with the config file are checked.  If the event
passes all cuts, EventCutStatus is set to true and saved to the RecoEvent store.
If any cut fails, EventCutStatus is set to false.

Eventually, this tool will also add a bit mask to the RecoEvent store telling which
Cuts were checked when the EventSelector tool was run.  The bitmask will have bits
filled as defined below in the configuration (first cut defined -> least
significant bit). 

## Configuration

Configurables tell EventSelector which cuts to run/check when defining the
master EventCutStatus bit checked by all following tools

```
verbosity bool
MRDRecoCut (1 or 0)
MCTruthCut (1 or 0)
```

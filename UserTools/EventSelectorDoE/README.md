# EventSelectorDoE

EventSelectorDoE

## Data

The event selector tool flags events in the input data file as passing or failing
various event selection criteria.  Current event selection flags that can be
utilized include:
  - MCTruthCut: Flags MC muon events generated within a defined radius and height,
                and requires a non-zero track length on the Muon 
  - PromptTrigOnly: Flags any event with MCTriggernum > 0.  For the DoE files, this
                is true for all events since only prompt info was stored

For each event, all cuts defined with the config file are checked.  If the event
passes all cuts, EventCutStatus is set to true and saved to the RecoEvent store.
If any cut fails, EventCutStatus is set to false.

Eventually, this tool will also add a bit mask to the RecoEvent store telling which
Cuts were checked when the EventSelectorDoE tool was run.  The bitmask will have bits
filled as defined below in the configuration (first cut defined -> least
significant bit). 

## Configuration

Configurables tell EventSelectorDoE which cuts to run/check when defining the
master EventCutStatus bit checked by all following tools

```
verbosity bool
MCTruthCut (1 or 0)
PromptTrigOnly (1 or 0)
```

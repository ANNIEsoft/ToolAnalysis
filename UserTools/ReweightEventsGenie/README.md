# ReweightEventsGenie

ReweightEventsGenie

## Data

Describe any data formats ReweightEventsGenie creates, destroys, changes, or analyzes. E.G.

**flux_weights** `map<string, vector<double>>`
* Beam flux weight systematics

**xsec_weights** `map<string, vector<double>>`
* Cross section weight systematics

**MCisCC** `bool`
* Truth. Is the event CC?

**MCQsquared** `double`
* True momentum transfer squared

**MCNuPx** `double`
**MCNuPy** `double`
**MCNuPz** `double`
**MCNuE** `double`
**MCNuPDG** `int`
* True neutrino momentum, energy, and PDG

**MCTgtPx** `double`
**MCTgtPy** `double`
**MCTgtPz** `double`
**MCTgtE** `double`
**MCTgtPDG** `int`
**MCTgtisP** `bool`
**MCTgtisN** `bool`
* True target momentum, energy, PDG, and is the target a proton or neutron

**MCFSLPx** `double`
**MCFSLPy** `double`
**MCFSLPz** `double`
**MCFSLE** `double`
**MCFSLm** `double`
* True final state lepton momentum, energy, and mass

**MC0pi0** `bool`
* Truth. There are no neutral charged pions in this event.

**MC0piPMgt160** `bool`
* Truth. There are no charged pions above Cherenkov threshold in this event.



## Configuration

Describe any configuration variables for ReweightEventsGenie.

```
param1 key1:value1|key2:value2|key3:value3
```

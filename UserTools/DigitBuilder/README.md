# Interface

Interface

## Data

This tool is responsible for loading hit information from the ANNIEEvent
store and converting them to RecoDigits.  RecoDigits are the standard hit format
used by all tools in the reconstruction chain.  

This tool is also used to save MC truth information into the RecoEvent store,
if the data is simulated and has truth info.  Muon start and stop vertex information
are gathered, as well as the number of pions and kaons that are present in the
event's parent particles.

By default, all individual PMT true hit times and charges (with time smeared in WCSim)
are loaded into a RecoDigit.  True LAPPD hit times, with a 100 ps time smearing, are
loaded.

## Configuration

Describe any configuration variables for Interface.

```
ParametricModel bool

If this bool is set to 1, the PMT parametric model of hits is used to fill RecoDigits.
To form a PMT parametric model hit, the following process is performed for a PMT:
  - Gather all true hits on the PMT in the current event.
  - The parametric hit time is taken as the median of all hit times.
  - The parametric hit charge is taken as the sum of all hit charges.

PhotoDetectorConfiguration string
This configuration is used to decide what digit types are loaded into RecoDigits and
used for reconstruction.  Options are (LAPPD_only, PMT_only, or All)

GetPionKaonInfo 1
Decides whether or not true pion/kaon information is loaded into the RecoEvent store.
If the MCPiK flag is to be used in the EventSelector, this must be set to 1!
```

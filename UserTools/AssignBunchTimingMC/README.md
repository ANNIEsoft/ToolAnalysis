# AssignBunchTimingMC

`AssignBunchTimingMC` applies the spill structure of the BNB to MC clusters. Currently, all MC events are "triggered" at the same time in WCSim; for certain analyses revolving around beam timing it may be necessary to have a beam-realistic simulation. The tool works by taking in a cluster produced by the `ClusterFinder` tool of form std::map<double, std::vector<MCHit>> from the CStore ("ClusterMapMC"), and applies a corrective timing factor to each event. The tool also leverages the true neutrino interaction vertex time from GENIE to accurately create a spill structure. For this reason, both the `LoadGenieEvent` and `ClusterFinder` tools must be ran beforehand. This tool is designed for use on Genie Tank and World WCSim samples. 

## Data

`AssignBunchTimingMC` produces a map of spill-adjusted cluster times and puts them into the ANNIEEvent store (all map keys are the original cluster time):

**fbunchTimes**  `std::map<double, double>`


## Configuration
```
# AssignBunchTimingMC Config File

verbosity 0

# BNB properties taken from: MicroBooNE https://doi.org/10.1103/PhysRevD.108.052010
bunchwidth 1.308       # BNB instrinic bunch spread [ns]
bunchinterval 18.936   # BNB bunch spacings [ns]
bunchcount 81          # number of BNB bunches per spill

sampletype 0           # Tank (0) or World (1) genie samples you are running over
prompttriggertime 0    # WCSim prompt trigger settings: (0 = default, t0 = 0 when a particle enters the volume)
                       #                                (1 = modified, t0 = 0 when the neutrino beam dump begins)
```


## Additional information

The "bunchTimes" have a spill structure that starts at ~0 ns and extends to M ns (depending on the bunch spacing and number of bunches). The tool is currently configured to the most recent Genie sample production (tank: 2023, world: early 2024) for both the WCSim tank and world events (both of which have different "beam dump" starting times and prompt trigger times).

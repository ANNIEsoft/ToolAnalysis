# PrintDQ

`PrintDQ` is a quick, easy to use data quality (DQ) tool that prints out run statistics. It's to be used over the beam ProcessedData created by the event building toolchain. This tool works by loading in `ANNIEEvent`, `RecoEvent`, `ClusterMap`, and `MrdTimeClusters` information, counting and calculating rates based on how many events fit various selection criteria. `PrintDQ` accesses PMT/MRD clusters, MRD tracks, and BRF fitting results, so it is necessary to run the following tools beforehand to ensure it executes properly:

- `LoadANNIEEvent`
- `LoadGeometry`
- `TimeClustering`
- `FindMRDTracks`
- `ClusterFinder`
- `ClusterClassifiers`
- `EventSelector`
- `FitRWMWaveform` (new, PR #289)

## Data

`PrintDQ` currently just prints out the run statistics and does not add anything to the Store. Here's an example of the print output from the tool:
```
**************************************
Run 4314
**************************************
 
total events:         6336
has LAPPD data:       121    (1.90972% +/- 0.175261%)
has BRF fit:          6324    (99.8106% +/- 1.77415%)
eventTimeTank = 0:    29    (0.457702% +/- 0.0851874%)
beam_ok:              5018    (79.1982% +/- 1.49664%)
total clusters:       279    (0.0440341 +/- 0.00269367)
prompt clusters:      273    (97.8495% +/- 8.32999%)
spill clusters:       236    (84.5878% +/- 7.48089%)
ext clusters:         6    (2.15054% +/- 0.887344%)
extended (CC):        0    (0% +/- 0%)
extended (NC):        31    (0.489268% +/- 0.0880898%)
Tank+MRD coinc:       14    (0.22096% +/- 0.0591191%)
1 MRD track:          5    (0.0789141% +/- 0.0353054%)
Tank+Veto coinc:      30    (0.473485% +/- 0.0866505%)
Tank+MRD+Veto coinc:  4    (0.0631313% +/- 0.0315756%)
```

- `total events`         = total events with an undelayed beam trigger (14)
- `has LAPPD data`       = events where the Data Steams contains LAPPDs (% given out of total undelayed beam trigger events)
- `has BRF fit`          = events where the BRF auxiliary waveform was successfully fit (% given out of total undelayed beam trigger events)
- `eventTimeTank`        = events that have no timestamps for their ADC waveform for some reason (% given out of total undelayed beam trigger events)
- `beam_ok`              = events with "good" beam conditions, as extracted by the BeamFetcherV2 tool (% given out of total undelayed beam trigger events). Good beam conditions are defined as: (0.5e12 < pot < 8e12), (172 < horn val < 176), (downstream/upstream toroids within 5%)
- `total clusters`       = total number of clusters found in the events with an undelayed beam trigger (rate given as clusters per event)
- `prompt clusters`      = total number of clusters found in the prompt 2us window (% given out of the total number of clusters)
- `spill clusters`       = total number of clusters found in the beam spill: 190:1750ns (% given out of the total number of clusters)
- `ext clusters`         = total number of clusters found in the extended window: >2us (% given out of the total number of clusters)
- `extended (CC)`        = total number of events with an extended window, CC/charge-based readout (% given out of total undelayed beam trigger events)
- `extended (NC)`        = total number of events with an extended window, NC/random/forced readout (% given out of total undelayed beam trigger events)
- `Tank+MRD coinc`       = total number of events with Tank + MRD coincidence (% given out of total undelayed beam trigger events)  - coincidence defined in the `EventSelector` tool.
- `1 MRD Track`          = total events with at least one instance of a single MRD track for a given MRD cluster (% given out of total undelayed beam trigger events)
- `Tank+Veto coinc`      = total number of events with Tank + Veto coincidence (% given out of total undelayed beam trigger events)  - coincidence defined in the `EventSelector` tool.
- `Tank+MRD+Veto coinc`  = total number of events with Tank + MRD + Veto coincidence (% given out of total undelayed beam trigger events)  - coincidences defined in the `EventSelector` tool.

## Configuration
```
# PrintDQ Config File

verbosity 0
```

## Additional information

This tool is meant for quick feedback on beam runs. Various statistics will be blank/missing if ran over source runs (but the tool should still work). The `LoadANNIEEvent` tool (to be run beforehand) loads in the ProcessedData via a `my_inputs.txt` file - make sure to populate this list with a single run's worth of part files. The tool is intended to be used over just one run at a time to provide an accurate snapshot of the run's conditions. This tool within a suitable toolchain may take several minutes depending on how many part files are present in the run (and on verbosity levels).

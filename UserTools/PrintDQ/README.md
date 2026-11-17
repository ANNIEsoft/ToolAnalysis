# PrintDQ

`PrintDQ` is a quick, easy to use data quality (DQ) tool that prints out run statistics. It's to be used over the beam ProcessedData created by the event building toolchain. This tool works by loading in `ANNIEEvent`, `RecoEvent`, `ClusterMap`, and `MrdTimeClusters` information, counting and calculating rates based on how many events fit various selection criteria. `PrintDQ` accesses PMT/MRD clusters, MRD tracks, LAPPD PPS information and RWM/BRF/BES fitting results, so it is necessary to run the following tools beforehand to ensure it executes properly:

- `LoadANNIEEvent`
- `LoadGeometry`
- `TimeClustering`
- `FindMRDTracks`
- `ClusterFinder`
- `ClusterClassifiers`
- `EventSelector`
- `LAPPDLoadStore`
- `FitRWMWaveform` (new, PR #289)

## Data

`PrintDQ` prints out the run statistics and does not add anything to the Store. It also populates a .csv file with the same statistics (`R<run>_PrintDQ.csv`). Here's an example of the print output from the tool:
```
**************************************
Run 6356
**************************************

total events:         1747
has LAPPD data:       74    (4.23583% +/- 0.502726%)
has Tank data:        1718    (98.34% +/- 3.34136%)
has RWM fit:          1718    (100% +/- 3.41196%)
has BRF fit:          1718    (100% +/- 3.41196%)
has BES fit:          1718    (100% +/- 3.41196%)
eventTimeTank = 0:    29    (1.65999% +/- 0.3108%)
beam_ok:              1744    (99.8283% +/- 3.37916%)
total clusters:       220    (0.12593 +/- 0.00900895)
prompt clusters:      122    (55.4545% +/- 6.25978%)
spill clusters:       107    (48.6364% +/- 5.73234%)
ext clusters:         98    (44.5455% +/- 5.40994%)
extended (CC):        26    (1.48827% +/- 0.294037%)
extended (NC):        8    (0.457928% +/- 0.162272%)
Tank+MRD coinc:       10    (0.57241% +/- 0.181529%)
1 MRD track:          6    (0.343446% +/- 0.140452%)
Tank+Veto coinc:      11    (0.629651% +/- 0.190444%)
Tank+MRD+Veto coinc:  3    (0.171723% +/- 0.0992294%)
Pass PPS quality:     49    (66.2162% +/- 12.1956%)
```

- `total events`         = total events with an undelayed beam trigger (14)
- `has LAPPD data`       = events where the Data Streams contains LAPPDs (% given out of total undelayed beam trigger events)
- `has Tank data`        = events where the Data Streams contains Tank (PMT) data (% given out of total undelayed beam trigger events)
- `has RWM fit`          = events with a full RWM fit, i.e. RWMFHWM > 0: both rising and falling edges found (% given out of events with Tank data)
- `has BRF fit`          = events with a BRF fit, i.e. BRFFirstPeakFit != 0 (% given out of events with Tank data)
- `has BES fit`          = events with a BES fit, i.e. BESwidth > 0 (% given out of events with Tank data). Always 0 before run 5870 (BES not recorded).
- `eventTimeTank`        = events that have no timestamps for their ADC waveform for some reason (% given out of total undelayed beam trigger events)
- `beam_ok`              = events with "good" beam conditions, as extracted by the BeamFetcherV2 tool (% given out of total undelayed beam trigger events). Good beam conditions are defined as: (0.5e12 < pot < 8e12), (172 < horn val < 176), (downstream/upstream toroids within 5%)
- `total clusters`       = total number of clusters found in the events with an undelayed beam trigger (rate given as clusters per event)
- `prompt clusters`      = total number of clusters found in the prompt 2us window (% given out of the total number of clusters)
- `spill clusters`       = total number of clusters found in the beam spill: 190:1750ns (% given out of the total number of clusters)
- `ext clusters`         = total number of clusters found in the extended window: >2us (% given out of the total number of clusters)
- `extended (CC)`        = total number of events with an extended window, CC/charge-based readout (% given out of total undelayed beam trigger events)
- `extended (NC)`        = total number of events with an extended window, NC/random/forced readout (% given out of total undelayed beam trigger events)
- `Tank+MRD coinc`       = total number of events with Tank + MRD coincidence (% given out of total undelayed beam trigger events)  - coincidence defined in the `EventSelector` tool.
- `1 MRD Track`          = total events with at least one MRD cluster that has exactly one MRD track (% given out of total undelayed beam trigger events)
- `Tank+Veto coinc`      = total number of events with Tank + Veto coincidence (% given out of total undelayed beam trigger events)  - coincidence defined in the `EventSelector` tool.
- `Tank+MRD+Veto coinc`  = total number of events with Tank + MRD + Veto coincidence (% given out of total undelayed beam trigger events)  - coincidences defined in the `EventSelector` tool.
- `Pass PPS quality`     = events with LAPPD data where no LAPPD is missing PPS ticks on both boards (% given out of events with LAPPD data)

## Configuration
```
# PrintDQ Config File

verbosity 0
```

## Additional information

This tool is meant for quick feedback on beam runs. Various statistics will be blank/missing if ran over source runs (but the tool should still work). The `LoadANNIEEvent` tool (to be run beforehand) loads in the ProcessedData via a `my_inputs.txt` file - make sure to populate this list with a single run's worth of part files. The tool is intended to be used over just one run at a time to provide an accurate snapshot of the run's conditions. This tool within a suitable toolchain may take several minutes depending on how many part files are present in the run (and on verbosity levels).

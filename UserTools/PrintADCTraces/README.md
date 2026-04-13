# PrintADCTraces

`PrintADCTraces` will read hit information (either from the "ClusterMap" or "Hits" from the "ANNIEEvent"), take the stored ADC traces in "RecoADCData", and output the traces to a root file. It will also filter traces/hits within some range of charges or times provided by the user.

The tool is also useful to assemble metadata of data pulses, such as the calculated baseline and noise. This data (along with the shape of the ADC traces) can be used to analyze the stability of the PMTs over time.

This tool does not put anything into the store.

## Output rootfile

`PrintADCTraces` produces a root file containing directories for each PMT channel - within each channel will be TGraphs for each ADC trace:

```
// ROOT file:
// ├── chankey/                # directory for each PMT channel
// ├── 332/
// │   ├── wf1 TGraph          # for each pulse, a TGraph
// │   └── wf2 TGraph
// ├── 463/
// │   ├── wf1 TGraph
// │   └── ...
// └── TraceSummary TTree      # metadata
//     ├── chan                # all pulse channel ids
//     ├── run                 # all pulse run numbers 
//     ├── eventTime           # all pulse event times
//     ├── hitT                # all pulse hit times [ns]
//     ├── hitPE               # all pulse hit charges [pe]
//     ├── hitBaseline         # all pulse baselines [adc]
//     └── hitNoise            # all pulse baseline sigma (noise) [adc]
```

where the TGraph name is given by: ```<run_number>_<p_file>_<eventtimetank>_<hitT>_<hitPE>```


## Configuration
```
# PrintADCTraces Config File

verbosity 0

hitPE_min 0.5                   # minimum hit charge [pe] to include in the root file
hitPE_max 1.5                   # maximum hit charge [pe] to include in the root file
                                    # omitting both of these will not impose a charge selection

hitT_min 0                      # minimum hit time [ns] to include in the root file
hitT_max 2000                   # maximum hit time [ns] to include in the root file
                                   # omitting both of these will not impose a time selection

MaxTraces 10000                 # maximum number of traces to include in the root file (default is 10,000 if not set by user)
                                   # if set to '0', no limit will be imposed

MaxTracesPerChannel 100         # maximum number of traces per channel to include in the root file
                                   # if set to '0' or left undefined, it will not impose a limit

useClusterHits 0                # fill root file with traces from clustered hits rather than from all event "Hits"

OutputFilename ADCTraces.root   # name of the output root file
```

## Example toolchain
```
LoadANNIEEvent
LoadGeometry
ClusterFinder   (only if you are using the clustered hits)
PrintADCTraces
```

## Additional information
- ADC traces will be in ADC (y) vs time [ns] (x). All pulse times will be relative (and zeroed) to the start time of the pulse. All pulse amplitudes (y) are baseline-subtracted. This way its easier to compare. You can cross reference the pulse features with the title (which contains the charge and time of the pulse) to determine more information.
- This tool is similar to `PrintADCData`, but instead of printing out all raw PMT waveforms, it only prints out the found pulse traces (and provides more identifying / filtering information based on charge / time).
- Runtime is ~30s for 10 part files (about the same whether or not `ClusterFinder` is included). 
- Filesize for the output root file is ~0.05MB per waveform per channel. If including 100 waveforms per PMT channel(`MaxTracesPerChannel 100`) the filesize is ~5MB, if 1000 the filesize is ~50MB.

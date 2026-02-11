# LAPPDWaveformDisplay

The `LAPPDWaveformDisplay` tool visualizes LAPPD waveforms for each event and generates a PDF file. Each page of the PDF contains four plots: the two plots on the left correspond to one end of the striplines, labeled side 0, and the two plots on the right correspond to the other end, labeled side 1. The bottom plots show the time-amplitude relationship for each channel, while the top plots are 2D histograms with time on the x-axis, strip number on the y-axis, and amplitude represented by color.

Each side consists of 30 channels, allowing a clear visualization of the waveform data on both sides of the stripline.

This tool can be used in several LAPPD-related toolchains to visualize and track waveform modifications. Many tools within these toolchains typically modify the waveform (e.g., time shifting, baseline subtraction) and save it under a new label in the `ANNIEEvent BoostStore`. The next tool uses this modified waveform, applies further changes, and saves it again with a new label. This tool is useful for visualizing the waveform at any stage of this process to track changes. It simply takes a labeled waveform as input and generates a PDF file on an event-by-event basis.

## Requirements

Before running the LAPPDWaveformDisplay, at least two tools must be executed. The first required tool is `LoadGeometry`, which is essential for LAPPD channel mapping. The second required tool is responsible for reading waveforms from a data file. These tools ensure that the necessary geometry and waveform data are available before visualization.

## Configuration

`LAPPDWaveformDisplay` has the following configuration parameters/variables:

Most parameters are self-explanatory by their names. However, special attention should be given to the ChannelOffset parameter. This offset is used to adjust the channel numbering in the waveform labels to match the convention defined in the Channel class. In ToolAnalysis, the data model Channel class assigns channel numbers starting from 1000 for LAPPDs. Therefore, the offset should typically be set to 1000 to ensure consistency. However, if the waveform label already uses channel numbers starting from 1000, then the offset should be set to 0.

```
# To display specific events, uncomment and set the range.
#FirstEventNumber 53
#LastEventNumber 85

InputWaveformLabel ABLSLAPPDData
ChannelOffset 0
TrigerChannel1 1005
TrigerChannel2 1035
WaveformDisplayVerbosity 0
RequireT0Signal 0
DisplayTriggerChannelInPlot 1
OutputFileName LAPPDWaveformDisplay.pdf
```

## LAPPD-Laser Setup Parameters

The following parameters are related to the LAPPD-laser setup in the dark room at Fermilab Lab 6. If this is not relevant to you, feel free to skip this part. It is a common practice among researchers working in the dark room to frequently visualize waveforms after data taking. If you want to provide the following information, set PrintRunInfo to 1.

The provided information will appear on the first page of the generated PDF file.

```
# If PrintRunInfo is set to 0, the other settings below it are ignored, and the run information is not printed. 
PrintRunInfo 0

Date 2025-03-05
LAPPDModel 151
Path /pnfs/annie/persistent/LAPPDData/LAPPD151/2025-03-12/all.txt
LaserStatus On
TriggerMode Forced
NDFilter 2.0
RunInfo "Cross-talk-data"
```

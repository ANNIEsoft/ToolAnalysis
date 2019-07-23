# LAPPDSim
LAPPDSim is used for the digital model of the LAPPDs. Currently the tool uses two additional classes, names LAPPDresponse and LAPPDDisplay. The LAPPDresponse class is used for the simulation of the waveforms, which result from hits on a LAPPD. The LAPPDDisplay class is used to create histograms for the verification of the functionality of the tool. The tool is currently able to save and display the following histograms:
MC hits on all LAPPDs for every event (x-axis: radius of the tank, y-axis: height of the tank, z-axis or colour code: arrival time)
MC hits for every LAPPD (x-axis: transverse coordinate, y-axis: parallel coordinate, z-axis: arrival time)
MC hits for every LAPPD (x-axis: time, y-axis: transverse coordinate, z-axis: number of events
MC hits for every LAPPD (x-axis: time, y-axis: parallel coordinate, z-axis: number of events
Waveform for every LAPPD left side and right side (x-axis: time, y-axis: strip number, z-axis: voltage)
Waveform for every strip of every LAPPD left and right side(x-axis: time, y-axis: voltage)
One can decide, whether histograms should only be written or displayed directly while the program is running. After each event one needs then to press a key to look into the next event.

## Data
This tool uses the following data:
**Geometry** `Geometry`
This is needed for the names of the histograms and for the storage of the waveforms

**MCLAPPDHits** `std::map<unsigned long, std::vector<MCLAPPDHit> >*`
This is needed for the creation of the waveforms and for the histograms.

This tool produces the following data:
**LAPPDWaveforms** `std::map<unsigned long, Waveform<double> >`
The waveform together with the corresponding channelkey is saved.

## Configuration
It is to note that one might need to adjust the inline number in the ToolChainConfig.

PathToPulsecharacteristics /nashome/m/mstender/ToolAnalysisForFelix/ToolAnalysis/UserTools/LAPPDSim/pulsecharacteristics.root #This is the path to the pulsecharacteristics.root file, which must be set before running the tool
EventDisplay 2 #0 = no event display; 1 = histograms will be written, but not displayed; 2 histograms will be displayed and written
OutputFile /nashome/m/mstender/ToolAnalysisForFelix/ToolAnalysis/LAPPDHistograms.root #This is the path to the output file. This must be also set befor running the tool.
ArtificialEvent 1 #1 = artificial Events will be used; 0 = MC Events will be used;

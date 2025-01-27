# AmBeWaveform

***********************
## Description
**********************

The `AmBeWaveform` toolchain produces a ROOT file containing all raw AmBe PMT waveforms.

For AmBe source runs, there is a small PMT within the AmBe housing that records the initial scintillation light within the BGO crystal and acts as the primary trigger for the rest of the PMTs. The small AmBe waveforms are stored in the [RWM auxillary channel](https://github.com/S81D/ToolAnalysis/blob/005d0930b57a266b9d8f4c1ef3ad07f31850e871/configfiles/LoadGeometry/AuxChannels.csv#L6). This waveform can be used to reduce backgrounds in the AmBe neutron analysis.

Currently, the `FitRWMWaveform` tool which does the heavy lifting for this toolchain is designed for beam runs and the fitting / storing of both the Booster RWM and RF waveforms. This toolchain will therefore populate the output root file with **both** Booster RWM and Booster RF raw waveforms (Booster RF waveforms will be useless for the AmBe source runs). TO DO: Adjust `FitRWMWaveform` tool to handle and fit small AmBe PMT waveforms and send the information to the store for use by other tools.


************************
## Tools
************************

The toolchain consists of the following tools:

```
LoadANNIEEvent
LoadGeometry
FitRWMWaveform
```


************************
## Additional information
************************

### FitRWMWaveform Config
```
verbosityFitRWMWaveform 0
maxPrintNumber 20000           # controls how many waveforms are written to the root file
printToRootFile 1              # whether to print the raw waveforms to the root file
```
For a 20 part file sample of an AmBe run, there will be ~7000 waveforms recorded in the Booster RWM channel. As stated above, the current iteration of the tool includes the Booster RF channel as well, so for 20 part files we can expect ~14,000 total waveforms. As a result, it is recommended `printToRootFile` is set to 20000 per 20 part files.


### Usage
The toolchain runs quickly (few seconds per part file), but when running over 20 part files as mentioned above the root file quickly explodes in size. For 20 part files, the root file size is ~700 MB. Until the tool is adjusted, the current solution is to use a bash script to run the toolchain in 20 part file chunks, then offload the root files from `/exp/annie/app/users/` to `scratch/` or `persistent/` to avoid overloading your app area.

You can run this bash script via: `sh generate_AmBe_waveforms.sh <run_number>`. Please make the appropriate changes to the bash file prior to running it (like user name, ToolAnalysis directoruy path, etc...).

# PrintADCData

PrintADCData

This tool is used to analyze properties in an ANNIEEvent booststore's
RecoADCHits vector.  The tool looks at the raw and calibrated waveforms
for Tank PMTs and outputs information including:
  - Example raw waveforms from all channels
  - The number of waveforms collected for each channel
  - The number of pulses collected for each channel 
  - The number of waveforms with at least one pulse for each channel
  - 2D histograms of the hit occupancy in y vs. phi space

## Data

Describe any data formats PrintADCData creates, destroys, changes, or analyzes. E.G.

Two files are produced after running this tool:
  - A ROOT file with raw waveforms and the 2D hit occupancy histograms
  - A text file with the waveform/pulse information described above

## Configuration

Describe any configuration variables for PrintADCData.

```

UseLEDWaveforms [int]
Specifies whether to show and save full waveforms from the DAQ, or 
the LED waveform windows produced from running 
PhaseIIADCCalibrator with MakeLEDWaveforms set at 1.  
1=Show/save LED waveforms, 
0= Show/save raw waveforms.

verbosity [int]
controls the amount of print output (0- lowest, 5-highest)

OutputFile [string]
Base filename given to both the ROOT and text file that are output from the tool.


SaveWaveforms [bool]
Determines whether or not to save raw waveforms in the output ROOT file.  
1 = yes, 0 = no

WavesWithPulsesOnly [bool]
Controls whether or not to save only waveforms that have a pulse in them.  Whether
or not the wave has a pulse is determined by logic output by the PhaseIIADCHitFinder
tool.

MaxWaveforms [int]
Controls the number of raw waveform examples saved to the output ROOT file.  
Used to keep the ROOT file from exploding under the sheer number of raw 
waveforms saved.

LEDsUsed [string]
Log string saved in the output text file to record what LEDs were used for the 
run being processed.  Will eventually be automated when the DAQ can control 
the LEDs.

LEDSetpoints [string]
Log string saved in the output text file to record what LED setpoints were used
 for the run being processed.  Will eventually be automated when the DAQ 
can control the LEDs.

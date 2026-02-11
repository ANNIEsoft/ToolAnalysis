# EBPMT

EBPMT tool is a part of Event Building version 2 tool chain.
For reference slides, see:
https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5633
EBPMT match the ADC timestamps to grouped trigger, and save the matching results to CStore for EBSaver.


## Data

**FinishedHits**
The PMT hits on each PMT are decoded from the PMTDataDecoder. While there are enough number of PMT loaded for a (ADC) PMT timestamp, the timestamp will be pushed to FinishedHits. The name was taken from ANNIEEventBuilder.


**RWMRawWaveforms**
**BRFRawWaveforms**
These two behaves like FinishedHits, but for raw RWM and BRF waveforms. 
The slides for BRF and RWM waveforms: https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5756


**PairedCTCTimeStamps**
After matching, the matched trigger timestamp will be saved here. The key is the main trigger word for each run type.

**PairedPMTTimeStamps**
After matching, the matched PMT timestamp will be saved here. The key is the main trigger word for each run type.
This and PairedCTCTimeStamps have the same index. A little bit dangerous, but overall works well.



## Configuration

**matchTargetTrigger** 
This gives which trigger word that the PMT timestamps should be matched to.

**matchTolerance_ns**
This gives the maximum allowed time tolerance between the PMT timestmap and the target trigger timestamp.

**exePerMatch** 
This gives how many loops need to be past for one matching between PMT timestmaps and target trigger timestamps.
500 is generally fine with beam runs

**matchToAllTriggers**
1 or 0. 1 means match PMT timestamps to all possible triggers, 0 means only match to the target trigger.

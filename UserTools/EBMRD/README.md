# EBMRD

EBMRD tool is a part of Event Building version 2 tool chain.
For reference slides, see:
https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5633

EBMRD match the MRD timestamp to grouped trigger, and save the matching results to CStore for EBSaver.

## Data


**PairedCTCTimeStamps**
After matching, the matched trigger timestamp will be saved here. The key is the main trigger word for each run type.
Saved as PairedMRDTriggerTimestamp in CStore.

**PairedMRDTimeStamps**
After matching, the matched MRD timestamp will be saved here. The key is the main trigger word for each run type.
This and PairedCTCTimeStamps have the same index. A little bit dangerous, but overall works well.
Saved as PairedMRDTimeStamps in CStore


## Configuration

**matchTargetTrigger** 
This gives which trigger word that the MRD timestamps should be matched to.

**matchTolerance_ns**
This gives the maximum allowed time tolerance between the MRD timestmap and the target trigger timestamp.

**exePerMatch** 
This gives how many loops need to be past for one matching between MRD timestmaps and target trigger timestamps.
500 is generally fine with beam runs. 100 would be better for AmBe runs

**matchToAllTriggers**
1 or 0. 1 means match MRD timestamps to all possible triggers, 0 means only match to the target trigger.

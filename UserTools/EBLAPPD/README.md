# EBLAPPD

EBLAPPD tool is a part of Event Building version 2 tool chain.
For reference slides, see:
https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5633

EBLAPPD match the LAPPD beamgate + LAPPD offset to grouped trigger, and save the matching results to CStore for EBSaver.

## Data

**PairedCTCTimeStamps**
After matching, the matched trigger timestamp will be saved here. The key is the main trigger word for each run type.
Saved as PairedLAPPDTriggerTimestamp in CStore.

**PairedLAPPDTimeStamps**
After matching, the matched LAPPD beamgate + offset will be saved here. The key is the main trigger word for each run type.
This and PairedCTCTimeStamps have the same index. A little bit dangerous, but overall works well.
Saved as PairedLAPPDTimeStamps in CStore


## Configuration

**matchTargetTrigger** 
This gives which trigger word that the LAPPD beamgate + offset should be matched to.

**matchTolerance_ns**
This gives the maximum allowed time tolerance between the LAPPD beamgate + offset and the target trigger timestamp.

**exePerMatch** 
This gives how many loops need to be past for one matching between MLAPPD beamgate + offset and target trigger timestamps.
500 is generally fine with beam runs. 

**matchToAllTriggers**
1 or 0. 1 means match to all possible triggers, 0 means only match to the target trigger.


# EBTriggerGrouper

EBTriggerGrouper is a part of the Event Builder V2 tool chain.
For reference slides, see:
https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5633


This tool take TimeToTriggerWordMap from the trigger data decoder as input, push the triggers and trigger words into TrigTimeForGroup and TrigWordForGroup, then use the pre defined trigger groups like BeamTriggers to group the triggers based on time tolorance. Finally save the grouped triggers to GroupedTriggersInTotal in CStore.

## Data

**TimeToTriggerWordMap** `std::map<uint64_t, std::vector<uint32_t>>`
**TimeToTriggerWordMapComplete** `std::map<uint64_t, std::vector<uint32_t>>`
Take from TriggerDataDecoder. This tool will push the info to 

**TrigTimeForGroup** `vector<uint64_t>`
**TrigWordForGroup** `vector<uint32_t>`
Buffer for timestamps. Only be used in this tool.


**GroupedTriggersInTotal** `std::map<int, std::vector<std::map<uint64_t, uint32_t>>>`
Groupped triggers, will be passed to other tools for matching.

## Configuration
For one group type, you need one option, and two configs from the config file, one config hard coded in the cpp file.
groupBeam: tell the tool group this run type or not
BeamTriggerMain: the main trigger of a run type. For instance, main beam trigger is 14
BeamTolerance: the time tolerance that you allow other triggers in this run type to be paired with a main trigger.
BeamTriggers: hard coded in the cpp file for safety. This vector contains all trigger word that allowed to be grouped as beam.
They are sent to GroupByTrigWord function for each run type. In case there will be more run types in the future.



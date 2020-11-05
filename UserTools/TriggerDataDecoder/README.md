# TriggerDataDecoder

TriggerDataDecoder

This tool decodes TriggerData class entries and passes the trigger data timestamp and 
word downstream via the CStore.

## Data

Describe any data formats TriggerDataDecoder creates, destroys, changes, or analyzes. E.G.

std::map<uint64_t,uint32_t>\* TimeToTriggerWordMap; Pointer to a map with key,value pair...
key: uint64_t timestamp, value: uint32_t trigger word.

The trigger word tells you what kind of trigger the timestamp is associated with.

## Configuration


```
verbosity (int)
Sets the print out level.  0 is silent, 5 is the most verbose

TriggerMaskFile (string)
Path to a file that lists the trigger words to place in the TimeToTriggerWordMap.  Each
trigger word should be placed on a separate line in the file.  Any line starting with # is
ignored by the processor.
```

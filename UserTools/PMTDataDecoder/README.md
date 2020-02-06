# PMTDataDecoder

PMTDataDecoder

The PMTDataEncoder is a tool used to process PMT waveforms in raw data files.  These
waveforms are saved to the CStore for further processing/event building downstream.


The philosophy behind the first implementation of the Event Builder is to use maps at 
several stages in the event building (Unprocessed, partially processed, fully processed
PMT records).

Once a full event's PMT information has been parsed, it's moved to a second level of
maps that hold the "completed" hit information.  Once this map has the information 
related to a full event, The CardData is matched to the TriggerData to build an 
ANNIEEvent.

#####Maps used in the tool#####

#Maps used to determine what CardData to process next#
std::map<int, int> SequenceMap;  //Key is CardID, Value is what sequence # is next
std::map<int, std::vector<int>> UnprocessedEntries; //Key is CardID, Value is vector of boost entry #s with an unprocessed entry

#Maps used in decoding frames; specifically, holds record header and record waveform info#
#as waveforms are built#

std::map<std::vector<int>, int> TriggerTimeBank;  //Key: {cardID, channelID}. Value: trigger time associated with wave in WaveBank 
std::map<std::vector<int>, std::vector<uint16_t>> WaveBank;  //Key: {cardID, channelID}.  If you're in sequence, the MTCTime doesn't matter for mapping

#Maps that store completed waveforms from cards#
std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank ## Data

  Input: Raw data files.

  Output: FinishedWaves [map] is saved to CStore.


## Configuration

Describe any configuration variables for PMTDataDecoder.

verbosity (int)
    Integer that controls the level of log output shown when running.

Mode (string)
    Controls whether tool runs assuming a single file is input or if the tool
    will be run continously and searching for files in the data stream.
    Currently either "FileList" or "SingleFile" are available.

InputFile (string)
    In "FileList" mode, String defining the path to a raw data file to process.  
    In "SingleFile" mode, points to a single RawData booststore.

EntriesPerExecute (int)
    Controls how many entries in the PMTData BoostStore are searched and 
    decoded per Execute loop.  The idea behind this configurable is to adjust
    how often the downstream Builder tools are accessed.  For example, 
    ANNIEEventBuilder checks if any clock times have all the PMT waveforms and
    builds an ANNIEEvent boost store if so.  Going to this tool after every
    PMTData entry parsing may be a waste of resources.

    if set to <=0, the entire PMTData BoostStore will be processed in a single 
    execution of the tool.

```
  Example of what you may want for a default config file:

  verbosity 2
  InputFile /path/to/VMEDataFile 
  LockStep 0
  CardDataEntriesPerExecute 5
```

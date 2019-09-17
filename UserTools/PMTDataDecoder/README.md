# PMTDataDecoder

PMTDataDecoder

The DataEncoder is a tool used to process Raw data files and ultimately convert them to
ANNIEEvent stores.


The philosophy behind the first implementation of the Event Builder is to use maps at 
several levels in the event building to hold information as the raw binary is parsed.  
Once a full event's PMT information has been parsed, it's moved to a second level of
maps that hold the "completed" hit information.  Once this map has the information 
related to a full event, The CardData is matched to the TriggerData to build an 
ANNIEEvent.


Maps for keeping track of what CardData classes have been, or need to, be processed
std::map<int, int> SequenceMap;  //Key is CardID, Value is what sequence # is next
std::map<int, std::vector<int>> UnprocessedEntries; //Key is CardID, Value is vector of boost entry #s with an unprocessed entry

Maps used in decoding frames; specifically, holds record header and record waveform info
std::map<std::vector<int>, int> TriggerTimeBank;  //Key: {cardID, channelID}. Value: trigger time associated with wave in WaveBank 
std::map<std::vector<int>, std::vector<uint16_t>> WaveBank;  //Key: {cardID, channelID}.  If you're in sequence, the MTCTime doesn't matter for mapping

Maps that store completed waveforms from cards
std::map<int, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank

## Data

  Input: Raw data files.

  Appends values to the ANNIEEvent store.  Ooh... Maybe an execute loop be written in a way where there's one execution per filling of ANNIEEvent.

## Configuration

Describe any configuration variables for PMTDataDecoder.

verbosity (int)
    Integer that controls the level of log output shown when running.

InputFile (string)
    String defining the path to a raw data file to process.

LockStep (bool)
    Indicates whether events are processed assuming the detector was being run
    in lock-step or not.  For single file processing, this will be set to false. 

CardDataEntriesPerExecute (int)
    Controls how many entries in the PMTData BoostStore are searched and 
    decoded per Execute loop.  The idea behind this configurable is to adjust
    how often the downstream Builder tools are accessed.  For example, 
    ANNIEEventBuilder checks if any clock times have all the PMT waveforms and
    builds an ANNIEEvent boost store if so.  Going to this tool after every
    PMTData entry parsing may be a waste of resources.

```
  verbosity 2
  InputFile /path/to/VMEDataFile 
  LockStep 0
```

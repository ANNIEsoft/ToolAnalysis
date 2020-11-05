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

#Pointers to data loaded from tools upstream
BoostStore\* PMTData: Pointer to a PMTData BoostStore (Used in Monitoring case only)
std::vector<CardData>\* Cdata; Pointer to a vector of CardData classes

#Maps used to determine what CardData to process next#
std::map<int, int> SequenceMap;  //Key is CardID, Value is what sequence # is next
std::map<int, std::vector<int>> UnprocessedEntries; //Key is CardID, Value is vector of boost entry #s with an unprocessed entry

#Maps used in decoding frames; specifically, holds record header and record waveform info#
#as waveforms are built#
std::map<std::vector<int>, int> TriggerTimeBank;  //Key: {cardID, channelID}. Value: trigger time associated with wave in WaveBank 
std::map<std::vector<int>, std::vector<uint16_t>> WaveBank;  //Key: {cardID, channelID}.  If you're in sequence, the MTCTime doesn't matter for mapping

#Maps that store completed waveforms from cards#
std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank ## Data
//A pointer to FinishedWaves is set in the CStore for tools to access downstream (InProgressTankEvents key in CStore)

std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > CStorePMTWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank ## Data
//This is just a copy of FinishedWaves, and is stored (FinishedPMTWaves key in CStore)
#THE ABOVE MAPS COULD PROBABLY BE COMBINED TO JUST ONE ENTRY IN THE CSTORE
#BUT DOWNSTREAM TOOLS WOULD HAVE TO BE CHANGED TO ALL ACCESS A POINTER OR THE ACTUAL DATA
#IT'S A TODO, BUT THINGS WORK FOR NOW

## Configuration

Describe any configuration variables for PMTDataDecoder.

verbosity (int)
    Integer that controls the level of log output shown when running.

Mode (string)
    Controls whether PMTData is loaded from the CStore filled by the LoadRawData
    tool (offline) or accessed from the CStore as filled by the DAQ (monitoring).

ADCCountsToBuildWaves (int)
    Minimum number of ADC counts in a wave required before moving it to the 
    FinishedWaves map. This is generally left as 0 such that complete waves 
    of any ADC length are saved for event building; however, it could be set to
    a threshold where only the long waveform acquisitions (~65 microseconds) are 
    saved for building downstream (a good threshold for this would be 3000 ADC counts). 

EntriesPerExecute (int)
    Number of CardData entries accessed per execution loop (Mode Monitoring only).

```
  Example of what you may want for a default config file in Offline mode:
  verbosity 2
  Mode Offline 
  ADCCountsToBuildWaves 0

  Example of what you may want for a default config file in Monitoring mode:
  verbosity 2
  Mode Monitoring
  ADCCountsToBuildWaves 0
  EntriesPerExecute -1
```

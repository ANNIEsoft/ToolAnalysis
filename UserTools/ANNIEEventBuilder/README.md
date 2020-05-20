# ANNIEEventBuilder

ANNIEEventBuilder

The ANNIEEventBuilder takes parsed MRD,Trigger, and PMT data from the 
MRDDataDecoder, TriggerDataDecoder, PMTDataDecoder tools and pairs the data 
into ANNIEEvent BoostStores based on timestamps.

The ANNIEEventBuilder has three key structs used throughout the event building process.

Struct TimeStream;
This struct holds vector of timestamps for each datastream (MRD, PMT, and CTC).  Entries in the vectors are 
ordered chronologically (earliest time at the first index).  These timestreams are used to pair data from each
timestream for building into an ANNIEEvent.

Struct Orphanage;
This struct holds either PMT timestamps associated with PMT data that never finished building all of it's waveforms
(maybe data was lost due to a FIFO overflow) or data from all three streams that was never successfully paired 
for building an ANNIEEvent.  Currently, these timestamps are used to delete orphans.  Eventually, more sophisticated
logic for attempting to merge Orphans together is needed.

Struct MRDEventMaps;
This struct holds all the maps that are accessed from the CStores built by the MRDDataDecoder tool.  Each contain key-value
pairs where the key is the MRD timestamp and the value is the data of interest (hit information, if the event has a 
beam or cosmic loopback hit, and the MRDTriggerType).

##BuildTypes Tank and MRD are simple.  They take any fully built Tank and MRD data and push them into ANNIEEvents.##

##TankAndMRD and TankAndMRDAndCTC BuildTypes are a bit more complex.##

TankAndMRD: Timestamps in the Tank and MRD data streams are paired together if 
they are within a tolerance set in the configurables.  Pairs are placed into the 
BeamTankMRDPairs map; see the PairTankPMTAndMRDTriggers() method.  
Tank PMT waveforms and MRD paddle hit info. related to these
pairs are then pushed into ANNIEEvent BoostStores.

TankAndMRDAndCTC: 
First, MRD data with a cosmic hit are paired with CTC timestamps containing the
CR trigger word (hard-coded as 35+1; see PairCTCCosmicPairs() method) and added
to the BuildMap for event building. Then,
CTC timestamps are individually paired with the Tank and MRD 
data stream timestamps using the time tolerances set in the configurables
(see the MergeStreams() method).
If a CTC timestamp pairs with BOTH a Tank and MRD timestamp or a Tank timestamp alone,
the CTC/PMT/MRD data are all put into the BuildMap object.  The BuildMap object is then 
used to combine paired timestamps and associated data into a single ANNIEEvent BoostStore.

## Data
Describe any data formats ANNIEEventBuilder creates, destroys, changes, or analyzes. E.G.

See the ANNIEEventBuilder.h header file for comments on the maps used/modified 
in the ANNIEEventBuilder tool.

## Configuration

Describe any configuration variables for ANNIEEventBuilder.

```
verbosity (int)
Indicates the printout level. 0 is quiet, 5 is the most verbose.

SavePath (string)
String that indicates the directory to save the processed file at.

ProcessedFilesBasename (string)
Base for the processed raw data file produced.  Each file will have the run number and 
subrun number appended at the end.

BuildType (string)
Type of build algorithm to use.  Possible options are:
Tank - Only build ANNIE events with PMT information.
MRD - Only build ANNIE events with MRD information.
TankAndMRD - Only pair Tank and MRD data into ANNIE events based on their relative timestamps.  
TankAndMRDAndCTC - Pair Tank and MRD data with CTC timestamps to fill data into ANNIE events.

NumEventsPerPairing (int)
Sets the minimum number of acquisitions found in any data stream needed before 
looping through the timestamp-pairing algorithms.

MinNumWavesInSet (int)
Minimum number of PMT waveforms that are associated with a single PMT timestamp before
the waves are collected and attempted to be placed in ANNIE events.

OrphanOldTankTimestamps (bool)
If any tank data in the InProgressTankEvents map is older than the most current timestamp
(difference set with OldTimestampThreshold config), the tank data is moved to 
the orphanage.  This helps keep the InProgressTankEvents map small, reducing loop
time whenever accessing entries in the map with a for loop.

OldTimestampThreshold (int)
Time threshold (in seconds) before a tank timestamp and it's waveforms are moved to
the orphanage.  Only used if OrphanOldTankTimestamps is 1.

DaylightSavingsSpring (bool)
Must be set to 1 if the data being processed was taken during daylight savings time.
The MRD and PMT datastreams are in different timezones, so the difference in time
between the streams varies based on whether daylight saving is happening.

ExecutesPerBuild (int)
Number of Execution loops ran through before the event building algorithms are 
checked.  

MRDTankTimeTolerance" (int)
When pairing MRD and Tank timestamps (TankAndMRD BuildType only), MRD and Tank data
will be paired into ANNIEEvents if their timestamps are within this time value.  
Value is given in milliseconds.

CTCTankTimeTolerance (int)
When pairing Tank and CTC timestamps (TankAndMRDAndCTC BuildType only), Tank and trigger data
will be paired into ANNIEEvents if their timestamps are within this time value.  
Value is given in nanoseconds.

CTCMRDTimeTolerance (int)
When pairing MRD and CTC timestamps (MRDAndMRDAndCTC BuildType only), MRD and trigger data
will be paired into ANNIEEvents if their timestamps are within this time value.  
Value is given in milliseconds.
```

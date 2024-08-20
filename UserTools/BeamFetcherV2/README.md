# BeamFetcherV2

The `BeamFetcherV2` tool obtains information about the status of the BNB from the IF database. It has two distinct modes of operation FetchFromTimes of FetchFromTrigger controlled by the boolean config flag `FetchFromTimes`. In FromTimes mode you will pass in start and stop times and a new file will be created with a BoostStore. In FromTrigger mode you first must run the TriggerDataDecoder then BeamFetcher will grab the trigger timestamp and find the nearest matching time for the beam device. FromTrigger mode will save the info into the CStore for access later on. The `IFBeamDBInterface` class is a helper class that handles the details of the communication with the IF database.

## Data

FetchFromTimes: The following objects will be saved in the BeamStatus BoostStore
* "BeamDBIndex" (Header) `map<int,pair<uint64_t,uint64_t>`
  * Designates the time interval that is stored in the given BoostStore entry
* "StartMillisecondsSinceEpoch" (Header) `uint64_t`
  * Designates the overall start time of database entries stored in the BoostStore
* "EndMillisecondsSinceEpoch" (Header) `uint64_t`
  * Designates the overall end time of database entries stored in the BoostStore
* "BeamDB" `map<string,map<uint64_t,BeamDataPoint>>`
  * Actual beam status information, keys are the device names (e.g. E:TOR875)

FetchFromTrigger: The following objects will be put into the CStore
* "BeamData" `map< uint_64, map<std::string, BeamDataPoint> >`
 * The info from the DB where the key is the associated trigger timestamp to the nearest milliseconds and the internal map is from device name (e.g. E:TOR875) to a BeamDataPoint object containing the readback value, units, and actual timestamp.
 
## Configuration

The main configuration variable for the `BeamFetcherV2` tool is `FetchFromTimes`, which determines whether to use the tigger timestamps or user input timestamps. The `DevicesFile`, `IsBundle`, and `TimeChunkStepInMilliseconds` variables are required regardless of the fetch mode. You can also set the  `SaveROOT` bool in order to save out a TTree with the timestamp and device values as the leaves. 

If `FetchFromTimes == 1` then you will also need the additional config variables. The preferred timestamp format is chosen with the `TimestampMode` variable (LOCALDATE/MSEC/DB). For LOCALDATE mode you use Start/EndDate files with string formatted times (like 2023-04-11 23:03:19.163505). For MSEC mode you use the Start/EndMillisecondsSinceEpoch variables. For DB mode you must first run `LoadRunInfo` and the run timestamps will be pulled from the CStore. 

```
# BeamFetcherV2 config file
verbose 1
#
# These three are always needed
#
DevicesFile ./configfiles/BeamFetcherV2/devices.txt # File containing one device per line or a bundle
IsBundle 0 # bool stating whether DevicesFile contains bundles or individual devices
FetchFromTimes 0 # bool defining how to grab the data (from input times (1) or trigger(0))
TimeChunkStepInMilliseconds 3600000 # one hour
SaveROOT 0 # bool, do you want to write a ROOT file with the timestamps and devices?
DeleteCTCData 0 # bool, delete viewed CTC timestamps? Helps reduce memory overhead when not running ANNIEEventBuilder.
#
# These parameters are only needed if FetchFromTimes == 1
#
OutputFile ./1604_beamdb
TimestampMode LOCALDATE
DaylightSavings 0 # Do we need to account for DST?
StartDate ./configfiles/BeamFetcher/my_start_date.txt #String form of start date stored in a file
EndDate ./configfiles/BeamFetcher/my_end_date.txt #String form of end date stored in a file
#StartMillisecondsSinceEpoch 1491132659000 # 6:30:49 AM 2 April 2017 (FNAL time) #msec format of start time
#EndMillisecondsSinceEpoch   1491164001000 # 3:13:21 PM 2 April 2017 (FNAL time) #msec format of end time
```

# BeamQuality

The `BeamQuality` tool is part of the Event Building chain in ANNIE and forwards information about the Beam Status to the `ANNIEEventBuilder` tool. It uses information that was previously retrieved from the beam database with the `BeamFetcherV2` tool. This mimic the `BeamQuality` tool that is used for the original `BeamFetcher`.

The information is saved in the form of the `BeamStatus` class. This class contains some basic information like the POT for the timestamp in question and some more detailed information about the horn currents. It is possible to already choose some beam quality cuts for the timestamp tolerance, horn currents, and POT values. However, some beam information will be stored in the object, so it will be possible to use slightly different cuts when analyzing the data later.

## Data

The Beam Status information is stored in the `BeamStatusMap` object and put in the CStore. The `ANNIEEventBuilder` tool can access the object in the CStore and write the information to the ANNIEEvent BoostStore.

The `BeamQuality` tool goes through the decoded trigger timestamps and searches for the beam status at each of these trigger timestamps (in case there was a beam trigger). The properties of the beam are then saved in the BeamStatus object and put into the `BeamStatusMap`. It then deletes the used BeamData to free up memory.

**BeamStatusMap** `map<uint64_t, BeamStatus>`
* Beam status for the trigger timestamps

The `BeamStatusMap` is stored in the form of a pointer, and the `ANNIEEventBuilder` will delete already built entries from the map to free up memory.

## Configuration

BeamQuality has the following configuration variables:

```
# BeamQuality config file
verbose 1
# POT window for "good" beam
POTMin 5e11
POTMax 8e12
# Horn current window for "good" beam (in kA)
HornCurrentMin 172
HornCurrentMax 176
# Fractional difference between the upstream and downstream POT measurements
BeamLossTolerance 0.05
```

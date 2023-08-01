# Stage1DataBuilder
Stage1DataBuilder

This tool is used to separate LAPPD data into separate BoostStores (1 per LAPPD), and save these with ANNIEEvent BoostStores created by DataDecoder upstream.

## Data

Describe any data formats Stage1DataBuilder creates, destroys, changes, or analyzes. E.G.

LAPPD1Data (BoostStore)
ANNIEEvent_noLAPPD (BoostStore)
Stage1Data (multientry BoostStore)

## Configuration

Describe any configuration variables for Stage1DataBuilder.

```
verbosity (int)

BaseName (string)
The base name of the output Stage1Data file

RunNumber (int)
```

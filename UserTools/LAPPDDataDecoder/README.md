# LAPPDDataDecoder

The `LAPPDDataDecoder` tool converts the LAPPD raw data into waveforms in the scope of the event building process. The conversion from raw data to waveforms is copied from the `LAPPDStoreReadIn` tool, and then just adapted to work with the event building toolchain.

## Data

The `LAPPDDataDecoder` tool produces the following data output objects which are then accessed by downstream tools:

* `LAPPDPPS = new std::vector<uint64_t>`;
* `LAPPDTimestamps = new std::vector<uint64_t>`;
* `LAPPDBeamgateTimestamps = new std::vector<uint64_t>`;
* `LAPPDPulses = new std::vector<PsecData>`;
* `bool LAPPDDataBuilt`;

They are saved with the followind keywords in the `CStore`:

```
 m_data->CStore.Set("InProgressLAPPDEvents",LAPPDPulses);
 m_data->CStore.Set("InProgressLAPPDPPS",LAPPDPPS);
 m_data->CStore.Set("InProgressLAPPDTimestamps",LAPPDTimestamps);
 m_data->CStore.Set("InProgressLAPPDBeamgate",LAPPDBeamgateTimestamps);
 m_data->CStore.Set("NewLAPPDDataAvailable",LAPPDDataBuilt);
```


## Configuration

The only configuration variable for the `LAPPDDataDecoder` tool is the number of entries that are supposed to be handled for each `Execute` step (similar to the other `Decoder` tools).

```
verbosity 1
EntriesPerExecute 10
```

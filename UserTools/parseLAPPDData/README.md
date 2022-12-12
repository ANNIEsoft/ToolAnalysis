# parseLAPPDData

The `parseLAPPDData` tool parses the LAPPD raw data and creates waveform and metadata objects which can be analyzed by further downstream tools. It is supposed to be used in combination with the `checkLAPPDStatus` tool and processed `ANNIEEvent` BoostStore files.

## Output

It produces the following output data objects:

```
m_data->CStore.Set("LAPPD_TimeSinceBeam",time_since_beamgate);
m_data->CStore.Set("LAPPD_InBeamWindow",in_beam_window);
m_data->CStore.Set("LAPPD_ParsedData",data);
m_data->CStore.Set("LAPPD_ParsedMeta",meta);
```

of following types:
* `time_since_beamgate: uint64_t`: time of LAPPD data with respect to start of beamgate in nanoseconds
* `in_beam_window: bool`: was the LAPPD data in the expected beam window between 7 microseconds and 10 microseconds? (currently very large window)
* `data:  map<int, std::vector<unsigned short>>`: the actual LAPPD waveform data
* `meta: std::vector<unsigned short>`: the LAPPD meta data\


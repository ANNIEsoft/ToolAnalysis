# MonitorSimReceiveSingleFile

MonitorSimReceiveSingleFile is used to simulate receiving monitoring data for a single input file.

## Data

MonitorSimReceiveSingleFile simulates the process of receiving data for a single input raw data file. It initializes the raw data file as the BoostStore `indata`, obtains the two sub-BoostStores `PMTData` and `MRDData` from `indata` and stores the `PMTData` and `MRDData` BoostStores in respective common BoostStores:

* `m_data->Stores["PMTData"]->Set("FileData",PMTData,false)`
* `m_data->Stores["CCData"]->Set("FileData",MRDData,false)`
* `m_data->Stores["CCData"]->Set("Single",MRDOut)`

MonitorSimReceiveSingleFile has the option to deliver the raw data as a complete file (`Mode File`) or as a single event (`Mode Single`) The Single Event option will only take effect for the MRD data, since the PMTData cannot be transmitted event-wise. The monitoring files can then obtain the BoostStores from there and process the data.

## Configuration

The path to the raw file can be specificied via the `MRDDataPath` configuration keyword. `OutPath` specifies the output path for monitoring plots and is forwarded to the Monitoring tools via the `CStore`. The `Mode` keyword specifies whether the raw data is transmitted in chunks of whole data files or single events (MRD only).

```
MRDDataPath /pnfs/annie/persistent/users/mnieslon/data/rawdata/beam/RAWDataR1442S0p21
OutPath ./monitoringplots/
Mode File	#Options: File/Single
verbose 0
```

# MonitorSimReceive

MonitorSimReceive is used to simulate receiving monitoring data.

## Data

MonitorSimReceive simulates the process of receiving data. It initializes the raw data file as the BoostStore `indata`, obtains the two sub-BoostStores `PMTData` and `MRDData` from `indata` and stores the `PMTData` and `MRDData` BoostStores in respective common BoostStores:

* `m_data->Stores["PMTData"]->Set("FileData",PMTData,false)`
* `m_data->Stores["CCData"]->Set("FileData",MRDData,false)`

The keywords for the BoostStores is "FileData" in both cases. The monitoring files can then obtain the BoostStores from there and process the data.

## Configuration

A list of raw files can be specificied via the `FileList` configuration keyword. The file should contain the path for one raw file per line. `OutPath` specifies the output path for monitoring plots and is forwarded to the Monitoring tools via the `CStore`.

```
FileList ./configfiles/Monitoring/MRDfiles.txt
OutPath ./monitoringplots/
verbose 0
```

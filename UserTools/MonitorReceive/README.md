# MonitorReceive

MonitorReceive

The MonitorReceive tool is used to continuously collect remote monitoring data.

## Data

MonitorReceive is continuously running and checks whether new data files have been produced. If so, it then provides the raw data to the Monitoring tools as follows: The tool initializes the raw data file as the BoostStore `indata` with the current raw data file, obtains the two sub-BoostStores `PMTData` and `MRDData` from `indata` and stores the `PMTData` and `MRDData` BoostStores in respective common BoostStores:

* `m_data->Stores["PMTData"]->Set("FileData",PMTData,false)`
* `m_data->Stores["CCData"]->Set("FileData",MRDData,false)`

The keywords for the BoostStores is "FileData" in both cases. The monitoring files can then obtain the BoostStores from there and process the data.

## Configuration

The MonitorReceive tool will note the path where to store the monitoring plots in the CStore, so that the subsequent Monitoring tools can obtain the path and use it. The user can configure this path via the configuration variable `OutPath`.

```
OutPath ./monitoringplots/
```

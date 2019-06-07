# MonitorMRDLive

MonitorMRDLive

## Data

Creates live event plots for raw data from the MRD DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorMRDLive has the following configuration variables:

```
verbose 2
OutputPath /ANNIECode/MRDMonitorTest/               #if output path for plots needs to be set manually
#OutputPath fromStore                               #if output path for plots can be taken from m_data
ActiveSlots configfiles/Monitoring/MRD_activech.txt #define which channels of the crate are connected
```

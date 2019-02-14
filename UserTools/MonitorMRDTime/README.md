# MonitorMRDTime

MonitorMRDTime

## Data

Creates time evolution plots for raw data from the MRD DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorMRDTime has the following configuration variables:

```
verbose 2
OutputPath /ANNIECode/MRDMonitorTest/               #if output path for plots needs to be set manually
#OutputPath fromStore                               #if output path for plots can be taken from m_data
ActiveSlots configfiles/Monitoring/MRD_activech.txt #define which channels of the crate are connected
StartTime 1970/1/1	                                #used for conversion of timestamps to date/times. default: 1970/1/1
OffsetDate 0	                                      #if the TimeStamp variable of MRDOut has an offset, adjust number of msec
DrawMarker 1                                        #graphs with (without) markers: 1 (0)
```


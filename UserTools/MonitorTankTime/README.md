# MonitorTankTime

MonitorTankTime

## Data

Creates time evolution plots for raw data from the Tank PMT DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorTankTime has the following configuration variables:

```
verbose 2
OutputPath /ANNIECode/TankMonitorTest/              #if output path for plots needs to be set manually
#OutputPath fromStore                               #if output path for plots can be taken from m_data
ActiveSlots configfiles/Monitoring/PMT_activech.txt #define which cards in which VME crates are connected
StartTime 1970/1/1	                                #used for conversion of timestamps to date/times. default: 1970/1/1
OffsetDate 0	                                      #if the TimeStamp variable of PMTOut has an offset, adjust number of msec
```


# MonitorTankLive

MonitorTankLive

## Data

Creates live event plots for raw data from the Tank PMT DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorTankLive has the following configuration variables:

```
verbose 2
OutputPath /ANNIECode/TankMonitorTest/              #if output path for plots needs to be set manually
#OutputPath fromStore                               #if output path for plots can be taken from m_data
ActiveSlots configfiles/Monitoring/PMT_activech.txt #define which cards in which VME crate are connected
```

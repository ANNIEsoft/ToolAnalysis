# MonitorMRDLive

MonitorMRDLive

## Data

Creates live event plots for raw data from the MRD DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorMRDLive has the following configuration variables:

```
verbose 2
OutputPath /ANNIECode/MRDMonitorTest/ #if output path for plots needs to be set manually
#OutputPath fromStore #if output path for plots can be taken from m_data
ActiveSlots configfiles/Monitoring/MRD_activeslots.txt #define which channels of the crate are connected
InActiveChannels configfiles/Monitoring/MRD_inactivech.txt  #define single inactive channels in otherwise active slots
LoopbackChannels configfiles/Monitoring/MRD_loopback.txt  #define position of loopback channels
AveragePlots 0  #should averaged plots be shown for crates/slots
```

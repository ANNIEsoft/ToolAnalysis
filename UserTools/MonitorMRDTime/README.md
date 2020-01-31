# MonitorMRDTime

MonitorMRDTime

## Data

Creates time evolution plots for raw data from the MRD DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorMRDTime has the following configuration variables:

```
verbose 2
OutputPath /ANNIECode/MRDMonitorTest/ #if output path for plots needs to be set manually
#OutputPath fromStore #if output path for plots can be taken from m_data
ActiveSlots configfiles/Monitoring/MRD_activeslots.txt  #define which channels of the crate are connected
InActiveChannels configfiles/Monitoring/MRD_inactivech.txt  #define which channels of slots are not active
LoopbackChannels configfiles/Monitoring/MRD_loopback.txt  #define the position of loopback channels
StartTime 1970/1/1  #used for conversion of timestamps to date/times. default: 1970/1/1
Mode Continuous #options: FileList / Continuous
PlotConfiguration configfiles/Monitoring/MRDTimePlotConfig.txt  #file containing instructions what to plot
PathMonitoring /monitoringfiles/  #path at which the monitoring plots are going to be saved
ImageFormat png #format in which monitoring plots are saved. Options: png, jpg, jpeg
UpdateFrequency 1.  #specify frequency for the file history plot, in mins
ForceUpdate 0 #force monitor plots to be produced even if there was no new data file available
DrawMarker 1  #graphs with (without) markers: 1 (0)
```


# MonitorMRDEventDisplay

MonitorMRDEventDisplay

## Data

Creates live Event Displays for raw data from the MRD DAQ, to be shown on the monitoring webpage. 

## Configuration

MonitorMRDEventDisplay has the following configuration variables:

```
verbose 0
#OutputPath /ANNIECode/MRDMonitorOutputTest/  #output path for live event display plots
OutputPath fromStore    #output path for live event display plots to be taken from m_data
CustomRange 1           #TDC range with user-defined values?
RangeMin 1              #minimum TDC value
RangeMax 100            #maximum TDC value
```

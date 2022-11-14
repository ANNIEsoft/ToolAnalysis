# MonitorLAPPDSC

The `MonitorLAPPDSC` tool is part of the official ANNIE online monitoring toolchain and produces different kinds of plots for the slow control monitoring of the LAPPDs.

Monitored variables are e.g. the temperature, humidity, HV values, LV values, thermistor resistance, salt bridge, and light level values.

Separate colors are used to distinguish the different LAPPDs in time evolution plots of the properties.

Slack messages are sent in case of severe warnings & errors via the webhook.

## Configuration

`MonitorLAPPDSC` has the following configuration variables:

```
# LAPPD SC Monitoring config file

verbose 0 #verbosity
OutputPath fromStore #output path for monitoring rootfiles
StartTime 1970/1/1 #start time for the conversion of timestamps into dates
PlotConfiguration ./configfiles/Monitoring/LAPPDSCPlotConfig.txt #file containing instructions what to plot
PathMonitoring /monitoringfiles/	#path at which the monitoring plots are going to be saved
ImageFormat png	#format in which monitoring plots are saved. Options: png, jpg, jpeg
UpdateFrequency 1. #specify frequency for the file history plot, in mins
ForceUpdate 0	#force monitor plots to be produced even if there was no new data file available
DrawMarker 0	#specify whether to use markers for the time evolution graphs or not
VoltageMin33 3.1  #Minimum accepted voltage for V33
VoltageMax33 3.5  #Maximum accepted voltage for V33
VoltageMin25 2.9  #Minium accepted voltage for V25
VoltageMax25 3.3  #Maximum accepted voltage for V25
VoltageMin12 1.6  #Minimum accepted voltage for V12
VoltageMax12 2.0  #Maximum accepted voltage for V12
LimitSaltLow 500000 #First warning threshold for salt bridge values
LimitSaltHigh 400000  #Second warning threshold for salt bridge values
LimitTempLow 50.  #First warning threshold for temperature
LimitTempHigh 58. #Second warning threshold for temperature
LimitHumLow 15. #First warning threshold for humidity
LimitHumHigh 20.  #Second warning threshold for humidity
LimitThermistorLow 7000.  #First warning threshold for thermistor
LimitThermistorHigh 5800. #Second warning threshold for thermistor
LAPPDIDFile ./configfiles/Monitoring/LAPPDIDs.txt #File containing the expected LAPPD IDs
```

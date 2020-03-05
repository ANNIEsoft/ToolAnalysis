# The Monitoring toolchain

***********************
## Description
***********************

The monitoring toolchain is designed to be running continuously on the live data being read out by the DAQ from the different subsystems (MRD CAMAC system, PMT VME system, LAPPDs). There can be multiple tools for one subsystem, exploing the fact that data can be delivered in little chunks (live data) or in file format. For the MRD system, e.g., there is the `MonitorMRDLive` tool that produces several plots based on the live data, the `MonitorMRDTime` tool that reads out whole data files and then plots time evolution and integrated hitmaps, and the `MonitorMRDEventDisplay` tool that shows Event Displays of the TDC values based on live data. For the VME system, there is the `MonitorTankTime` tool that looks at whole VME raw data files and plots time evolution plots of pedestals, sigmas and rates as well as ADC frequency plots and ADC buffer plots. The PMT data needs to be prepared via the `PMTDataDecoder` tool for the `MonitorTankTime` tool to work.

***********************
## Configuration
***********************

Current configuration file contain information regarding the hardware setup that is currently in use:

* `MRD_activeslots.txt`: Defines which slots of the two CAMAC crates are occupied by the TDCs (`crate slot`)
* `MRD_inactivech.txt`: Defines which channels of the TDCs are not expecting any input signals (`crate slot channel`)
* `MRD_loopback.txt`: Defines the MRD loopback channels (`name crate slot channel`)
* `MRDTimePlotConfig.txt`: Configuration file for the `MonitorMRDTime` tool. Each line represents user-defined plots that should be produced & saved for a given time-frame. 
* `PMT_activeslots.txt`: Defines which slots of the three VME crates are occupied by ADCs (`crate slot`)
* `PMT_disabledch.txt`: Defines which channels of the ADCs are not expecting any input signals (`crate slot channel`)
* `PMT_signalch.txt`: Defines the RWM and BRF channels within the VME crates (`name crate slot channel`)

************************
## Continuous mode 
************************

The Monitoring toolchain is normally running in a way that all the tools are waiting for a keyword in the `State` variable, indicating that either `Live` or `FileData` is available to be processed. The tools `MonitorMRDLive` and `MonitorMRDEventDisplay` simply take the live data and produce their respective plots, while the `MonitorMRDTime` and `MonitorTankTime` tools write the monitoring data into dedicated rootfiles that can also be accessed later before producing the current time evolution plots (see "Custom time evolution plots")

************************
## Custom time evolution plots 
************************

Since all time evolution plots are created by accessing monitoring rootfiles that are created by the `MonitorMRDTime` tool, it is possible to create monitoring plots also for time frames that lie in the past and have corresponding monitoring rootfiles. The user therefore has to use the toolchain without `MonitorReceive` and set the number of `Execute` steps to 1 in `ToolChainConfig`. It is also necessary to specify `ForceUpdate 1` in the `MonitorMRDTime` and `MonitorTankTime` config files. 

************************
## Plot configuration files
************************

The plots that are to be produced can be customized in the `MRDTimePlotConfig.txt`/`PMTTimePlotConfig.txt` files. Each line in the config file specifies the plots for a custom time frame. The format for specifying the plots is the following:

`TIME_FRAME(hours)	T_END("TEND_LASTFILE" / "YYYYMMDDTHHMMSS")	PLOTS_LABEL	PLOTTYPE_1	PLOTTYPE_2	PLOTTYPE_3	PLOTTYPE_4 	(etc, arbitrary number of plot types may be specified)`

Available plot types are:
* `MRDTimePlotConfig.txt`: Hitmap, PieChartTrigger, TriggerEvolution, TimeEvolution, RatePhysical, RateElectronics, FileHistory
* `PMTTimePlotConfig.txt`: RateElectronics, RatePhysical, PedElectronics, PedPhysical, SigmaElectronics, SigmaPhysical, TimeEvolution, TimeDifference, FileHistory

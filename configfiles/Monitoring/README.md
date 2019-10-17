# The Monitoring toolchain

***********************
## Description
***********************

The monitoring toolchain is designed to be running continuously on the live data being read out by the DAQ from the different subsystems (MRD CAMAC system, PMT VME system, LAPPDs). There can be multiple tools for one subsystem, exploing the fact that data can be delivered in little chunks (live data) or in file format. For the MRD system, e.g., there is the `MonitorMRDLive` tool that produces several plots based on the live data, the `MonitorMRDTime` tool that reads out whole data files and then plots time evolution and integrated hitmaps, and the `MonitorMRDEventDisplay` tool that shows Event Displays of the TDC values based on live data.

***********************
## Configuration
***********************

Current configuration file contain information regarding the hardware setup that is currently in use:

* `MRD_activeslots.txt`: Defines which slots of the two CAMAC crates are occupied by the TDCs (`crate slot`)
* `MRD_inactivech.txt`: Defines which channels of the TDCs are not expecting any input signals (`crate slot channel`)
* `MRD_loopback.txt`: Defines the MRD loopback channels (`name crate slot channel`)
* `MRDTimePlotConfig.txt`: Configuration file for the `MonitorMRDTime` tool. Each line represents user-defined plots that should be produced & saved for a given time-frame. 

************************
## Continuous mode 
************************

The Monitoring toolchain is normally running in a way that all the tools are waiting for a keyword in the `State` variable, indicating that either `Live` or `FileData` is available to be processed. The tools `MonitorMRDLive` and `MonitorMRDEventDisplay` simply take the live data and produce their respective plots, while the `MonitorMRDTime` tool writes the monitoring data into dedicated rootfiles that can also be accessed later before producing the current time evolution plots (see "Custom time evolution plots")

************************
## Custom time evolution plots 
************************

Since all time evolution plots are created by accessing monitoring rootfiles that are created by the `MonitorMRDTime` tool, it is possible to create monitoring plots also for time frames that lie in the past and have corresponding monitoring rootfiles. The user therefore has to use the toolchain without `MonitorReceive` and set the number of `Execute` steps to 1 in `ToolChainConfig`. It is also necessary to specify `ForceUpdate 1` in the `MonitorMRDTime` config file. 

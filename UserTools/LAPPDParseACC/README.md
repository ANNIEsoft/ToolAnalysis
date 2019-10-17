# LAPPDParseACC

This tool is made to parse the files produced from the ANNIE Central Card electronics. The design and format of those files is described at this [github page](https://github.com/lappd-daq/acdc-daq). It supports using multiple boards, where events may or may not contain all boards. 




## Options
The following need to be defined at configuration level
```
store_name <string name of the store, say ANNIEEvent, this is also where raw waveforms will be stored>
geometry_name <name of geometry object stored in the store_name above>
lappd_data_filepath <path to the raw data>
lappd_data_filename <base name of the three files output from ACDC, e.g. mydata (no extension)>
```


## Stores
The following items are accessed and saved from the store
```
# Geometry class of the data being analysed. (accessed)
This is used to determine which detectors are LAPPDs, which channel keys are associated with those LAPPDs, and which channel keys map to which channel numbers and ACDC board numbers. Thus, it is critical for the user to set up a map between boards and channels during the geometry building phase. In writing this tool, I followed the example of the LoadGeometry tool.  

# AllLAPPDChannels, map<unsigned long, Channel>* (created and saved)
This is a map that contains LAPPD channel objects indexed by unique geometry channel key


#LAPPDWaveforms, map<unsigned long, Waveform<double>>* (created and saved)
This is the raw data created by parsing the data files. 


```
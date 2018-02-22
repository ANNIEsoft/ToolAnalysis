# LAPPDParseACC

This tool is made to parse the files produced from the ANNIE Central Card electronics. The design and format of those files is described at this [github page](https://github.com/lappd-daq/acdc-daq).


## Options
The following need to be defined at configuration level
```
filepath <the base folder of the input files>
filename <the base name of the three files>
print <true/false> # Verbosity level
```

an example of this would be
```
filepath ./fakedata
filename test
print false
```
This would read the three following files
```
fakedata/test.ped
fakedata/test.acdc
fakedata/test.meta
```

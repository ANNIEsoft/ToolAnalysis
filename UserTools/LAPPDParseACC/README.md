# LAPPDParseACC

This tool is made to parse the files produced from the ANNIE Central Card electronics. The design and format of those files is described at this [github page](https://github.com/lappd-daq/acdc-daq).

There are three items added into an `ANNIEEvent` boost store:

**Pedestal Data**
`map<int, vector<Waveform<double>>>` 
* key: some integer representing boards and channels (soon to be replaced with Geometry class).
* value: vector of length one with a Waveform whose samples represent the raw pedestal data in PSEC ADC counts
* store->("rawPedData")

**ACC Data**
`map<int, vector<Waveform<double>>>` 
* key: some integer representing boards and channels (soon to be replaced with Geometry class).
* value: vector of length one with a Waveform whose samples represent the pedestal subtracted data in PSEC ADC counts
* store->"RawLAPPDData"

**Meta Data**
`map<int, map<string, double>>`
* key: some integer representing boards and channels (soon to be replaced with Geometry class).
* value: a map between strings and double with all the keywords listed in the data.meta file and their corresponding values
* store->Header->"metaData"

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

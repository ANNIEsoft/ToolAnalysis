# ReadConfigInfo

ReadConfigInfo

## Data

Reads the ANNIEEvent Header `ANNIEEvent->Header->Get("ConfigInfo",config_info)`
and saves the content of the config_info string into an outputfile.
The ANNIEEvent BoostStore is provided by the LoadANNIEEvent Tool.
It can produce a single outputfile, or an individual outputfile for each Processed ANNIEEvent file given to the LoadANNIEEvent Tool.

## Configuration

Describe any configuration variables for ReadConfigInfo.

```
verbosity 3
#If there is no FileName variable the filenames will be produced automatically as ConfigInfo_R*S*p*.txt.
#Otherwise only a single file will be produced with the name given with FileName
#FileName ReadConfigInfoTest.txt 
```

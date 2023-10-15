# SaveConfigInfo

SaveConfigInfo

## Data

Collects the git commit hash of the current folder and the content of a dynamic number of files into the
`std::string config_info` variable.
This string is then forwarded to the ANNIEEventBuilder Tool via `m_data->CStore.Set("ConfigInfo",config_info)`.
In ANNIEEventBuilder.cpp it is then saved in the ANNIEEvent Header `ANNIEEvent->Header->Set("ConfigInfo",config_info)`
This is done for each part of file of the respective run.

The SaveConfigInfo Tool is currently used in the `configfiles/DataDecoderwConfigInfo` ToolChain.

## Configuration


```
verbosity 0
OutFileName config_info.txt #Save the config_info string additionally as a text file, external to the ANNIEEvent BoostStore file
ToolChainConfigFile ./configfiles/DataDecoderwConfigInfo/ToolChainConfig #The path and filename of the ToolChainConfig file, from which all other files are read
FullDepth 1 # some config files link to other *.txt or *.csv files and such. For "FullDepth 1" these are also saved, for 0 they are not saved.
```
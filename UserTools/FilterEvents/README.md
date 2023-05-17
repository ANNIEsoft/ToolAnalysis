# FilterEvents

The `FilterEvents` tool reads in ANNIEEvent BoostStore files and saves them to a Filtered output BoostStore files in case selected cuts were passed for a given event. The cuts themselves are defined in the `EventSelector` tool and official filters should be defined in the `configfiles/Filters/` directory.

## Data

The `FilterEvents` tool reads in the complete `ANNIEEvent` BoostStore file and saves all the objects to a new, filtered output BoostStore file. 

* Input data: `ANNIEEvent` BoostStore (typically loaded with the `LoadANNIEEvent` tool)
* Output data: `ANNIEEvent` BoostStore, saved to a new file. Only events which passed the cuts specified in the `EventSelector` tool are saved to this output BoostStore file

Within the output BoostStore file, a separate flag indicates that the events have been filtered. This flag is a Boolean and is called "FilteredEvent". In addition, the FilterName is also saved to the output BoostStore:

* `FilteredEvent (bool)`: indicates whether the object is part of a filtered output BoostStore file
* `FilterName (string)`: The name of the applied filter in the `EventSelector` tool.

## Configuration

`FilterEvents` uses the following configuration variables:

```
verbosity 2                             #how much logging output do you want?
FilteredFilesBasename FilteredEvents    #base name of the BoostStore output files
                                        #the name of the Filter will be appended automatically to this basename
SavePath /path/to/output/               #where should the output files be stored
```

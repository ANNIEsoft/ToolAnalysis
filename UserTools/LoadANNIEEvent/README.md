# LoadANNIEEvent

LoadANNIEEvent loads the `ANNIEEvent` BoostStore from a stored `ANNIEEvent` file. It loops over all the events in the BoostStore and provides one event for each `Execute` step for the subsequent tools in the toolchain.

A list containing all the input ANNIEEvent files should be specified by using the `FileForListOfInputs` command. 

Other tools can influence which event numbers are loaded by setting the variable `UserEvent` in the `CStore` to `true` and setting the desired event number for the respective Execute step via the `LoadEvNr` variable in the `CStore`.

The `EventOffset` variable specifies whether the toolchain should start at a certain event number. If set to 0, it will start from the first event, but if set to e.g. 99, the first loaded event will be the 100th event. 

The `GlobalEvNr` variable enables the possibility to calculate global event numbers for the entire specified file list. This is useful if one for example wants to loop over multiple files belonging to the same run and wants to introduce a unique event ID mapping within this run. (Otherwise, the event IDs are duplicated since every part file will start counting events from 0 again)

## Configuration

Describe any configuration variables for LoadANNIEEvent.

```
verbose int
FileForListOfInputs string
EventOffset 0
GlobalEvNr 1
```

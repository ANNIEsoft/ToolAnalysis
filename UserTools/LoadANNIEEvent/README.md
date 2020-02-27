# LoadANNIEEvent

LoadANNIEEvent loads the `ANNIEEvent` BoostStore from a stored `ANNIEEvent` file. It loops over all the events in the BoostStore and provides one event for each `Execute` step for the subsequent tools in the toolchain.

A list containing all the input ANNIEEvent files should be specified by using the `FileForListOfInputs` command. 

Other tools can influence which event numbers are loaded by setting the variable `UserEvent` in the `CStore` to `true` and setting the desired event number for the respective Execute step via the `LoadEvNr` variable in the `CStore`.

## Configuration

Describe any configuration variables for LoadANNIEEvent.

```
verbose int
FileForListOfInputs string
```

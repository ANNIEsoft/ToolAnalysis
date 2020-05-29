# PrintRecoEvent

PrintRecoEvent goes through the RecoEvent store and prints the properties of its stored objects.

## Data

PrintRecoEvent tries to access all the objects which could be available in the RecoEvent store and prints their properties, if found. It does not create or forward any objects to other tools, but is supposed to give an overview over the contents of the RecoEvent store.

## Configuration

PrintRecoEvent has the following configuration options:

```
verbosity 1
isMC 0
```

If the option `isMC` is specified, the tool will try to access and print information that's only available in MC, such as the number of pions, the true energy or the true vertex of the muon.

# checkLAPPDStatus

The `checkLAPPDStatus` tool checks whether an event in an `ANNIEEvent` BoostStore contains LAPPD data and sets a flag in the `CStore` to indicate the outcome of the check.

If LAPPD data is found, the following variable is set:
```
m_data->CStore.Set("LAPPD_HasData",true);
```

If no LAPPD data is present for the respective entry of the `ANNIEEvent` BoostStore, the same variable is set to `false` instead.


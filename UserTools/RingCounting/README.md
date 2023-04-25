# RingCounting

The RingCounting Tool is used to classify events into single- and multi-Cherenkov-ring-events.
For this a machine learning approach is used. 
---
This Tool uses PMT data in the CNNImage format (see UserTools/CNNImage). For further details on the tool and
the models used (including performance etc.) see the documentation found on the anniegpvm-machines at (**Todo**)
```
/pnfs/annie/persistent/users/dschmid/RingCountingStore/documentation/
```
---

All models can be found at
```
/pnfs/annie/persistent/users/dschmid/RingCountingStore/models/
```
and (some) at
```
/annie/app/users/dschmid/RingCountingStore/models/
```
## Data

- Currently does not add predictions to any BoostStore. This is planned in a future update (**Todo**)

---
## Configuration

The following configuration variables must be set for the tool to function properly (further details)
are found at the top of the RingCounting.py file.
---
Exemplary configuration:
```
files_to_load configfiles/RingCounting/files_to_load.txt
version 1_0_0
model_path /annie/app/users/dschmid/RingCountingStore/models/
pmt_mask november_22
save_to RC_output.csv
```

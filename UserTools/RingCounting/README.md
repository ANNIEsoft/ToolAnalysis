# RingCounting

The RingCounting Tool is used to classify events into single- and multi-Cherenkov-ring-events.
For this a CNN-based machine learning approach is used. 
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

## Data

- In the "load_from_file" mode, this tool adds single- and multi-ring (SR/MR) predictions to the RecoEvent BoostStore. When theh "load_from_file" config parameter is set to 0, the tool instead outputs the predictions to a csv file.
- The predictions are stored in the "RingCountingSRPrediction" and "RingCountingMRPrediction" variables 

---
## Configuration

The following configuration variables must be set for the tool to function properly (further details)
are found at the top of the RingCounting.py file.
---
Exemplary configuration:
```
load_from_file 0                                                # If set to 1, load CNNImage formatted csv files
files_to_load configfiles/RingCounting/files_to_load.txt        # txt file containing files to load in case load_from_file == 1
version 1_0_0                                                   # Model version
model_path /annie/app/users/dschmid/RingCountingStore/models/   # Model path
pmt_mask november_22                                            # PMT mask (zeroed out)
save_to RC_output.csv                                           # if load_from_file == 1, save predictions as csv
```

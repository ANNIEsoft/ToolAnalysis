# CNNImage

CNNImage creates ANNIE event display information in a csv-file format that can directly be loaded into Machine Learning classifier frameworks like CNNs for image classification purposes. Currently only PMT information from the side PMTs is loaded, no information from the top/bottom PMTs and LAPPDs is used.

## Data

CNNImage creates one `.csv`-file and one `.root`-file. The csv-file contains the event display information in single rows for each event, whereas the root-file provides the same information in a 2D histogram format.


## Configuration

CNNImage uses the following configuration variables:

```
verbosity 1     
Mode Charge             #options: Charge/Time
Dimension 10            #choose suitable image size, image will be dimension x dimension pixels
OutputFile cnn_10       #csv/root file name
DetectorConf ANNIEp2v6  #specify detector version of simulation
```

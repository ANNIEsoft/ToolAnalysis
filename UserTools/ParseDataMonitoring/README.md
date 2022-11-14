# ParseData

The `ParseDataMonitoring` tool is a part of the official ANNIE monitoring toolchain and decodes the LAPPD raw data in a readable format for the subsequent tools.

The tool uses the functionality of `LAPPDStoreReadIn` and outputs both a metadata vector as well as a LAPPD waveform object, consisting of a map of channelkeys to waveforms.

## Configuration

The `ParseDataMonitoring` tool uses the following configuration variables:

```
verbose 0   #verbosity
MaxEntriesPerLoop 1000  #max number of BoostStore entries which are done in one Execute step
DoPedSubtraction 1 #Do pedestal substraction flag
Nboards 6 #Number of pedestal files to be read in, 2boards per LAPPD --> Nboards 6 = 3 LAPPDs
Pedinputfile .configfiles/Monitoring/LAPPD_Ped/PEDS_ACDC  #PED input file (Store)
PedinputfileTXT ./configfiles/Monitoring/LAPPD_Ped/PEDS_ACDC_board #prefix of the pedestal files path+name. index and filetype will be set automatically
GlobalBoardConfig ./configfiles/Monitoring/LAPPDGlobalBoardConfig.txt #Global LAPPD Board numbering scheme. Needed in case of multiple LAPPDs in order to combine ACC_ID and board ID to a global LAPPD Board ID that is unique 
```

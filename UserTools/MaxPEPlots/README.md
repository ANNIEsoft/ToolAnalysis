# MaxPEPlots

The `MaxPEPlots` tool investigates the efficiency of issuing extended windows in ANNIE as a function of different parameters. It loops over the pulses recorded within the prompt window, and compares for each channel whether the Multi-P.E. threshold was passed, i.e. whether an extended window should have been opened. It then checks whether there was actually an extended window and computes the efficiency of issuing extended windows. The efficiency is calculated as a function of 

* the number of PMTs above MPE threshold
* the maximum observed ADC value within the event
* the pulse time within the prompt acquisition window.

## Output

The output is saved in a ROOT-file in the form of several histograms and `TEfficiency` objects.


## Configuration

`MaxPEPlots` has the following configuration parameters:

```
MaxPEFilename string #output filename
ThresholdFile string #path to file specifying the MPE ADC threshold values for each channel
verbosity int #verbosity of the tool
```

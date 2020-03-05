# MrdPaddleEfficiencyCalc

The `MrdPaddleEfficiencyCalc` tool calculates the efficiencies of MRD paddles based on the observed and expected hits found by the `MrdPaddleEfficiencyPreparer` tool. The results are displayed in electronics space, detector space, and channelkey space.

## Data

The `MrdPaddleEfficiencyCalc` tool uses the histograms provided by the `MrdPaddleEfficiencyPreparer` tool, namely

* `expectedhits_layerXX_chkeyYYY`
* `observedhits_layerXX_chkeyYYY`

And calculates the efficiency histograms

* `efficiency_layerXX_chkeyYYY`: Channel-wise efficiency with position resolution
* `efficiency_layerXX_chkeyYYY_avg`: Channel-wise efficiency without position resolution
* `efficiency_layerXX_Y`: Layer-wise efficiencies
* `eff_chankey`: Efficiency as a function of chankey
* `eff_top`: Efficiency in detector space (top view)
* `eff_side`: Efficiency in detector space (side view)
* `canvas_elec`: Efficiency in electronics space 

which are stored in a dedicated output ROOT-file.

## Configuration

The tool uses the following configuration variables

```
InputFile inputfile.root
OutputFile outputfile.root
InactiveFile inactive_channels.dat    #List of inactive MRD channels
verbosity 1
```

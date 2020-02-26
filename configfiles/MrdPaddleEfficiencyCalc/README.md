# MrdPaddleEfficiencyCalc

***********************
# Description
**********************

The `MrdPaddleEfficiencyCalc` toolchain consists only of the `MrdPaddleEfficiencyCalc` tool and in addition uses the `LoadGeometry` tool to load the necessary geometry information. It calculates paddle efficiency values for the MRD paddles based on the `MrdPaddleEfficiency` toolchain.

************************
# ToolChain
************************

```
myLoadGeometry LoadGeometry configfiles/MrdPaddleEfficiencyCalc/LoadGeometryConfig
myMrdPaddleEfficiencyCalc MrdPaddleEfficiencyCalc configfiles/MrdPaddleEfficiencyCalc/MrdPaddleEfficiencyCalcConfig
```

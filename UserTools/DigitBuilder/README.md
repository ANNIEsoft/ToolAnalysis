# Interface

Interface

## Data

This tool is responsible for loading hit information from the ANNIEEvent
store and converting them to RecoDigits.  RecoDigits are the standard hit format
used by all tools in the reconstruction chain.  

By default, all individual PMT true hit times and charges (with time smeared in WCSim)
are loaded into a RecoDigit.  True LAPPD hit times, with a 100 ps time smearing, are
loaded.

The threshold in charge for creating 1 digit can be configured with the `DigitChargeThr` 
variable in the config file.

## Configuration

Describe any configuration variables for Interface.

```
isMC bool

ParametricModel bool

If this bool is set to 1, the PMT parametric model of hits is used to fill RecoDigits.
To form a PMT parametric model hit, the following process is performed for a PMT:
  - Gather all true hits on the PMT in the current event.
  - The parametric hit time is taken as the median of all hit times.
  - The parametric hit charge is taken as the sum of all hit charges.

PhotoDetectorConfiguration string
This configuration is used to decide what digit types are loaded into RecoDigits and
used for reconstruction.  Options are (LAPPD_only, PMT_only, or All)

LAPPDIDFile string         

DigitChargeThr double

ChankeyToPMTIDMap string
This string contains the path to a conversion file between channelkeys and WCSim IDs, to be used with data.

```

# PMTWaveformSim

PMTWaveformSim generates PMT waveforms based on the simulated PMT hits. For each MCHit a lognorm waveform is sampled based on fit parameters extracted from SPE waveforms in data. The fit functions are taken from Daya Bay and are used to model a main peak and two reflections. The fit parameters are sampled using the associated fit errors, and captures the random behavior of a PMT's response. A full extended readout window (70 us) is sampled and waveforms from overlapping hits are added. A random baseline between 300 and 350 ADC is added. Random noise is applied, where the noise sigma is sampled from a gaussian with mean 1 and std dev of 0.25. The baseline and noise envelope values are hardcoded at the moment. Finally, any ADC counts above 4095 are clipped to mimic saturation. The resulting MC waveform hits look like Data hits, and can be passed to tools downstream as if they are Data hits (so long as the configuration is set for those tools).

The implementation of this tool and details on the waveform simulation can be found here: https://annie-docdb.fnal.gov/cgi-bin/sso/RetrieveFile?docid=5962&filename=MC%20Tuning%20and%20Validation%20_%20March%202025%20Collab%20Meeting%20_%20Steven%20Doran.pdf&version=1

Details on the fits used can be found here: https://github.com/S81D/PMTPulseFit

## Data

**RawADCDataMC** `std::map<unsigned long, std::vector<MCWaveform<uint16_t>> >`
* The raw simulated waveforms. The key is PMT channel number, then inner vector should always have size == 1

**CalibratedADCData** `std::map<unsigned long, std::vector<CalibratedADCWaveform<double>> >`
* The "calibrated" simulated waveforms. The baseline and sigma are the true, randomly-generated values. The key is PMT channel number, then inner vector should always have size == 1


## Configuration

`PMTWaveformSim` uses a fit file with SPE fits extracted from the data for each PMT. Any toolchain using this tool must have a `PMTWaveformLognormFit.csv` file in that toolchain directory.

```
verbosity 0
PMTParameterFile configfiles/PMTWaveformSim/PMTWaveformLognormFit.csv # file containing the fit parameters and uncertainties
useTimeSmearing 0   # instead of the default WCSim time smearing, add in realistic time smearing based on the PMT laser analysis
Prewindow 80        # number of clock ticks before the MC hit time to begin sampling the fit function
ReadoutWindow 200   # number of clock ticks around the MC hit time over which waveforms are sampled
T0Offset 25         # A timing offset (in clock ticks) that can be used to vary the pulse start times
                    # (it is primarily used to ensure the lognorm fits on the main peak avoid NaNs when sampling the fit params)
                    # (The T0offset set to 25 shifts the sampling to avoid fit blowups, then the code shifts it back by 25. 25 is therefore the "default" and is recommended.)
TimeShift 0         # This is an artificial time shift to the MC Hits [ns]
                    # (as WCSim simulates particles from t0 = 0, this can be used as a toggle on the "trigger time")
MakeDebugFile 0     # Produce a root file containing all the simulated waveforms
```

## Other notes
#### CAUTION: In line 90, the RNG seed is set in Initialization based on the clock time. If any other downstream tools are also setting a random seed, there may be unexpected behavior as a result of this tool.

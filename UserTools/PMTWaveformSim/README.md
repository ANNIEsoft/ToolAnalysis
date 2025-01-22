# PMTWaveformSim

PMTWaveformSim generates PMT waveforms based on the simulated PMT hits. For each MCHit a lognorm waveform is sampled based on fit parameters extracted from SPE waveforms in data. The fit parameters are randomly sampled in a way that conserved the covariance between parameters and captures the random behavior of a PMT's response. A full extended readout window (70 us) is sampled and waveforms from overlapping hits are added. A random baseline between 300 and 350 ADC is added. Random noise is applied, where the noise sigma is sampled from a gaussian with mean 1 and std dev of 0.25. The baseline and noise envelope values are hardcoded at the moment. Finally, any ADC counts above 4095 are clipped to mimick saturation. 

## Data

**RawADCDataMC** `std::map<unsigned long, std::vector<MCWaveform<uint16_t>> >`
* The raw simulated waveforms. The key is PMT channel number, then inner vector should always have size == 1

**CalibratedADCData** `std::map<unsigned long, std::vector<CalibratedADCWaveform<double>> >`
* The "calibrated" simulated waveforms. The baseline and sigma are the true, randomly-generated values. The key is PMT channel number, then inner vector should always have size == 1


## Configuration

```
PMTParameterFile configfiles/PMTWaveformSim/PMTWaveformLognormFit.csv # file containing the fit parameters and triangular Cholesky decomposed covariance matrix
Prewindow 10 # number of clock ticks before the MC hit time to begin sampling the fit function
ReadoutWindow 35 # number of clock ticks around the MC hit time over which waveforms are sampled
T0Offset 0 # A timing offset (in clock ticks) that can be used to align the pulse start time (can be positive or negative)
MakeDebugFile 0 # Produce a root file containing all the simulated waveforms
```

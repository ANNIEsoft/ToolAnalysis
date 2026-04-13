#ifndef PMTWaveformSim_H
#define PMTWaveformSim_H

#include <string>
#include <iostream>

// ANNIE includes
#include "Tool.h"
#include "CalibratedADCWaveform.h"
#include "Waveform.h"

// ROOT includes
#include "TFile.h"
#include "TRandom3.h"

struct PMTFitParams
{
  double  p0, p1, p2, T1, T2, R1, R2;
  double up0, up1, up2, uT1, uT2, uR1, uR2;
};


/**
 * \class PMTWaveformSim
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: D. Ajana $
* $Date: 2024/11/05 10:44:00 $
* Contact: dja23@fsu.edu
*/
class PMTWaveformSim: public Tool {


 public:

  PMTWaveformSim(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  int LoadFromStores(); ///< Fetch all necessary info from the CStore

  bool LoadPMTParameters(); ///< load in:  1. lognorm fit information from the csv file and 2. PMT timing uncertainties from laser analysis
  bool SampleFitParameters(int pmtid);  ///< sample fit parameters in a way that preserves covariance
  uint16_t CustomLogNormalPulse(double hit_t0, uint16_t t0_clocktick, double hit_charge);  //< construct simulated ADC pulses using sampled fit values
  void ConvertMapToWaveforms(const std::map<uint16_t, uint16_t> &sample_map,  ///< construct a waveform with the simulated pulse + baseline modulation, to feed into the PhaseIIADCHitFinder
			     std::vector<Waveform<uint16_t>> &rawWaveforms,
			     std::vector<CalibratedADCWaveform<double>> &calWaveforms,
			     double noiseSigma, int baseline);

  void FillDebugGraphs(const std::map<unsigned long, std::vector<Waveform<uint16_t>> > &RawADCDataMC);   ///< debugging
  bool TimeSmearing(int pmtid);   ///< prior to sampling the fits, we can add realistic time smearing (instead of relying on WCSim's time smearing) to the MCHit (true) time
    
 private:

  // To load from the ANNIEEvent
  std::map<unsigned long, std::vector<MCHit>> *fMCHits = nullptr;

  Geometry *fGeo = nullptr;

  // Config variables
  uint16_t fPrewindow;                         // number of clock ticks before the MC hit time to begin sampling the fit function
  uint16_t fReadoutWindow;                     // number of clock ticks around the MC hit time over which waveforms are sampled
  int fT0Offset;                               // a timing offset (in clock ticks) that can be used to align the pulse start time (can be positive or negative)
  bool fuseTimeSmearing;                       // whether to implement time smearing based on the uncertainties extracted from the PMT laser timing analysis
                                               //       (this should be used in place of the standard WCSim time smearing)
  double fTimeShift;                           // [ns] to shift MC hit times by - if generating WCSim particles, there is often a pileup at t = 0. When doing the hit integration,
                                               //       this can artificially shorten the "pre"-pulse integration (doesn't typically happen in data).

  std::string fPMTParameterFile;
  
  TRandom3 *fRandom;

  std::map<int, PMTFitParams> fPMTParamMap;
  std::map<int, double> fPMTJitterMap;
  double fP0, fP1, fP2;                        // main peak parameters
  double fT1, fT2, fR1, fR2;                   // reflection amplitudes and time spacings
  double fTimeSmear;


  bool fDebug;
  TFile *fOutFile;
  
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
};


#endif

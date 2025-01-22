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
  double  p0; double  p1; double  p2;
  double u00;
  double u10; double u11; 
  double u20; double u21; double u22;
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
  int LoadFromStores();

  bool LoadPMTParameters();
  bool SampleFitParameters(int pmtid);
  uint16_t CustomLogNormalPulse(double hit_t0, uint16_t t0_clocktick, double hit_charge);
  void ConvertMapToWaveforms(const std::map<uint16_t, uint16_t> &sample_map,
			     std::vector<Waveform<uint16_t>> &rawWaveforms,
			     std::vector<CalibratedADCWaveform<double>> &calWaveforms,
			     double noiseSigma, int baseline);

  void FillDebugGraphs(const std::map<unsigned long, std::vector<Waveform<uint16_t>> > &RawADCDataMC);
    
 private:

  // To load from the ANNIEEvent
  std::map<unsigned long, std::vector<MCHit>> *fMCHits = nullptr;

  Geometry *fGeo = nullptr;

  // Config variables
  uint16_t fPrewindow;
  uint16_t fReadoutWindow;
  int fT0Offset;
  std::string fPMTParameterFile;
  
  TRandom3 *fRandom;

  std::map<int, PMTFitParams> fPMTParamMap;
  double fP0, fP1, fP2;
  
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

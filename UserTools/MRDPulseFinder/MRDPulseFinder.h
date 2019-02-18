#ifndef MRDPulseFinder_H
#define MRDPulseFinder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Waveform.h"
#include "ADCPulse.h"
#include "CalibratedADCWaveform.h"

class MRDPulseFinder: public Tool {


 public:

  MRDPulseFinder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  std::vector<ADCPulse> FindPulse(vector<short unsigned int>* someWave);


 private:

   int evnum;
   double thresh;
   double deltat;
   double bline;
   double sigbline;
   int channelno;
   int minibuffernum;
   std::map<unsigned long, std::vector<Waveform<unsigned short>>> rawadcdata;
   std::map<unsigned long, std::vector<CalibratedADCWaveform<double>>> caladc;


};


#endif

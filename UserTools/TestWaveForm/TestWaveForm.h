#ifndef TestWaveForm_H
#define TestWaveForm_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Waveform.h"

class TestWaveForm: public Tool {


 public:

  TestWaveForm();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  std::string GetStatus(detectorstatus detectorStatus);
  std::string GetStatus(channelstatus channelStatus);
  double IsItZero(double number);


 private:
    Geometry* _geom = nullptr;
   std::map<unsigned long, Waveform<double> >* _waveforms = nullptr;




};


#endif

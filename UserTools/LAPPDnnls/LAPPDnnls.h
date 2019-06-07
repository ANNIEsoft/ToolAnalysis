#ifndef LAPPDnnls_H
#define LAPPDnnls_H

#include <string>
#include <iostream>
#include "nnls.h"

#include "Tool.h"

class LAPPDnnls: public Tool {


 public:

  LAPPDnnls();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  nsNNLS::matrix* BuildTemplateMatrix(Waveform<double> tempwave, size_t nrows); //makes the nnls matrix A given a root template file
  nsNNLS::vector* BuildWaveformVector(Waveform<double> wave, vector<float> times, double template_timestep); //formats the waveform into the vector format expected by nnls algo




 private:





};


#endif

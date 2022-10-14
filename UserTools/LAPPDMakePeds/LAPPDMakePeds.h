#ifndef LAPPDMakePeds_H
#define LAPPDMakePeds_H

#include <string>
#include <iostream>
#include <fstream>
#include "Tool.h"
#include "TH1.h"
#include "TFile.h"
#include "TString.h"
#include "TF1.h"
#include "TTree.h"


/**
 * \class LAPPDMakePeds
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDMakePeds: public Tool {


 public:

  LAPPDMakePeds(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  //TFile* pedrootfile;

 private:

   std::map<unsigned long, vector<TH1D>>* pedhists;

   string InputWavLabel;
   string PedOutFname;
   int verbostiylevel;
   int NChannels;
   Geometry* _geom;
   int eventcount;




};


#endif

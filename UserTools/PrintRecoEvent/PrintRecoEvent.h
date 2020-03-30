#ifndef PrintRecoEvent_H
#define PrintRecoEvent_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "RecoCluster.h"


/**
 * \class PrintRecoEvent
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class PrintRecoEvent: public Tool {


 public:

  PrintRecoEvent(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  int verbosity;
  bool isMC;

  int mcpi0count;
  int mcpipluscount;
  int mcpiminuscount;
  int mck0count;
  int mckpluscount;
  int mckminuscount;

  RecoVertex *truevertex = nullptr;
  RecoVertex *truestopvertex = nullptr;
  std::vector<RecoDigit>* recodigits = nullptr;
  double truemuonenergy;
  double truetracklengthinwater;
  double truetracklengthinmrd;
  bool singleringevent;
  bool multiringevent;  
  int pdgprimary;
  int nrings;
  std::vector<int> indexparticlesring;
  int projectedmrdhit;

  int eventcutstatus;
  int eventflagapplied;
  int eventflagged;

  std::vector<RecoVertex*> vseedvtxlist;
  std::vector<double> vseedfomlist;

  bool hitcleaningdone;
  std::vector<RecoCluster*>* hitcleaningclusters = nullptr;
  std::map<std::string,double>* hitcleaningparam = nullptr;

  RecoVertex *simpleposition = nullptr;
  RecoVertex *pointposition = nullptr;
  RecoVertex *simpledirection = nullptr;
  RecoVertex *pointdirection = nullptr;
  RecoVertex *pointvertex = nullptr;
  RecoVertex *extendedvertex = nullptr;  


  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;

};


#endif

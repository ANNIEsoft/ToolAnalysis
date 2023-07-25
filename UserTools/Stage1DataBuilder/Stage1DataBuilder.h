#ifndef Stage1DataBuilder_H
#define Stage1DataBuilder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "BoostStore.h"
#include "PsecData.h"
#include "ADCPulse.h"


/**
 * \class Stage1DataBuilder
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class Stage1DataBuilder: public Tool {


 public:

  Stage1DataBuilder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  void GetANNIEEvent();


 private:
    int verbosity;
    int v_debug = 3;

    std::string Basename;
    BoostStore* Stage1Data = nullptr;
    BoostStore* LAPPD1Data = nullptr;
    BoostStore* ANNIEEvent1 = nullptr;





};


#endif

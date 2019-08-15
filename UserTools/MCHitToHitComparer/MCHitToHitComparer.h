#ifndef MCHitToHitComparer_H
#define MCHitToHitComparer_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Hit.h"

/**
 * \class MCHitToHitComparer
 *
 * This tool is used to print time and charge information from the Hits and MCHits maps in the
 * ANNIEEvent store.  Additional functionality could be added to add hits and MCHits into
 * Histograms and show the time/charge distributions in the Finalise loop. 
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class MCHitToHitComparer: public Tool {


 public:

  MCHitToHitComparer(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:
  std::map<unsigned long,std::vector<Hit>>* hit_map=nullptr;
  std::map<unsigned long,std::vector<MCHit>>* mchit_map=nullptr;

  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=1;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
};


#endif

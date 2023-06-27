#ifndef FilterLAPPDEvents_H
#define FilterLAPPDEvents_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"
#include "BoostStore.h"
#include "TriggerClass.h"
#include "BeamStatus.h"
#include "PsecData.h"
#include "Hit.h"
#include "ADCPulse.h"


/**
 * \class FilterLAPPDEvents
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class FilterLAPPDEvents: public Tool {


 public:

  FilterLAPPDEvents(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
	void SetAndSaveEvent();
	std::vector<std::vector<std::string>> LoadDetailedRunCSV(std::string filename);


 private:

	std::string DesiredCuts;
	std::string FilteredFilesBasename;
	std::string SavePath;
	std::string csvInputFile;
    std::string RangeOfRuns;
	int verbosity;
    BoostStore* FilteredEvents = nullptr;
    std::vector<std::string> CSVList;
    std::vector<std::vector<std::string>> DetailedRunInfo;
//    size_t current_csv;
    int matched;
    std::vector<std::vector<int>> testvector;



};


#endif

#ifndef LoadGeometry_H
#define LoadGeometry_H

#include <string>
#include <iostream>

#include "Tool.h"

class LoadGeometry: public Tool {


 public:

  LoadGeometry();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  bool FileExists(const std::string& name);
  std::string GetKeyLine(const std::string& name);


 private:
  std::string fFACCMRDGeoFile;
  std::string fTankPMTGeoFile;
  std::string fVetoPMTGeoFile;
  std::string fLAPPDGeoFile;
  std::string fDetectorGeoFile;
  
	//verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
};


#endif

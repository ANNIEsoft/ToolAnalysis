#ifndef LoadGeometry_H
#define LoadGeometry_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Geometry.h"

class LoadGeometry: public Tool {


 public:

  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  bool FileExists(const std::string& name);
  std::string GetKeyLine(const std::string& name);
  ConstructGeometry();
  InitializeGeometry();
  LoadFACCMRDDetectors();

  Geometry* AnnieGeometry;

 private:
  std::string fFACCMRDGeoFile;
  std::string fTankPMTGeoFile;
  std::string fVetoPMTGeoFile;
  std::string fLAPPDGeoFile;
  std::string fDetectorGeoFile;
  
  //Labels used in Geometry files to mark the legend and data entries
  std::string LegendLineLabel = "LEGEND_LINE";
  std::string DataStartLineLabel = "DATA_START";
  std::string DataEndLineLabel = "DATA_END";


  //Vector of strings indicating variables of interest and their data types in
  //The MRD file.  Used in the LoadFACCMRDDetectors() method
  std::string[20] MRDIntegerValues = {"detector_num","channel_num","detector_system","orientation","layer","side","num",
                                   "rack","TDC slot","TDC channel","discrim_slot","discrim_ch",
                                   "patch_panel_row","patch_panel_col","amp_slot","amp_channel",
                                   "hv_crate","hv_slot","hv_channel","nominal_HV","polarity"};
  std::string[6] MRDDoubleValues = {"x_center","y_center","z_center","x_width","y_width","z_width"};
  std::string[3] MRDStringValues = {"PMT_type","cable_label","paddle_label"};

	//verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
};


#endif

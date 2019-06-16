#ifndef LoadGeometry_H
#define LoadGeometry_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Geometry.h"
#include <boost/algorithm/string.hpp>

class LoadGeometry: public Tool {


 public:

  LoadGeometry();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  bool FileExists(std::string name);
  std::string GetLegendLine(std::string name);
  void InitializeGeometry();
  void LoadFACCMRDDetectors();
  Detector  ParseMRDDataEntry(std::vector<std::string> SpecLine, 
                               std::vector<std::string> MRDLegendEntries);
  Geometry* AnnieGeometry;

 private:
  std::string fFACCMRDGeoFile;
  std::string fTankPMTGeoFile;
  std::string fLAPPDGeoFile;
  std::string fDetectorGeoFile;
  
  //Labels used in Geometry files to mark the legend and data entries
  std::string LegendLineLabel = "LEGEND_LINE";
  std::string DataStartLineLabel = "DATA_START";
  std::string DataEndLineLabel = "DATA_END";


  //Vector of strings indicating variables of interest and their data types in
  //The MRD file.  Used in the LoadFACCMRDDetectors() method
  std::vector<std::string> MRDIntegerValues{"detector_num","channel_num","detector_system","orientation","layer","side","num",
                                   "rack","TDC slot","TDC channel","discrim_slot","discrim_ch",
                                   "patch_panel_row","patch_panel_col","amp_slot","amp_channel",
                                   "hv_crate","hv_slot","hv_channel","nominal_HV","polarity"};
  std::vector<std::string> MRDDoubleValues{"x_center","y_center","z_center","x_width","y_width","z_width"};
  std::vector<std::string> MRDStringValues{"PMT_type","cable_label","paddle_label","notes"};

	//verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=1;
  int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
};


#endif

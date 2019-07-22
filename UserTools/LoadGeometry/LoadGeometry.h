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
  void LoadLAPPDs();
  bool  ParseMRDDataEntry(std::vector<std::string> SpecLine,
                               std::vector<std::string> MRDLegendEntries);
  bool  ParseLAPPDDataEntry(std::vector<std::string> SpecLine,
                              std::vector<std::string> LAPPDLegendEntries);
  Geometry* AnnieGeometry;
  int detector_num_store = 0;
  int counter = 0;
  Detector* adet;
  int LAPPD_channel_count;

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

  //Vector of strings indicating variables of interest and their data types in
  //The LAPPD file.  Used in the LoadLAPPDs() method
  std::vector<std::string> LAPPDIntegerValues{"detector_num","channel_strip_side","channel_strip_num"};
  std::vector<std::string> LAPPDDoubleValues{"detector_position_x","detector_position_y","detector_position_z",
                                             "detector_direction_x","detector_direction_y","detector_direction_z",
                                             "channel_position_x","channel_position_y","channel_position_z"};
  std::vector<std::string> LAPPDStringValues{"detector_type","detector_status","channel_status"};
  std::vector<std::string> LAPPDUnIntValues{"channel_signal_crate","channel_signal_card","channel_signal_channel",
                                            "channel_level2_crate","channel_level2_card","channel_level2_channel",
                                            "channel_hv_crate","channel_hv_card","channel_hv_channel","channel_num"};

	//verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=1;
  int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
};


#endif

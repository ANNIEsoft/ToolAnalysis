#include "LoadGeometry.h"

LoadGeometry::LoadGeometry():Tool(){}


bool LoadGeometry::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // Make the ANNIEEvent Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("ANNIEEvent");
  if(recoeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  m_variables.Get("verbosity", verbosity);
  m_variables.Get("FACCMRDGeoFile", fFACCMRDGeoFile);
  m_variables.Get("TankPMTGeoFile", fTankPMTGeoFile);
  m_variables.Get("LAPPDGeoFile", fLAPPDGeoFile);
  m_variables.Get("DetectorGeoFile", fDetectorGeoFile);

  //Check files exist 
  if(!this->FileExists(fDetectorGeoFile)){
    Log("LoadGeometry Tool: File for Detector Geometry does not exist!",v_error,verbosity); 
    std::cout << "Filepath was... " << fDetectorGeoFile << std::endl;
    return false;
  }
  if(!this->FileExists(fFACCMRDGeoFile)){
    Log("LoadGeometry Tool: File for FACC/MRD Geometry does not exist!",v_error,verbosity);
    std::cout << "Filepath was... " << fFACCMRDGeoFile << std::endl;
    return false;
  }
  if(!this->FileExists(fTankPMTGeoFile)){
    Log("LoadGeometry Tool: File for Tank PMT Geometry does not exist!",v_error,verbosity);
    std::cout << "Filepath was... " << fTankPMTGeoFile << std::endl;
    return false;
  }

  //Make the map of channel key to crate space info
  CrateSpaceToChannelNumMap = new std::map<std::vector<int>,int>;

  //Initialize the geometry using the geometry CSV file entries 
  this->InitializeGeometry();

  //Load MRD Geometry Detector/Channel Information
  this->LoadFACCMRDDetectors();

  //Load TankPMT Geometry Detector/Channel Information
  this->LoadTankPMTDetectors();

  m_data->Stores.at("ANNIEEvent")->Header->Set("AnnieGeometry",AnnieGeometry,true);

  m_data->CStore.Set("CrateSpaceToChannelNumMap",CrateSpaceToChannelNumMap);
  
  //AnnieGeometry->GetChannel(0); // trigger InitChannelMap
  
  return true;
}


bool LoadGeometry::Execute(){
  return true;
}


bool LoadGeometry::Finalise(){
  std::cout << "LoadGeometry tool exitting" << std::endl;
  return true;
}


void LoadGeometry::InitializeGeometry(){
  Log("LoadGeometry tool: Now loading DetectorGeoFile and initializing geometry",v_message,verbosity);
  //Get the Detector file data key
  std::string DetectorLegend = this->GetLegendLine(fDetectorGeoFile);
  std::vector<std::string> DetectorLegendEntries;
  boost::split(DetectorLegendEntries,DetectorLegend, boost::is_any_of(","), boost::token_compress_on); 
 
  //Initialize at zero; will be set later after channels are loaded 
  int numtankpmts = 0;
  int numlappds = 0;
  int nummrdpmts = 0; 
  int numvetopmts = 0;

  //Initialize data that will be fed to Geometry (units in meters)
  int geometry_version;
  double tank_xcenter,tank_ycenter,tank_zcenter; 
  double tank_radius,tank_halfheight, pmt_enclosed_radius, pmt_enclosed_halfheight;
  double mrd_width,mrd_height,mrd_depth,mrd_start;

  std::string line;
  ifstream myfile(fDetectorGeoFile.c_str());
  if (myfile.is_open()){
    //First, get to where data starts
    while(getline(myfile,line)){
      if(line.find("#") != std::string::npos) continue;
      if(line.find(DataStartLineLabel)!=std::string::npos) break;
    }
    //Loop over lines, collect all detector data (should only be one line here)
    while(getline(myfile,line)){
      std::cout << line << std::endl; //has our stuff;
      if(line.find("#")!=std::string::npos) continue;
      if(line.find(DataEndLineLabel)!=std::string::npos) break;
      std::vector<std::string> DataEntries;
      boost::split(DataEntries,line, boost::is_any_of(","), boost::token_compress_on); 
      for (int i=0; i<DataEntries.size(); i++){
        //Check Legend at i, load correct data type
        int ivalue;
        double dvalue;
        if(DetectorLegendEntries.at(i) == "geometry_version") ivalue = std::stoi(DataEntries.at(i));
        else dvalue = std::stod(DataEntries.at(i));
        if (DetectorLegendEntries.at(i) == "geometry_version") geometry_version = ivalue;
        if (DetectorLegendEntries.at(i) == "tank_xcenter") tank_xcenter = dvalue;
        if (DetectorLegendEntries.at(i) == "tank_ycenter") tank_ycenter = dvalue;
        if (DetectorLegendEntries.at(i) == "tank_zcenter") tank_zcenter = dvalue;
        if (DetectorLegendEntries.at(i) == "tank_radius") tank_radius = dvalue;
        if (DetectorLegendEntries.at(i) == "tank_halfheight") tank_halfheight = dvalue;
        if (DetectorLegendEntries.at(i) == "pmt_enclosed_radius") pmt_enclosed_radius = dvalue;
        if (DetectorLegendEntries.at(i) == "pmt_enclosed_halfheight") pmt_enclosed_halfheight = dvalue;
        if (DetectorLegendEntries.at(i) == "mrd_width") mrd_width = dvalue;
        if (DetectorLegendEntries.at(i) == "mrd_height") mrd_height = dvalue;
        if (DetectorLegendEntries.at(i) == "mrd_depth") mrd_depth = dvalue;
        if (DetectorLegendEntries.at(i) == "mrd_start") mrd_start = dvalue;
      } 
    }
    Position tank_center(tank_xcenter, tank_ycenter, tank_zcenter);
    // Initialize the Geometry
    AnnieGeometry = new Geometry(geometry_version,
                                 tank_center,
                                 tank_radius,
                                 tank_halfheight,
                                 pmt_enclosed_radius,
                                 pmt_enclosed_halfheight,
                                 mrd_width,
                                 mrd_height,
                                 mrd_depth,
                                 mrd_start,
                                 numtankpmts,
                                 nummrdpmts,
                                 numvetopmts,
                                 numlappds,
                                 geostatus::FULLY_OPERATIONAL);
  } else {
    Log("LoadGeometry tool: Something went wrong opening a file!!!",v_error,verbosity);
  }
  myfile.close();
}

void LoadGeometry::LoadFACCMRDDetectors(){
  //First, get the MRD file data key
  Log("LoadGeometry tool: Now loading FACC/MRD detectors",v_message,verbosity);
  std::string MRDLegend = this->GetLegendLine(fFACCMRDGeoFile);
  std::vector<std::string> MRDLegendEntries;
  boost::split(MRDLegendEntries,MRDLegend, boost::is_any_of(","), boost::token_compress_on); 
 
  std::string line;
  ifstream myfile(fFACCMRDGeoFile.c_str());
  if (myfile.is_open()){
    //First, get to where data starts
    while(getline(myfile,line)){
      if(line.find("#")!=std::string::npos) continue;
      if(line.find(DataStartLineLabel)!=std::string::npos) break;
    }
    //Loop over lines, collect all detector specs 
    while(getline(myfile,line)){
      std::cout << line << std::endl; //has our stuff;
      if(line.find("#")!=std::string::npos) continue;
      if(line.find(DataEndLineLabel)!=std::string::npos) break;
      std::vector<std::string> SpecLine;
      boost::split(SpecLine,line, boost::is_any_of(","), boost::token_compress_on); 
      if(verbosity>4) std::cout << "This line of data: " << line << std::endl;
      //Parse data line, make corresponding detector/channel
      bool add_ok = this->ParseMRDDataEntry(SpecLine,MRDLegendEntries);
      if(not add_ok){
        std::cerr<<"Faild to add Detector to Geometry!"<<std::endl;
      }
    }
  } else {
    Log("LoadGeometry tool: Something went wrong opening a file!!!",v_error,verbosity);
  }
  if(myfile.is_open()) myfile.close();
    Log("LoadGeometry tool: FACC/MRD Detector/Channel loading complete",v_message,verbosity);
}

bool LoadGeometry::ParseMRDDataEntry(std::vector<std::string> SpecLine,
        std::vector<std::string> MRDLegendEntries){
  //Parse the line for information needed to fill the detector & channel classes
  int detector_num,channel_num,detector_system,orientation,layer,side,num,
      rack,TDC_slot,TDC_channel,discrim_slot,discrim_ch,
      patch_panel_row,patch_panel_col,amp_slot,amp_channel,
      hv_crate,hv_slot,hv_channel,nominal_HV,polarity;
  double x_center,y_center,z_center,x_width,y_width,z_width;
  std::string PMT_type,cable_label,paddle_label;

  //Search for Legend entry.  Fill value type if found.
  Log("LoadGeometry tool: parsing data line into variables",v_debug,verbosity);
  for (int i=0; i<SpecLine.size(); i++){
    int ivalue;
    double dvalue;
    std::string svalue;
    for (int j=0; j<MRDIntegerValues.size(); j++){
      if(MRDLegendEntries.at(i) == MRDIntegerValues.at(j)){
        ivalue = std::stoi(SpecLine.at(i));
        break;
      }
    }
    for (int j=0; j<MRDStringValues.size(); j++){
      if(MRDLegendEntries.at(i) == MRDStringValues.at(j)){
        svalue = SpecLine.at(i);
        break;
      }
    }
    for (int j=0; j<MRDDoubleValues.size(); j++){
      if(MRDLegendEntries.at(i) == MRDDoubleValues.at(j)){
        dvalue = std::stod(SpecLine.at(i));
        break;
      }
    }
    //Integers
    if (MRDLegendEntries.at(i) == "detector_num") detector_num = ivalue;
    if (MRDLegendEntries.at(i) == "channel_num") channel_num = ivalue;
    if (MRDLegendEntries.at(i) == "detector_system") detector_system = ivalue;
    if (MRDLegendEntries.at(i) == "orientation") orientation = ivalue;
    if (MRDLegendEntries.at(i) == "layer") layer = ivalue;
    if (MRDLegendEntries.at(i) == "side") side = ivalue;
    if (MRDLegendEntries.at(i) == "num") num = ivalue;
    if (MRDLegendEntries.at(i) == "rack") rack = ivalue;
    if (MRDLegendEntries.at(i) == "TDC_slot") TDC_slot = ivalue;
    if (MRDLegendEntries.at(i) == "TDC_channel") TDC_channel = ivalue;
    if (MRDLegendEntries.at(i) == "discrim_slot") discrim_slot = ivalue;
    if (MRDLegendEntries.at(i) == "discrim_ch") discrim_ch = ivalue;
    if (MRDLegendEntries.at(i) == "patch_panel_row") patch_panel_row = ivalue;
    if (MRDLegendEntries.at(i) == "patch_panel_col") patch_panel_col = ivalue;
    if (MRDLegendEntries.at(i) == "amp_slot") amp_slot = ivalue;
    if (MRDLegendEntries.at(i) == "amp_channel") amp_channel = ivalue;
    if (MRDLegendEntries.at(i) == "hv_crate") hv_crate = ivalue;
    if (MRDLegendEntries.at(i) == "hv_slot") hv_slot = ivalue;
    if (MRDLegendEntries.at(i) == "hv_channel") hv_channel = ivalue;
    if (MRDLegendEntries.at(i) == "nominal_HV") nominal_HV = ivalue;
    if (MRDLegendEntries.at(i) == "polarity") polarity = ivalue;
    //Doubles
    if (MRDLegendEntries.at(i) == "x_center") x_center = dvalue;
    if (MRDLegendEntries.at(i) == "y_center") y_center = dvalue;
    if (MRDLegendEntries.at(i) == "z_center") z_center = dvalue;
    if (MRDLegendEntries.at(i) == "x_width") x_width = dvalue;
    if (MRDLegendEntries.at(i) == "y_width") y_width = dvalue;
    if (MRDLegendEntries.at(i) == "z_width") z_width = dvalue;
    //Strings
    if (MRDLegendEntries.at(i) == "PMT_type") PMT_type = svalue;
    if (MRDLegendEntries.at(i) == "paddle_label") paddle_label = svalue;
    if (MRDLegendEntries.at(i) == "cable_label") cable_label = svalue;
  } 

  // Parse whether this is an MRD or Veto Paddle
  std::string dettype = "unknown";
  if (detector_system == 0) dettype = "Veto";
  else if (detector_system == 1) dettype = "MRD";
  //FIXME Need the direction of the MRD PMT
  //FIXME: things that are not loaded in with the default det/channel format:
  //  - discrim_slot, discrim_ch
  //  - patch_panel_row, patch_panel_col, amp_slot, amp_channel
  //  - nominal_HV, polarity
  //  - cable_label, paddle_label
  //
  if(verbosity>4) std::cout << "Filling a FACC/MRD data line into Detector/Channel classes" << std::endl;
  Detector adet(detector_num,
                dettype,
                "MRD", //Change to orientation for PaddleDetector class?
                Position( x_center/100.,
                          y_center/100.,
                          z_center/100.),
                Direction(0.,
                          0.,
                          0.),
                PMT_type,
                detectorstatus::ON,
                0.);

  int MRD_x, MRD_y, MRD_z;
  // orientation 0=horizontal, 1=vertical
  MRD_x = (orientation) ? num  : side;
  MRD_y = (orientation) ? side : num;
  // veto layers are both cabled as z=0, with the layers differentiated by x=0, x=1
  // in practice of course, both span the same x, but are offset in z.
  if(layer>0) MRD_z = layer;
  else        MRD_z = side;
  
  Paddle apad( detector_num,
               MRD_x,
               MRD_y,
               MRD_z,
               orientation,
               Position( x_center/100.,
                         y_center/100.,
                         z_center/100.),
               std::pair<double,double>{x_center-(x_width/200.), x_center+(x_width/200.)},
               std::pair<double,double>{y_center-(y_width/200.), y_center+(y_width/200.)},
               std::pair<double,double>{z_center-(z_width/200.), z_center+(z_width/200.)});
  
  Channel pmtchannel( channel_num,
                      Position(0,0,0.),
                      -1, // stripside
                      -1, // stripnum
                      rack,
                      TDC_slot,
                      TDC_channel,
                      -1,                 // TDC has no level 2 signal handling
                      -1,
                      -1,
                      hv_crate,
                      hv_slot,
                      hv_channel,
                      channelstatus::ON);
  
  // Add this channel to the geometry
  if(verbosity>4) cout<<"Adding channel "<<channel_num<<" to detector "<<detector_num<<endl;
  adet.AddChannel(pmtchannel);

  // Also add this channel to the electronics map
  std::vector<int> crate_map{rack,TDC_slot,TDC_channel};
  if(CrateSpaceToChannelNumMap->count(crate_map)==0){
    CrateSpaceToChannelNumMap->emplace(crate_map, channel_num);
  } else {
    Log("LoadGeometry Tool: ERROR: Tried assigning a channel_num to a crate space already defined!!! ",v_error, verbosity);
  }

  if(verbosity>5) cout<<"Adding detector to Geometry"<<endl;
  AnnieGeometry->AddDetector(adet);
  if(verbosity>4) cout<<"Adding paddle to Geometry"<<endl;
  AnnieGeometry->SetDetectorPaddle(detector_num, apad);
  return true;
}

void LoadGeometry::LoadTankPMTDetectors(){
  //First, get the Tank PMT file legend key
  Log("LoadGeometry tool: Now loading TankPMT detectors",v_message,verbosity);
  std::string TankPMTLegend = this->GetLegendLine(fTankPMTGeoFile);
  std::vector<std::string> TankPMTLegendEntries;
  boost::split(TankPMTLegendEntries,TankPMTLegend, boost::is_any_of(","), boost::token_compress_on); 
 
  std::string line;
  ifstream myfile(fTankPMTGeoFile.c_str());
  if (myfile.is_open()){
    //First, get to where data starts
    while(getline(myfile,line)){
      if(line.find("#")!=std::string::npos) continue;
      if(line.find(DataStartLineLabel)!=std::string::npos) break;
    }
    //Loop over lines, collect all detector specs 
    while(getline(myfile,line)){
      std::cout << line << std::endl; //has our stuff;
      if(line.find("#")!=std::string::npos) continue;
      if(line.find(DataEndLineLabel)!=std::string::npos) break;
      std::vector<std::string> SpecLine;
      boost::split(SpecLine,line, boost::is_any_of(","), boost::token_compress_on); 
      if(verbosity>4) std::cout << "This line of data: " << line << std::endl;
      //Parse data line, make corresponding detector/channel
      bool add_ok = this->ParseTankPMTDataEntry(SpecLine,TankPMTLegendEntries);
      if(not add_ok){
        std::cerr<<"Failed to add Tank PMT Detector to Geometry!"<<std::endl;
      }
    }
  } else {
    Log("LoadGeometry tool: Something went wrong opening the Tank PMT file!!!",v_error,verbosity);
  }
  if(myfile.is_open()) myfile.close();
    Log("LoadGeometry tool: Tank PMT Detector/Channel loading complete",v_message,verbosity);
}

bool LoadGeometry::ParseTankPMTDataEntry(std::vector<std::string> SpecLine,
        std::vector<std::string> TankPMTLegendEntries){

  //Parse the line for information needed to fill the Tdetector & channel classes
  int detector_num,channel_num,panel_number,signal_crate,signal_slot,signal_channel,
      mt_crate, mt_slot, mt_channel, hv_crate,hv_slot,hv_channel,nominal_HV,polarity;
  double x_pos,y_pos,z_pos,x_dir,y_dir,z_dir;
  std::string detector_tank_location,PMT_type,cable_label,detector_status;

  //Search for Legend entry.  Fill value type if found.
  Log("LoadGeometry tool: parsing data line into variables",v_debug,verbosity);
  for (int i=0; i<SpecLine.size(); i++){
    int ivalue;
    double dvalue;
    std::string svalue;
    for (int j=0; j<TankPMTIntegerValues.size(); j++){
      if(TankPMTLegendEntries.at(i) == TankPMTIntegerValues.at(j)){
        ivalue = std::stoi(SpecLine.at(i));
        break;
      }
    }
    for (int j=0; j<TankPMTStringValues.size(); j++){
      if(TankPMTLegendEntries.at(i) == TankPMTStringValues.at(j)){
        svalue = SpecLine.at(i);
        break;
      }
    }
    for (int j=0; j<TankPMTDoubleValues.size(); j++){
      if(TankPMTLegendEntries.at(i) == TankPMTDoubleValues.at(j)){
        dvalue = std::stod(SpecLine.at(i));
        break;
      }
    }

    //Integers
    if (TankPMTLegendEntries.at(i) == "detector_num") detector_num = ivalue;
    if (TankPMTLegendEntries.at(i) == "channel_num") channel_num = ivalue;
    if (TankPMTLegendEntries.at(i) == "panel_number") panel_number = ivalue;
    if (TankPMTLegendEntries.at(i) == "signal_crate") signal_crate = ivalue;
    if (TankPMTLegendEntries.at(i) == "signal_slot") signal_slot = ivalue;
    if (TankPMTLegendEntries.at(i) == "signal_channel") signal_channel = ivalue;
    if (TankPMTLegendEntries.at(i) == "mt_crate") mt_crate = ivalue;
    if (TankPMTLegendEntries.at(i) == "mt_slot") mt_slot = ivalue;
    if (TankPMTLegendEntries.at(i) == "mt_channel") mt_channel = ivalue;
    if (TankPMTLegendEntries.at(i) == "hv_crate") hv_crate = ivalue;
    if (TankPMTLegendEntries.at(i) == "hv_slot") hv_slot = ivalue;
    if (TankPMTLegendEntries.at(i) == "hv_channel") hv_channel = ivalue;
    if (TankPMTLegendEntries.at(i) == "nominal_HV") nominal_HV = ivalue;
    //Doubles
    if (TankPMTLegendEntries.at(i) == "x_pos") x_pos = dvalue;
    if (TankPMTLegendEntries.at(i) == "y_pos") y_pos = dvalue;
    if (TankPMTLegendEntries.at(i) == "z_pos") z_pos = dvalue;
    if (TankPMTLegendEntries.at(i) == "x_dir") x_dir = dvalue;
    if (TankPMTLegendEntries.at(i) == "y_dir") y_dir = dvalue;
    if (TankPMTLegendEntries.at(i) == "z_dir") z_dir = dvalue;
    //Strings
    if (TankPMTLegendEntries.at(i) == "detector_tank_location") detector_tank_location = svalue;
    if (TankPMTLegendEntries.at(i) == "PMT_type") PMT_type = svalue;
    if (TankPMTLegendEntries.at(i) == "cable_label") cable_label = svalue;
    if (TankPMTLegendEntries.at(i) == "detector_status") detector_status = svalue;
  } 

  //Parse out the Detector Status for filling into Detector class
  detectorstatus detstatus;
  channelstatus chanstatus;
  if(detector_status == "ON"){
    detstatus = detectorstatus::ON;
    chanstatus = channelstatus::ON;
  }
  else if(detector_status == "OFF"){
    detstatus = detectorstatus::OFF;
    chanstatus = channelstatus::OFF;
  }
  else if(detector_status == "UNSTABLE"){
    detstatus = detectorstatus::UNSTABLE;
    chanstatus = channelstatus::UNSTABLE;
  }
  else {
    Log("LoadGeometry Tool: Undefined status of Tank PMT detector",v_error,verbosity);
    if (verbosity > v_error) std::cout << "channel_num is " << channel_num << std::endl;
  } 

  //FIXME: things that are not loaded in with the default det/channel format:
  //      - panel_number
  
  if(verbosity>4) std::cout << "Filling a Tank PMT data line into Detector/Channel classes" << std::endl;
  Detector adet(detector_num,
                "Tank",
                detector_tank_location, 
                Position( x_pos/1000.,
                          y_pos/1000.,
                          z_pos/1000.),
                Direction(x_dir,
                          y_dir,
                          z_dir),
                PMT_type,
                detstatus,
                0.);

  Channel pmtchannel( channel_num,
                      Position(0,0,0.),
                      -1, // stripside
                      -1, // stripnum
                      signal_crate,
                      signal_slot,
                      signal_channel,
                      mt_crate,
                      mt_slot,
                      mt_channel,
                      hv_crate,
                      hv_slot,
                      hv_channel,
                      chanstatus); //channel status same as detector status here
  
  // Add this channel to the geometry
  if(verbosity>4) cout<<"Adding channel "<<channel_num<<" to detector "<<detector_num<<endl;
  adet.AddChannel(pmtchannel);
  if(verbosity>5) cout<<"Adding detector to Geometry"<<endl;
  AnnieGeometry->AddDetector(adet);
  return true;
}

bool LoadGeometry::FileExists(std::string name) {
  ifstream myfile(name.c_str());
  return myfile.good();
}


std::string LoadGeometry::GetLegendLine(std::string name) {
  if(verbosity>4) std::cout << "Getting legend of file: " << name << std::endl;
  std::string line;
  std::string legendline = "null";
  ifstream myfile(name.c_str());
  if (myfile.is_open()){
    while(std::getline(myfile,line)){
      if(verbosity>4) std::cout << line << std::endl;
      if(line.find("#") != std::string::npos) continue;
      if(line.find(LegendLineLabel) != std::string::npos){
        //Next line is the title line
        getline(myfile,line);
        legendline = line;
        if(verbosity>4) std::cout<<"Legend line loaded. Legend is: " << legendline << std::endl;
        break;
      }
    }
  } else {
    Log("LoadGeometry tool: Something went wrong opening a file!!!",v_error,verbosity);
  }
  if(legendline=="null"){
    Log("LoadGeometry tool: Legend line label not found!!!",v_error,verbosity);
  }
  myfile.close();
  return legendline;
}


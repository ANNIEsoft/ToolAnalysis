#include "LoadGeometry.h"

LoadGeometry::LoadGeometry():Tool(){}


bool LoadGeometry::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // Make the RecoDigit Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  m_variables.Get("FACCMRDGeoFile", fFACCMRDGeoFile);
  m_variables.Get("TankPMTGeoFile", fTankPMTGeoFile);
  m_variables.Get("VetoPMTGeoFile", fVetoPMTGeoFile);
  m_variables.Get("LAPPDGeoFile", fLAPPDGeoFile);
  m_variables.Get("DetectorGeoFile", fDetectorGeoFile);
  //Check files exist if(!this->FileExists(fFACCMRDGeoFile)){
		Log("LoadGeometry Tool: File for FACC/MRD Geomtry does not exist!",v_error,verbosity); 
		return false;
  }

  this->ConstructGeometry();
  return true;
}


bool LoadGeometry::Execute(){
  return true;
}


bool LoadGeometry::Finalise(){
  std::cout << "LoadGeometry tool exitting" << std::endl;
  return true;
}

void LoadGeometry::ConstructGeometry(){
  //Initialize the geometry using the geometry CSV file entries 
  this->InitializeGeometry();

  //Load MRD Geometry Detector/Channel Information
  this->LoadFACCMRDDetectors();
}

void LoadGeometry::LoadFACCMRDDetectors(){
  //First, get the MRD file data key
  std::string MRDLegend = this->GetLegendLine(fFACCMRDGeoFile);
  std::vector<std::string> LegendEntries;
  boost::split(LegendEntries,MRDLegend, boost::is_any_of(","), boost::token_compress_on); 
 
  std::string line;
  ifstream myfile(fDetectorGeoFile.c_str());
  if (myfile.is_open()){
    //First, get to where data starts
    while(getline(myfile,line)){
      if(line.find("#") continue;
      if(line.find(DataStartLineLabel){
        break;
      }
    }
    //Loop over lines, collect all detector data
    while(getline(myfile,line)){
      std::cout << line << std::endl; //has our stuff;
      if(line.find("#") continue;
      if(line.find(DataEndLineLabel)) break;
      std::vector<std::string> DataEntries;
      //Parse data line, make corresponding detector/channel
      Detector* FACCMRDDetector = this->ParseMRDDataEntry(DataEntries);
  } else {
    Log("LoadGeometry tool: Something went wrong opening a file!!!",v_error,verbosity);
  }
  Log("LoadGeometry tool: Legend line label not found!!!",v_error,verbosity);
}

Detector* LoadGeometry::ParseMRDDataEntry(std::vector<std::string> DataEntries){
  //Parse the line for information needed to fill the detector & channel classes
		Detector adet(uniquedetectorkey,
					  "MRD",
					  CylLocString,
					  Position( apmt.GetPosition(0)/100.,
					            apmt.GetPosition(1)/100.,
					            apmt.GetPosition(2)/100.),
					  Direction(apmt.GetOrientation(0),
					            apmt.GetOrientation(1),
					            apmt.GetOrientation(2)),
					  apmt.GetName(),
					  detectorstatus::ON,
					  0.);

		Channel pmtchannel( uniquechannelkey,
							Position(0,0,0.),
							0, // stripside
							0, // stripnum
							TDC_Crate_Num,
							TDC_Card_Num,
							TDC_Chan_Num,
							-1,                 // TDC has no level 2 signal handling
							-1,
							-1,
							LeCroy_HV_Crate_Num,
							LeCroy_HV_Card_Num,
							LeCroy_HV_Chan_Num,
							channelstatus::ON);
		
		// Add this channel to the geometry
		if(verbosity>4) cout<<"Adding channel "<<uniquechannelkey<<" to detector "<<uniquedetectorkey<<endl;
		adet.AddChannel(pmtchannel);
}
void LoadGeometry::InitializeGeometry(){
  //Get the Detector file data key
  std::string DetectorLegend = this->GetLegendLine(fDetectorGeoFile);
  std::vector<std::string> LegendEntries;
  boost::split(LegendEntries,DetectorLegend, boost::is_any_of(","), boost::token_compress_on); 
 
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
      if(line.find("#") continue;
      if(line.find(DataStartLineLabel){
        break;
      }
    }
    //Loop over lines, collect all detector data (should only be one line here)
    while(getline(myfile,line)){
      std::cout << line << std::endl; //has our stuff;
      if(line.find("#") continue;
      if(line.find(DataEndLineLabel)) break;
      std::vector<std::string> DataEntries;
      boost::split(DataEntries,line, boost::is_any_of(","), boost::token_compress_on); 
      for (int i=0; i<DataEntries.size(); i++){
        //Check Legend at i, load correct data type
        int ivalue;
        double dvalue;
        if(LegendEntries.at(i) == "geometry_version") ivalue = std:stoi(DataEntries.at(i));
        else dvalue = std::stod(DataEntries.at(i);
        if (LegendEntries.at(i) == "geometry_version") geometry_version = ivalue;
        if (LegendEntries.at(i) == "tank_xcenter") tank_xcenter = dvalue;
        if (LegendEntries.at(i) == "tank_ycenter") tank_ycenter = dvalue;
        if (LegendEntries.at(i) == "tank_zcenter") tank_zcenter = dvalue;
        if (LegendEntries.at(i) == "tank_radius") tank_radius = dvalue;
        if (LegendEntries.at(i) == "tank_halfheight") tank_halfheight = dvalue;
        if (LegendEntries.at(i) == "pmt_enclosed_radius") pmt_enclosed_radius = dvalue;
        if (LegendEntries.at(i) == "pmt_enclosed_halfheight") pmt_enclosed_halfheight = dvalue;
        if (LegendEntries.at(i) == "mrd_width") mrd_width = dvalue;
        if (LegendEntries.at(i) == "mrd_height") mrd_height = dvalue;
        if (LegendEntries.at(i) == "mrd_depth") mrd_depth = dvalue;
        if (LegendEntries.at(i) == "mrd_start") mrd_start = dvalue;
      } 
    }
    Position tank_center(tank_xcenter, tank_ycenter, tank_zcenter);
    // Initialize the Geometry
    AnnieGeometry = new Geometry(GeometryVersion,
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
  Log("LoadGeometry tool: Legend line label not found!!!",v_error,verbosity);
}

bool LoadGeometry::FileExists(const std::string& name) {
  ifstream myfile(name.c_str());
  return myfile.good();
}


std::string DigitBuilder::GetLegendLine(const std::string& name) {
  std::string line;
  ifstream myfile(name.c_str());
  if (myfile.is_open()){
    while(getline(myfile,line)){
      if(line.find("#") continue;
      if(line.find(LegendLineLabel){
        //Next line is the title line
        getline(myfile,line);
        return line;
      }
    }
  } else {
    Log("LoadGeometry tool: Something went wrong opening a file!!!",v_error,verbosity);
  }
  Log("LoadGeometry tool: Legend line label not found!!!",v_error,verbosity);
  return "null";
}


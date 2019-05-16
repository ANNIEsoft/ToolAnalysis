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
  //Check files exist
  if(!this->FileExists(fFACCMRDGeoFile)){
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
  int GeometryVersion = 1;

  //First, get the MRD file data key
  std::string MRDKey = this->GetKeyLine(fFACCMRDGeoFile);
 
  //Initialize at zero; will be set later after channels are loaded 
  int numtankpmts = 0;
  int numlappds = 0;
  int nummrdpmts = 0; 
  int numvetopmts = 0;

  //units in meters
  double tank_xcenter = 0; 
  double tank_ycenter = 0; 
  double tank_zcenter = 0;
  Position tank_center(tank_xcenter, tank_ycenter, tank_zcenter);
  double tank_radius = 0;
  double tank_halfheight = 0;
  //Currently hard-coded; estimated with a tape measure on the ANNIE frame :)
  double pmt_enclosed_radius = 1.0;
  double pmt_enclosed_halfheight = 1.45;
  // geometry variables are in MRDSpecs.hh
  double mrd_width  =  (MRDSpecs::MRD_width)  / 100.;
  double mrd_height =  (MRDSpecs::MRD_height) / 100.;
  double mrd_depth  =  (MRDSpecs::MRD_depth)  / 100.;
  double mrd_start  =  (MRDSpecs::MRD_start)  / 100.;
  if(verbosity>1) cout<<"we have "<<numtankpmts<<" tank pmts, "<<nummrdpmts
  				  <<" mrd pmts and "<<numlappds<<" lappds"<<endl;
  
  // Initialize the Geometry
  Geometry* AnnieGeometry = new Geometry(GeometryVersion,
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

bool LoadGeometry::FileExists(const std::string& name) {
  ifstream myfile(name.c_str());
  return myfile.good();
}


std::string DigitBuilder::GetKeyLine(const std::string& name) {
  std::string KeyLineLabel = "KEY_LINE";
  std::string line;
  ifstream myfile(name.c_str());
  if (myfile.is_open()){
    while(getline(myfile,line)){
      if(line.find("#") continue;
      if(line.find(KeyLineLabel){
        //Next line is the title line
        getline(myfile,line);
        return line;
      }
    }
  } else {
    Log("LoadGeometry tool: Something went wrong opening a file!!!",v_error,verbosity);
  }
  Log("LoadGeometry tool: Key line label not found!!!",v_error,verbosity);
  return "null";

}


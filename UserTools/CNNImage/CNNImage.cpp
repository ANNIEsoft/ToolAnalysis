#include "CNNImage.h"

CNNImage::CNNImage():Tool(){}


bool CNNImage::Initialise(std::string configfile, DataModel &data){

  if (verbosity >=2) std::cout <<"Initialising tool: CNNImage..."<<std::endl;

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();
  m_data= &data; //assigning transient data pointer



  dimensionLAPPD = 1;
  //read in configuration file

  includeTopBottom = false;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("DetectorConf",detector_config);
  m_variables.Get("DataMode",data_mode);
  m_variables.Get("SaveMode",save_mode);
  m_variables.Get("DimensionX",dimensionX);
  m_variables.Get("DimensionY",dimensionY);
  m_variables.Get("IncludeTopBottom",includeTopBottom);
  m_variables.Get("OutputFile",cnn_outpath);
  m_variables.Get("DimensionLAPPD",dimensionLAPPD);


  if (data_mode != "Normal" && data_mode != "Charge-Weighted" && data_mode != "TimeEvolution") data_mode = "Normal";
  if (save_mode != "Geometric" && save_mode != "PMT-wise") save_mode = "Geometric";
  if (verbosity > 0) {
    std::cout <<"data_mode: "<<data_mode<<std::endl;
    std::cout <<"save_mode: "<<save_mode<<std::endl;
  }
  npmtsX = 16;    //in every row, we have 16 PMTs (8 sides, 2 PMTs per side per row)
  nlappdX = dimensionLAPPD;
  nlappdY = dimensionLAPPD;
  if (!includeTopBottom) npmtsY = 6;     //we have 6 rows of PMTs (excluding top and bottom PMTs)
  else npmtsY = 10;		//for top & bottom PMTs, include 2 extra rows of PMTs at top and at the bottom


  //get geometry

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  tank_radius = geom->GetTankRadius();
  tank_height = geom->GetTankHalfheight();
  double barrel_compression = 0.82;
  if (detector_config == "ANNIEp2v6") tank_height*=barrel_compression;
  if (tank_radius < 1.) tank_radius = 1.37504;
  Position detector_center = geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);
  std::map<std::string,std::map<unsigned long,Detector*> >* Detectors = geom->GetDetectors();

  //read in PMT positions

  max_y = -100.;
  min_y = 100.;

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin();
                                                    it != Detectors->at("Tank").end();
                                                  ++it){
    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    pmt_detkeys.push_back(detkey);
    unsigned long chankey = apmt->GetChannels()->begin()->first;
    pmt_chankeys.push_back(chankey);
    Position position_PMT = apmt->GetDetectorPosition();
    if (verbosity > 2) std::cout <<"detkey: "<<detkey<<std::endl;
    if (verbosity > 2) std::cout <<"chankey: "<<chankey<<std::endl;
    x_pmt.insert(std::pair<int,double>(detkey,position_PMT.X()-tank_center_x));
    y_pmt.insert(std::pair<int,double>(detkey,position_PMT.Y()-tank_center_y));
    z_pmt.insert(std::pair<int,double>(detkey,position_PMT.Z()-tank_center_z));
    if (verbosity > 2) std::cout <<"Detector ID: "<<detkey<<", position: ("<<position_PMT.X()<<","<<position_PMT.Y()<<","<<position_PMT.Z()<<")"<<std::endl;
    if (verbosity > 2) std::cout <<"Rho PMT "<<detkey<<": "<<sqrt(x_pmt.at(detkey)*x_pmt.at(detkey)+z_pmt.at(detkey)*z_pmt.at(detkey))<<std::endl;
    if (verbosity > 2) std::cout <<"Y PMT: "<<y_pmt.at(detkey)<<std::endl;
    //std::cout <<"tank location: "<<apmt->GetTankLocation()<<", y: "<<y_pmt.at(detkey)<<std::endl;
    if (y_pmt[detkey]>max_y && apmt->GetTankLocation()!="OD") max_y = y_pmt.at(detkey);
    if (y_pmt[detkey]<min_y && apmt->GetTankLocation()!="OD") min_y = y_pmt.at(detkey);

  }

  std::cout <<"max_y = "<<max_y<<std::endl;
  std::cout <<"min_y = "<<min_y<<std::endl;

  //order the PMT 2D positions 
  for (unsigned int i_pmt = 0; i_pmt < y_pmt.size(); i_pmt++){

    double x,y;
    unsigned long detkey = pmt_detkeys[i_pmt];
    Position pmt_pos(x_pmt[detkey],y_pmt[detkey],z_pmt[detkey]);
    unsigned long chankey = pmt_chankeys[i_pmt];
    Detector *apmt = geom->ChannelToDetector(chankey);
    if (apmt->GetTankLocation()=="OD") continue;  //don't include OD PMTs
    if ((y_pmt[detkey] >= max_y-0.001 || y_pmt[detkey] <= min_y+0.001) && !includeTopBottom) continue;     //don't include top/bottom PMTs if specified
 
    if (y_pmt[detkey] >= max_y-0.001) {
      ConvertPositionTo2D_Top(pmt_pos, x, y);
    }
    else if (y_pmt[detkey] <= min_y+0.001) {
	ConvertPositionTo2D_Bottom(pmt_pos, x, y);
    }
    else {
       ConvertPositionTo2D(pmt_pos, x, y);
    }
    x = (round(100*x)/100.);
    y = (round(100*y)/100.);
    if (fabs(x-0.52)<0.001) x = 0.53;
    if (fabs(x-0.47)<0.001) x = 0.48;
    if (y_pmt[detkey] <= min_y + 0.001){
      if (fabs(x-0.23)<0.001) x = 0.22;
      if (fabs(x-0.77)<0.001) x = 0.78;
    }
    if (y_pmt[detkey] >= max_y - 0.001){
      if (fabs(x-0.19)<0.001) x = 0.21;
      if (fabs(x-0.34)<0.001) x = 0.32;
      if (fabs(x-0.48)<0.001) x = 0.50;
      if (fabs(x-0.66)<0.001) x = 0.68;
    }
    if (y_pmt[detkey] >= max_y-0.001) vec_pmt2D_x_Top.push_back(x);
    else if (y_pmt[detkey] <= min_y+0.001) vec_pmt2D_x_Bottom.push_back(x);
    else vec_pmt2D_x.push_back(x);
    vec_pmt2D_y.push_back(y);
    if (verbosity > 2) std::cout <<detkey<<"  "<<x<<"  "<<y<<std::endl;
  }

  if (verbosity > 2) std::cout <<"vec_pmt2D_* size: "<<vec_pmt2D_x.size()<<std::endl;
  std::sort(vec_pmt2D_x.begin(),vec_pmt2D_x.end());
  std::sort(vec_pmt2D_y.begin(),vec_pmt2D_y.end());
  std::sort(vec_pmt2D_x_Top.begin(),vec_pmt2D_x_Top.end());
  std::sort(vec_pmt2D_x_Bottom.begin(),vec_pmt2D_x_Bottom.end());
  vec_pmt2D_x.erase(std::unique(vec_pmt2D_x.begin(),vec_pmt2D_x.end()),vec_pmt2D_x.end());
  vec_pmt2D_y.erase(std::unique(vec_pmt2D_y.begin(),vec_pmt2D_y.end()),vec_pmt2D_y.end());
  vec_pmt2D_x_Top.erase(std::unique(vec_pmt2D_x_Top.begin(),vec_pmt2D_x_Top.end()),vec_pmt2D_x_Top.end());
  vec_pmt2D_x_Bottom.erase(std::unique(vec_pmt2D_x_Bottom.begin(),vec_pmt2D_x_Bottom.end()),vec_pmt2D_x_Bottom.end());
  if (verbosity > 1) std::cout <<"Sorted 2D position vectors: "<<std::endl;
  for (unsigned int i_x=0;i_x<vec_pmt2D_x.size();i_x++){
    if (verbosity > 1) std::cout <<"x vector "<<i_x<<": "<<vec_pmt2D_x.at(i_x)<<std::endl;
  }
  for (unsigned int i_y=0;i_y<vec_pmt2D_y.size();i_y++){
    if (verbosity > 1) std::cout <<"y vector "<<i_y<<": "<<vec_pmt2D_y.at(i_y)<<std::endl;
  }
  for (unsigned int i_x=0;i_x<vec_pmt2D_x_Top.size();i_x++){
    if (verbosity > 1) std::cout <<"x top vector "<<i_x<<": "<<vec_pmt2D_x_Top.at(i_x)<<std::endl;
  }
  for (unsigned int i_x=0;i_x<vec_pmt2D_x_Bottom.size();i_x++){
    if (verbosity > 1) std::cout <<"x bottom vector "<<i_x<<": "<<vec_pmt2D_x_Bottom.at(i_x)<<std::endl;
  }
  

  //read in lappd positions

  max_y_lappd = -100.;
  min_y_lappd = 100.;

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("LAPPD").begin();
                                                    it != Detectors->at("LAPPD").end();
                                                  ++it){
    Detector* anlappd = it->second;
    unsigned long detkey = it->first;
    lappd_detkeys.push_back(detkey);
    unsigned long chankey = anlappd->GetChannels()->begin()->first;
    Position position_LAPPD = anlappd->GetDetectorPosition();
    if (verbosity > 2) std::cout <<"detkey: "<<detkey<<std::endl;
    if (verbosity > 2) std::cout <<"chankey: "<<chankey<<std::endl;
    x_lappd.insert(std::pair<int,double>(detkey,position_LAPPD.X()-tank_center_x));
    y_lappd.insert(std::pair<int,double>(detkey,position_LAPPD.Y()-tank_center_y));
    z_lappd.insert(std::pair<int,double>(detkey,position_LAPPD.Z()-tank_center_z));
    if (verbosity > 2) std::cout <<"Detector ID: "<<detkey<<", position: ("<<position_LAPPD.X()<<","<<position_LAPPD.Y()<<","<<position_LAPPD.Z()<<")"<<std::endl;
    if (verbosity > 2) std::cout <<"Rho LAPPD "<<detkey<<": "<<sqrt(x_lappd.at(detkey)*x_lappd.at(detkey)+z_lappd.at(detkey)*z_lappd.at(detkey))<<std::endl;
    if (verbosity > 2) std::cout <<"Y LAPPD: "<<y_lappd.at(detkey)<<std::endl;
    if (y_lappd[detkey]>max_y_lappd) max_y_lappd = y_lappd.at(detkey);
    if (y_lappd[detkey]<min_y_lappd) min_y_lappd = y_lappd.at(detkey);

  }

  //order the lappd 2D positions 
  for (unsigned int i_lappd = 0; i_lappd < y_lappd.size(); i_lappd++){

    double x,y;
    unsigned long detkey = lappd_detkeys[i_lappd];
    Position lappd_pos(x_lappd[detkey],y_lappd[detkey],z_lappd[detkey]);
    if (y_lappd[detkey] >= max_y || y_lappd[detkey] <= min_y) continue;     //don't include top/bottom/OD PMTs for now
    ConvertPositionTo2D(lappd_pos, x, y);
    //std::cout << "CNNImage: Converting Position ("<<x_lappd[detkey]<<", "<<y_lappd[detkey]<<", "<<z_lappd[detkey]<<" for detkey "<<detkey<<" to 2D yields "<<x<<", "<<y<<std::endl;
    vec_lappd2D_x.push_back(x);
    vec_lappd2D_y.push_back(y);
    if (verbosity > 2) std::cout << detkey <<"  "<<x<<"  "<<y<<std::endl;
  }

  if (verbosity > 2) std::cout <<"vec_lappd2D_* size: "<<vec_lappd2D_x.size()<<std::endl;
  std::sort(vec_lappd2D_x.begin(),vec_lappd2D_x.end());
  std::sort(vec_lappd2D_y.begin(),vec_lappd2D_y.end());
  vec_lappd2D_x.erase(std::unique(vec_lappd2D_x.begin(),vec_lappd2D_x.end()),vec_lappd2D_x.end());
  vec_lappd2D_y.erase(std::unique(vec_lappd2D_y.begin(),vec_lappd2D_y.end()),vec_lappd2D_y.end());
  std::cout <<"Sorted 2D position vectors: "<<std::endl;
  for (unsigned int i_x=0;i_x<vec_lappd2D_x.size();i_x++){
    if (verbosity > 2) std::cout <<"x vector "<<i_x<<": "<<vec_lappd2D_x.at(i_x)<<std::endl;
  }
  for (unsigned int i_y=0;i_y<vec_lappd2D_y.size();i_y++){
    if (verbosity > 2) std::cout <<"y vector "<<i_y<<": "<<vec_lappd2D_y.at(i_y)<<std::endl;
  }


  //define root and csv files to save histograms (root-files temporarily, for cross-checks)

  std::string str_root = ".root";
  std::string str_csv = ".csv";
  std::string str_time = "_time";
  std::string str_charge = "_charge";
  std::string str_lappd = "_lappd";
  std::string str_Rings = "_Rings";
  std::string str_MRD = "_MRD";
  std::string rootfile_name = cnn_outpath + str_root;
  std::string csvfile_name = cnn_outpath + str_charge + str_csv;
  std::string csvfile_time_name = cnn_outpath + str_time + str_csv;
  std::string csvfile_lappd_name = cnn_outpath + str_lappd + str_charge + str_csv;
  std::string csvfile_lappd_time_name = cnn_outpath + str_lappd + str_time + str_csv;
  std::string csvfile_MRD = cnn_outpath + str_MRD + str_csv;
  std::string csvfile_Rings = cnn_outpath + str_Rings + str_csv;



  file = new TFile(rootfile_name.c_str(),"RECREATE");
  outfile.open(csvfile_name.c_str());
  outfile_time.open(csvfile_time_name.c_str());
  outfile_lappd.open(csvfile_lappd_name.c_str());
  outfile_lappd_time.open(csvfile_lappd_time_name.c_str());
  outfile_Rings.open(csvfile_Rings.c_str());
  outfile_MRD.open(csvfile_MRD.c_str());


  return true;
}


bool CNNImage::Execute(){

  if (verbosity >=2) std::cout <<"Executing tool: CNNImage..."<<std::endl;

  //get ANNIEEvent store information
  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl;}/*return false;*/

  m_data->Stores["ANNIEEvent"]->Get("MCParticles",mcparticles); //needed to retrieve true vertex and direction
  m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits", MCLAPPDHits);
  m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",runnumber);
  m_data->Stores["ANNIEEvent"]->Get("SubRunNumber",subrunnumber);
  m_data->Stores["ANNIEEvent"]->Get("EventTime",EventTime);
  m_data->Stores.at("ANNIEEvent")->Get("NumMrdTimeClusters",mrdeventcounter);
  m_data->Stores.at("RecoEvent")->Get("NRings",nrings);
  
  std::cout << "Rings: " << nrings<< endl ;
  std::cout << "TDCdata" << mrdeventcounter<< endl;

  //clear variables & containers
  charge.clear();
  time.clear();
  hitpmt_detkeys.clear();
  charge_lappd.clear();
  time_lappd.clear();
  hits_lappd.clear();
  total_charge_lappd.clear();

  for (unsigned int i_pmt=0; i_pmt<pmt_detkeys.size();i_pmt++){

    unsigned long detkey = pmt_detkeys[i_pmt];
    charge.emplace(detkey,0.);
    time.emplace(detkey,0.);
  }

  std::cout <<"lappd_detkeys.size() = "<<lappd_detkeys.size()<<std::endl;
  for (unsigned int i_lappd=0; i_lappd<lappd_detkeys.size(); i_lappd++){
    unsigned long detkey = lappd_detkeys[i_lappd];
    //std::cout <<"Initializing lappd detkey "<<detkey<<std::endl;
    total_charge_lappd.emplace(detkey,0);
    std::vector<std::vector<double>> temp_lappdXY;
    std::vector<std::vector<int>> temp_int_lappdXY;;
    for (int lappdX=0; lappdX<dimensionLAPPD; lappdX++){
      std::vector<double> temp_lappdY;
      std::vector<int> temp_int_lappdY;
      temp_lappdY.assign(dimensionLAPPD,0.);
      temp_int_lappdY.assign(dimensionLAPPD,0);
      temp_lappdXY.push_back(temp_lappdY);
      temp_int_lappdXY.push_back(temp_int_lappdY);
    }
    charge_lappd.emplace(detkey,temp_lappdXY);
    time_lappd.emplace(detkey,temp_lappdXY);
    hits_lappd.emplace(detkey,temp_int_lappdXY);
  }

  //make basic selection cuts to only look at clear event signatures

  bool bool_primary=false;
  bool bool_geometry=false;
  bool bool_nhits=false;

  if (verbosity > 1) std::cout <<"Loop through MCParticles..."<<std::endl;
  for(unsigned int particlei=0; particlei<mcparticles->size(); particlei++){
    MCParticle aparticle = mcparticles->at(particlei);
    if (verbosity > 2) std::cout <<"particle "<<particlei<<std::endl;
    if (verbosity > 2) std::cout <<"Parent ID: "<<aparticle.GetParentPdg()<<std::endl;
    if (verbosity > 2) std::cout <<"PDG code: "<<aparticle.GetPdgCode()<<std::endl;
    if (verbosity > 2) std::cout <<"Flag: "<<aparticle.GetFlag()<<std::endl;
    if (aparticle.GetParentPdg() !=0 ) continue;
    if (aparticle.GetFlag() !=0 ) continue;
//    if (!(aparticle.GetPdgCode() == 11 || aparticle.GetPdgCode() == 13)) continue;    //primary particle for Cherenkov tracking should be muon or electron
//    else {
      truevtx = aparticle.GetStartVertex();
      truevtx_x = truevtx.X()-tank_center_x;
      truevtx_y = truevtx.Y()-tank_center_y;
      truevtx_z = truevtx.Z()-tank_center_z;
      double distInnerStr_Hor = tank_radius - sqrt(pow(truevtx_x,2)+pow(truevtx_z,2));
      double distInnerStr_Vert1 = max_y - truevtx_y;
      double distInnerStr_Vert2 = truevtx_y - min_y;
      double distInnerStr_Vert;
      if (distInnerStr_Vert1 > 0 && distInnerStr_Vert2 > 0) {
        if (distInnerStr_Vert1>distInnerStr_Vert2) distInnerStr_Vert=distInnerStr_Vert2;
        else distInnerStr_Vert=distInnerStr_Vert1;
      } else if (distInnerStr_Vert1 <=0) distInnerStr_Vert=distInnerStr_Vert2;
      else distInnerStr_Vert=distInnerStr_Vert1;
      bool_geometry = (distInnerStr_Vert>0.2 && distInnerStr_Hor>0.2);
      bool_primary = true;
//    }
  }

  //---------------------------------------------------------------
  //-------------------Iterate over MCHits ------------------------
  //---------------------------------------------------------------

  int vectsize = MCHits->size();
  if (verbosity > 1) std::cout <<"Tool CNNImage: MCHits size: "<<vectsize<<std::endl;
  total_hits_pmts=0;
  for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
    unsigned long chankey = apair.first;
    if (verbosity > 3) std::cout <<"chankey: "<<chankey<<std::endl;
    Detector* thistube = geom->ChannelToDetector(chankey);
    unsigned long detkey = thistube->GetDetectorID();
    if (verbosity > 3) std::cout <<"detkey: "<<detkey<<std::endl;
    if (thistube->GetDetectorElement()=="Tank"){
      if (thistube->GetTankLocation()=="OD") continue;
      hitpmt_detkeys.push_back(detkey);
      std::vector<MCHit>& Hits = apair.second;
      int hits_pmt = 0;
      for (MCHit &ahit : Hits){
        if (ahit.GetTime()>-10. && ahit.GetTime()<40.){
          charge[detkey] += ahit.GetCharge();
          if (data_mode == "Normal") time[detkey] += ahit.GetTime();
          else if (data_mode == "Charge-Weighted") time[detkey] += (ahit.GetTime()*ahit.GetCharge());
          hits_pmt++;
        }
      }
      if (data_mode == "Normal" && hits_pmt>0) time[detkey]/=hits_pmt;         //use mean time of all hits on one PMT
      else if (data_mode == "Charge-Weighted" && charge[detkey]>0.) time[detkey] /= charge[detkey];
      total_hits_pmts++;
    }
  }

  std::cout <<"done with MCHits loop"<<std::endl;
  if (total_hits_pmts>=10) bool_nhits=true;

  //---------------------------------------------------------------
  //-------------------Iterate over MCLAPPDHits ------------------------
  //---------------------------------------------------------------

  total_hits_lappds=0;
  total_charge_lappds=0;
  min_time_lappds=9999;
  max_time_lappds=-9999;
  vectsize = MCLAPPDHits->size();
  if (verbosity > 1) std::cout <<"Tool CNNImage: MCLAPPDHits size: "<<vectsize<<std::endl;
  for(std::pair<unsigned long, std::vector<MCLAPPDHit>>&& apair : *MCLAPPDHits){
    unsigned long chankey = apair.first;
    if (verbosity > 3) std::cout <<"chankey: "<<chankey<<std::endl;
    Detector* det = geom->ChannelToDetector(chankey);
    unsigned long detkey = det->GetDetectorID();
    std::vector<MCLAPPDHit>& hits = apair.second;
    if (verbosity > 2) std::cout <<"detector key: "<<detkey<<std::endl;
    for (MCLAPPDHit& ahit : hits){
        std::vector<double> temp_pos = ahit.GetPosition();
        double x_lappd = temp_pos.at(0)-tank_center_x;    //global x-position (in detector coordinates)
        double y_lappd = temp_pos.at(1)-tank_center_y;    //global y-position (in detector coordinates)
        double z_lappd = temp_pos.at(2)-tank_center_z;    //global z-position (in detector coordinates)
        std::vector<double> local_pos = ahit.GetLocalPosition();

        //VERIFY: x_local_lappd is local_pos.at(1) and y_local_lappd is local_pos.at(0)?
        //Or the other way around?
        //-----------------------------------------------------------------------------------------
        double x_local_lappd = local_pos.at(1);           //local parallel position within LAPPD
        double y_local_lappd = local_pos.at(0);           //local transverse position within LAPPD
        //-----------------------------------------------------------------------------------------
        //std::cout <<"LAPPDHit, local hit : "<<x_local_lappd<<", "<<y_local_lappd<<", global hit: "<<x_lappd<<", "<<y_lappd<<", "<<z_lappd<<std::endl;
        double lappd_charge = 1.0;
        double t_lappd = ahit.GetTime();
        int binx_lappd = round((x_local_lappd+0.1)/0.2*dimensionLAPPD);       //local positions can be positive and negative, 10cm > x_local_lappd > -10cm 
        int biny_lappd = round((y_local_lappd+0.1)/0.2*dimensionLAPPD);
        if (binx_lappd < 0) binx_lappd=0;         //FIXME: hits outside of -10cm...10cm should probably be just discarded
        if (biny_lappd < 0) biny_lappd=0;         //FIXME: hits outside of -10cm...10cm should probably be just discarded
        if (binx_lappd > dimensionLAPPD-1) binx_lappd = dimensionLAPPD-1;     //FIXME: hits outside of -10cm...10cm should probably be just discarded
        if (biny_lappd > dimensionLAPPD-1) biny_lappd = dimensionLAPPD-1;     //FIXME: hits outside of -10cm...10cm should probably be just discarded
        //std::cout <<"binx_lappd: "<<binx_lappd<<", biny_lappd: "<<biny_lappd<<std::endl;
        //std::cout <<"charge: "<<std::endl;
        //std::cout <<charge_lappd[detkey].at(binx_lappd).at(biny_lappd)<<std::endl;
        if (t_lappd>-10. && t_lappd<40){
          charge_lappd[detkey].at(binx_lappd).at(biny_lappd) += lappd_charge;
          time_lappd[detkey].at(binx_lappd).at(biny_lappd) += t_lappd;
          hits_lappd[detkey].at(binx_lappd).at(biny_lappd)++;
          total_charge_lappd[detkey] += lappd_charge;
          total_charge_lappds += lappd_charge;
          if (max_time_lappds < t_lappd) max_time_lappds = t_lappd;
          if (min_time_lappds > t_lappd) min_time_lappds = t_lappd;
        }
        for (int i_lappdX=0; i_lappdX<dimensionLAPPD; i_lappdX++){
          for (int i_lappdY=0; i_lappdY<dimensionLAPPD; i_lappdY++){
            if (hits_lappd[detkey].at(i_lappdX).at(i_lappdY)>0) time_lappd[detkey].at(i_lappdX).at(i_lappdY)/=hits_lappd[detkey].at(i_lappdX).at(i_lappdY); 
          }
        }
      //time[detkey]/=hits_pmt;         //use mean time of all hits on one PMT
      total_hits_lappds++;
    }
  }

  if (min_time_lappds>0.) min_time_lappds=0.;

  //---------------------------------------------------------------
  //------------- Determine max+min values ------------------------
  //---------------------------------------------------------------

  maximum_pmts = 0;
  max_time_pmts = 0;
  min_time_pmts = 999.;
  total_charge_pmts = 0;
  for (unsigned int i_pmt=0;i_pmt<hitpmt_detkeys.size();i_pmt++){
    unsigned long detkey = hitpmt_detkeys[i_pmt];
    if (charge[detkey]>maximum_pmts) maximum_pmts = charge[detkey];
    total_charge_pmts+=charge[detkey];
    if (time[detkey]>max_time_pmts) max_time_pmts = time[detkey];
    if (time[detkey]<min_time_pmts) min_time_pmts = time[detkey];
  }

  maximum_lappds = 0;
  for (unsigned int i_lappd = 0; i_lappd < lappd_detkeys.size();i_lappd++){
    unsigned long detkey = lappd_detkeys[i_lappd];
    for (int iX=0; iX<dimensionLAPPD; iX++){
      for (int iY=0; iY<dimensionLAPPD; iY++){
        if (charge_lappd[detkey].at(iX).at(iY) > maximum_lappds) maximum_lappds = charge_lappd[detkey].at(iX).at(iY);
      }
    }
  }


  //---------------------------------------------------------------
  //-------------- Readout MRD ------------------------------------
  //---------------------------------------------------------------
  int num_mrd_paddles=0;
  int num_mrd_layers=0;
  int num_mrd_conslayers=0;
  int num_mrd_adjacent=0;
  double mrd_padperlayer = 0.;
  bool layer_occupied[11] = {0};
  double mrd_paddlesize[11];
  std::vector<std::vector<double>> mrd_hits;
  for (int i_layer=0; i_layer<11; i_layer++){
    std::vector<double> empty_hits;
    mrd_hits.push_back(empty_hits);
  }
  std::vector<int> temp_cons_layers;
  if(TDCData->size()==0){
      std::cout <<"No TDC hits"<<std::endl;
  } else {
      std::vector<int> temp_cons_layers;
      for(auto&& anmrdpmt : (*TDCData)){
          unsigned long chankey = anmrdpmt.first;
          Detector *thedetector = geom->ChannelToDetector(chankey);
          if(thedetector->GetDetectorElement()!="MRD") {
              continue;                 // this is a veto hit, not an MRD hit.
          }
          num_mrd_paddles++;
          int detkey = thedetector->GetDetectorID();
          Paddle *apaddle = geom->GetDetectorPaddle(detkey);
          int layer = apaddle->GetLayer();
          layer_occupied[layer-1]=true;
          if (apaddle->GetOrientation()==1) {
              mrd_hits.at(layer-2).push_back(0.5*(apaddle->GetXmin()+apaddle->GetXmax()));
              mrd_paddlesize[layer-2]=apaddle->GetPaddleWidth();
          }
          else if (apaddle->GetOrientation()==0) {
              mrd_hits.at(layer-2).push_back(0.5*(apaddle->GetYmin()+apaddle->GetYmax()));
              mrd_paddlesize[layer-2]=apaddle->GetPaddleWidth();
          }
      }
      if (num_mrd_paddles > 0) {
          for (int i_layer=0;i_layer<11;i_layer++){
              if (layer_occupied[i_layer]==true) {
                  num_mrd_layers++;
                  if (num_mrd_conslayers==0) num_mrd_conslayers++;
                  else {
                      if (layer_occupied[i_layer-1]==true) num_mrd_conslayers++;
                      else {
                          temp_cons_layers.push_back(num_mrd_conslayers);
                          num_mrd_conslayers=0;
                      }
                  }
              }
              for (unsigned int i_hitpaddle=0; i_hitpaddle<mrd_hits.at(i_layer).size(); i_hitpaddle++){
                  for (unsigned int j_hitpaddle= i_hitpaddle+1; j_hitpaddle < mrd_hits.at(i_layer).size(); j_hitpaddle++){
                      if (fabs(mrd_hits.at(i_layer).at(i_hitpaddle)-mrd_hits.at(i_layer).at(j_hitpaddle))-mrd_paddlesize[i_layer] < 0.001) num_mrd_adjacent++;
                  }
              }       
          }
      } 
      std::vector<int>::iterator it = std::max_element(temp_cons_layers.begin(),temp_cons_layers.end());
      if (it != temp_cons_layers.end()) num_mrd_conslayers = *it;
      else num_mrd_conslayers=0;
      mrd_padperlayer = double(num_mrd_paddles)/num_mrd_layers;
  }
  //---------------------------------------------------------------
  //-------------- Create CNN images ------------------------------
  //---------------------------------------------------------------

  //define histogram as an intermediate step to the CNN
  std::stringstream ss_cnn, ss_title_cnn, ss_cnn_time, ss_title_cnn_time, ss_cnn_pmtwise, ss_title_cnn_pmtwise, ss_cnn_time_pmtwise, ss_title_cnn_time_pmtwise;
  ss_cnn<<"hist_cnn"<<evnum;
  ss_title_cnn<<"EventDisplay (CNN), Event "<<evnum;
  ss_cnn_time<<"hist_cnn_time"<<evnum;
  ss_title_cnn_time<<"EventDisplay Time (CNN), Event "<<evnum;
  ss_cnn_pmtwise<<"hist_cnn_pmtwise"<<evnum;
  ss_title_cnn_pmtwise<<"EventDisplay (CNN, pmt wise), Event "<<evnum;
  ss_cnn_time_pmtwise << "hist_cnn_time_pmtwise"<<evnum;
  ss_title_cnn_time_pmtwise <<"EventDisplay Time (CNN, pmt wise), Event "<<evnum;
  TH2F *hist_cnn = new TH2F(ss_cnn.str().c_str(),ss_title_cnn.str().c_str(),dimensionX,0.5-TMath::Pi()*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,dimensionY,0.5-(0.45*tank_height/tank_radius+2)*size_top_drawing, 0.5+(0.45*tank_height/tank_radius+2)*size_top_drawing);
  TH2F *hist_cnn_time = new TH2F(ss_cnn_time.str().c_str(),ss_title_cnn_time.str().c_str(),dimensionX,0.5-TMath::Pi()*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,dimensionY,0.5-(0.45*tank_height/tank_radius+2)*size_top_drawing, 0.5+(0.45*tank_height/tank_radius+2)*size_top_drawing);
  TH2F *hist_cnn_pmtwise = new TH2F(ss_cnn_pmtwise.str().c_str(),ss_title_cnn_pmtwise.str().c_str(),npmtsX,0,npmtsX,npmtsY,0,npmtsY);
  TH2F *hist_cnn_time_pmtwise = new TH2F(ss_cnn_time_pmtwise.str().c_str(),ss_title_cnn_time_pmtwise.str().c_str(),npmtsX,0,npmtsX,npmtsY,0,npmtsY);
  //define LAPPD histograms (1 per LAPPD)
  std::vector<TH2F*> hists_lappd, hists_time_lappd;
  for (unsigned int i_lappd = 0; i_lappd < lappd_detkeys.size(); i_lappd++){
    std::stringstream ss_hist_lappd, ss_title_lappd, ss_hist_time_lappd, ss_title_time_lappd;
    ss_hist_lappd<<"hist_lappd"<<i_lappd<<"_ev"<<evnum;
    ss_title_lappd<<"EventDisplay (CNN), LAPPD "<<i_lappd<<", Event "<<evnum;
    ss_hist_time_lappd<<"hist_time_lappd"<<i_lappd<<"_ev"<<evnum;
    ss_title_time_lappd<<"EventDisplay Time (CNN), LAPPD "<<i_lappd<<", Event "<<evnum;
    TH2F *hist_lappd = new TH2F(ss_hist_lappd.str().c_str(),ss_title_lappd.str().c_str(),dimensionLAPPD,0,dimensionLAPPD,dimensionLAPPD,0,dimensionLAPPD);    //resolution in x/y direction should be 1cm, over a total length of 20cm each
    hists_lappd.push_back(hist_lappd);
    TH2F *hist_time_lappd = new TH2F(ss_hist_time_lappd.str().c_str(),ss_title_time_lappd.str().c_str(),dimensionLAPPD,0,dimensionLAPPD,dimensionLAPPD,0,dimensionLAPPD);    //resolution in x/y direction should be 1cm, over a total length of 20cm each
    hists_time_lappd.push_back(hist_time_lappd);
  }

  //fill the events into the histogram

  for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];

    double x,y;
    Position pmt_pos(x_pmt[detkey],y_pmt[detkey],z_pmt[detkey]);
    ConvertPositionTo2D(pmt_pos, x, y);
    
    int binx = hist_cnn->GetXaxis()->FindBin(x);
    int biny = hist_cnn->GetYaxis()->FindBin(y);
    if (verbosity > 2) std::cout <<"CNNImage tool: binx: "<<binx<<", biny: "<<biny<<", charge fill: "<<charge[detkey]<<", time fill: "<<time[detkey]<<std::endl;

    if (maximum_pmts < 0.001) maximum_pmts = 1.;
    if (fabs(max_time_pmts) < 0.001) max_time_pmts = 1.;
   
    double charge_fill = charge[detkey]/maximum_pmts;
    hist_cnn->SetBinContent(binx,biny,hist_cnn->GetBinContent(binx,biny)+charge_fill);
    double time_fill = (time[detkey]-min_time_pmts)/(max_time_pmts-min_time_pmts);
    hist_cnn_time->SetBinContent(binx,biny,hist_cnn_time->GetBinContent(binx,biny)+time_fill);

    //fill the pmt-wise histogram
    if ((y_pmt[detkey]>=max_y || y_pmt[detkey]<=min_y) && !includeTopBottom) continue;       //don't include endcaps in the pmt-wise histogram for now
    if (y_pmt[detkey]>=max_y) ConvertPositionTo2D_Top(pmt_pos,x,y);
    if (y_pmt[detkey]<=min_y) ConvertPositionTo2D_Bottom(pmt_pos,x,y);
    double xCorr, yCorr;
    xCorr = (round(100*x)/100.);
    yCorr = (round(100*y)/100.);
    if (fabs(xCorr-0.52)<0.001) xCorr = 0.53;
    if (fabs(xCorr-0.47)<0.001) xCorr = 0.48;
    std::vector<double>::iterator it_x, it_y;
    if (y_pmt[detkey]>=max_y){
      if (fabs(xCorr-0.19)<0.001) xCorr = 0.21;
      if (fabs(xCorr-0.34)<0.001) xCorr = 0.32;
      if (fabs(xCorr-0.48)<0.001) xCorr = 0.50;
      if (fabs(xCorr-0.66)<0.001) xCorr = 0.68;
      it_x = std::find(vec_pmt2D_x_Top.begin(),vec_pmt2D_x_Top.end(),xCorr);
    }
    else if (y_pmt[detkey]<=min_y){
      if (fabs(xCorr-0.23)<0.001) xCorr = 0.22;
      if (fabs(xCorr-0.77)<0.001) xCorr = 0.78;
      it_x = std::find(vec_pmt2D_x_Bottom.begin(),vec_pmt2D_x_Bottom.end(),xCorr);
    }
    else {
      it_x = std::find(vec_pmt2D_x.begin(),vec_pmt2D_x.end(),xCorr);
    }
    it_y = std::find(vec_pmt2D_y.begin(),vec_pmt2D_y.end(),yCorr);
    int index_x, index_y;
    if (y_pmt[detkey]>=max_y) index_x = std::distance(vec_pmt2D_x_Top.begin(),it_x);
    else if (y_pmt[detkey]<=min_y) index_x = std::distance(vec_pmt2D_x_Bottom.begin(),it_x);
    else index_x = std::distance(vec_pmt2D_x.begin(),it_x);
    index_y = std::distance(vec_pmt2D_y.begin(),it_y);
    //std::cout <<"index_x: "<<index_x<<", index_y: "<<index_y<<std::endl;
    hist_cnn_pmtwise->SetBinContent(index_x+1,index_y+1,charge_fill);
    hist_cnn_time_pmtwise->SetBinContent(index_x+1,index_y+1,time_fill);

    //fill the lappd-wise histograms
    for (unsigned int i_lappd=0; i_lappd<lappd_detkeys.size(); i_lappd++){
      unsigned long detkey = lappd_detkeys.at(i_lappd);
      for (int iX=0; iX < dimensionLAPPD; iX++){
        for (int iY=0; iY < dimensionLAPPD; iY++){
          double lappd_charge_fill = charge_lappd[detkey].at(iX).at(iY)/maximum_lappds;
          double lappd_time_fill = (time_lappd[detkey].at(iX).at(iY)-min_time_lappds)/(max_time_lappds-min_time_lappds);
          if (lappd_time_fill < 0)  std::cout <<"Min LAPPD time: "<<min_time_lappds<<", Max LAPPD time: "<<max_time_lappds<<", time_lappd: "<<time_lappd[detkey].at(iX).at(iY)<<", fill time: "<<lappd_time_fill<<std::endl;
          hists_lappd.at(i_lappd)->SetBinContent(iX+1,iY+1,lappd_charge_fill);
          hists_time_lappd.at(i_lappd)->SetBinContent(iX+1,iY+1,lappd_time_fill);
        }
      }
    }

  }

  //save information from histogram to csv file
  //(1 line corresponds to 1 event, histogram entries flattened out to a 1D array)
 
  bool passed_eventselection;
  m_data->Stores["RecoEvent"]->Get("EventCutStatus",passed_eventselection);
  std::cout <<"passed_eventselection: "<<passed_eventselection<<std::endl;



  //if (bool_primary && bool_geometry && bool_nhits) {
  if (passed_eventselection) {
    // safe Rings and MRD information
    outfile_Rings << nrings << endl;
    outfile_MRD << mrdeventcounter<< ","<< num_mrd_paddles <<","<< num_mrd_layers<<","<<num_mrd_conslayers<<","<<num_mrd_adjacent<<","<<mrd_padperlayer<< endl;

    //save root histograms
    hist_cnn->Write();
    hist_cnn_time->Write();
    hist_cnn_pmtwise->Write();
    hist_cnn_time_pmtwise->Write();
    for (unsigned int i_lappd=0; i_lappd<lappd_detkeys.size();i_lappd++){
      hists_lappd.at(i_lappd)->Write();
      hists_time_lappd.at(i_lappd)->Write();
    }
    //std::cout <<"save_mode: "<<save_mode<<std::endl;
    if (save_mode == "Geometric"){
      for (int i_binY=0; i_binY < hist_cnn->GetNbinsY();i_binY++){
        for (int i_binX=0; i_binX < hist_cnn->GetNbinsX();i_binX++){
          outfile << hist_cnn->GetBinContent(i_binX+1,i_binY+1);
          if (i_binX != hist_cnn->GetNbinsX()-1 || i_binY!=hist_cnn->GetNbinsY()-1) outfile<<",";
          outfile_time << hist_cnn_time->GetBinContent(i_binX+1,i_binY+1);
          if (i_binX != hist_cnn_time->GetNbinsX()-1 || i_binY!=hist_cnn_time->GetNbinsY()-1) outfile_time<<",";    
        }
      }
    } else if (save_mode == "PMT-wise"){
      //std::cout <<"Entering PMT-wise save mode"<<std::endl;
      for (int i_binY=0; i_binY < hist_cnn_pmtwise->GetNbinsY();i_binY++){
        for (int i_binX=0; i_binX < hist_cnn_pmtwise->GetNbinsX();i_binX++){
          outfile << hist_cnn_pmtwise->GetBinContent(i_binX+1,i_binY+1);
          if (i_binX != hist_cnn_pmtwise->GetNbinsX()-1 || i_binY!=hist_cnn_pmtwise->GetNbinsY()-1) outfile<<",";
          outfile_time << hist_cnn_time_pmtwise->GetBinContent(i_binX+1,i_binY+1);
          if (i_binX != hist_cnn_time_pmtwise->GetNbinsX()-1 || i_binY!=hist_cnn_time_pmtwise->GetNbinsY()-1) outfile_time<<",";    
        }
      }
      //std::cout <<"lappd_detkeys.size(): "<<lappd_detkeys.size()<<std::endl;
      for (unsigned int i_lappd=0;i_lappd<lappd_detkeys.size();i_lappd++){
        //std::cout <<"Filling i_lappd "<<i_lappd<<std::endl;
        for (int i_binY=0; i_binY < hists_lappd.at(i_lappd)->GetNbinsY();i_binY++){
          for (int i_binX=0; i_binX < hists_lappd.at(i_lappd)->GetNbinsX();i_binX++){
            //std::cout <<"i_binY "<<i_binY<<", i_binX "<<i_binX<<", Filled "<<hists_lappd.at(i_lappd)->GetBinContent(i_binX+1,i_binY+1)<<std::endl;
            outfile_lappd << hists_lappd.at(i_lappd)->GetBinContent(i_binX+1,i_binY+1);
            if (i_binX != hists_lappd.at(i_lappd)->GetNbinsX()-1 || i_binY!=hists_lappd.at(i_lappd)->GetNbinsY()-1 || i_lappd != lappd_detkeys.size()-1) {
              //std::cout <<"i_binY "<<i_binY<<", i_binX "<<i_binX<<", comma will be set!"<<std::endl;
              outfile_lappd<<",";
            }
            outfile_lappd_time << hists_time_lappd.at(i_lappd)->GetBinContent(i_binX+1,i_binY+1);
            if (i_binX != hists_time_lappd.at(i_lappd)->GetNbinsX()-1 || i_binY!=hists_time_lappd.at(i_lappd)->GetNbinsY()-1 || i_lappd != lappd_detkeys.size()-1) outfile_lappd_time<<","; 
          }
        }
      }
      outfile_lappd << std::endl;
      outfile_lappd_time << std::endl;
    }
    outfile << std::endl;
    outfile_time << std::endl;
  }

  return true;
}


bool CNNImage::Finalise(){

  if (verbosity >=2 ) std::cout <<"Finalising tool: CNNImage..."<<std::endl;
  file->Close();
  outfile.close();
  outfile_time.close();
  outfile_lappd.close();
  outfile_lappd_time.close();
  outfile_Rings.close();
  outfile_MRD.close();
  return true;
}

void CNNImage::ConvertPositionTo2D(Position xyz_pos, double &x, double &y){

    if (fabs(xyz_pos.Y()-max_y)<0.01){
      //top PMTs
      x=0.5-size_top_drawing*xyz_pos.X()/tank_radius;
      y=0.5+((0.45*tank_height)/tank_radius+1)*size_top_drawing-size_top_drawing*xyz_pos.Z()/tank_radius;
    } else if (fabs(xyz_pos.Y()-min_y)<0.01 || fabs(xyz_pos.Y()+1.30912)<0.01){
      //bottom PMTs
      x=0.5-size_top_drawing*xyz_pos.X()/tank_radius;
      y=0.5-(0.45*tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*xyz_pos.Z()/tank_radius;
    } else {
      //barrel PMTs
      double phi;
      if (xyz_pos.X()>0 && xyz_pos.Z()>0) phi = atan(xyz_pos.Z()/xyz_pos.X())+TMath::Pi()/2;
      else if (xyz_pos.X()>0 && xyz_pos.Z()<0) phi = atan(xyz_pos.X()/-xyz_pos.Z());
      else if (xyz_pos.X()<0 && xyz_pos.Z()<0) phi = 3*TMath::Pi()/2+atan(xyz_pos.Z()/xyz_pos.X());
      else if (xyz_pos.X()<0 && xyz_pos.Z()>0) phi = TMath::Pi()+atan(-xyz_pos.X()/xyz_pos.Z());
      else if (fabs(xyz_pos.X())<0.0001){
        if (xyz_pos.Z()>0) phi = TMath::Pi();
        else if (xyz_pos.Z()<0) phi = 2*TMath::Pi();
      }
      else if (fabs(xyz_pos.Z())<0.0001){
        if (xyz_pos.X()>0) phi = 0.5*TMath::Pi();
        else if (xyz_pos.X()<0) phi = 3*TMath::Pi()/2;
      }
      else phi = 0.;
      if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
      phi-=TMath::Pi();
      if (phi < - TMath::Pi()) phi = -TMath::Pi();
      if (phi<-TMath::Pi() || phi>TMath::Pi())  std::cout <<"Drawing Event: Phi out of bounds! "<<", x= "<<xyz_pos.X()<<", y="<<xyz_pos.Y()<<", z="<<xyz_pos.Z()<<std::endl;
      x=0.5+phi*size_top_drawing;
      y=0.5+xyz_pos.Y()/tank_height*tank_height/tank_radius*size_top_drawing;
      }
}

void CNNImage::ConvertPositionTo2D_Top(Position xyz_pos, double &x, double &y){

	double rho = sqrt(xyz_pos.X()*xyz_pos.X()+xyz_pos.Z()*xyz_pos.Z());
	if (rho < 0.8) y = 0.9;
	else y = 0.8;

	double phi;
	if (xyz_pos.X()>0 && xyz_pos.Z()>0) phi = atan(xyz_pos.Z()/xyz_pos.X())+TMath::Pi()/2;
        else if (xyz_pos.X()>0 && xyz_pos.Z()<0) phi = atan(xyz_pos.X()/-xyz_pos.Z());
        else if (xyz_pos.X()<0 && xyz_pos.Z()<0) phi = 3*TMath::Pi()/2+atan(xyz_pos.Z()/xyz_pos.X());
        else if (xyz_pos.X()<0 && xyz_pos.Z()>0) phi = TMath::Pi()+atan(-xyz_pos.X()/xyz_pos.Z());
        else if (fabs(xyz_pos.X())<0.0001){
          if (xyz_pos.Z()>0) phi = TMath::Pi();
          else if (xyz_pos.Z()<0) phi = 2*TMath::Pi();
        }
        else if (fabs(xyz_pos.Z())<0.0001){
          if (xyz_pos.X()>0) phi = 0.5*TMath::Pi();
          else if (xyz_pos.X()<0) phi = 3*TMath::Pi()/2;
        }
        else phi = 0.;
        if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
        phi-=TMath::Pi();
        if (phi < - TMath::Pi()) phi = -TMath::Pi();
        if (phi<-TMath::Pi() || phi>TMath::Pi())  std::cout <<"Drawing Event: Phi out of bounds! "<<", x= "<<xyz_pos.X()<<", y="<<xyz_pos.Y()<<", z="<<xyz_pos.Z()<<std::endl;
        x=0.5+phi*size_top_drawing; 

}

void CNNImage::ConvertPositionTo2D_Bottom(Position xyz_pos, double &x, double &y){

        double rho = sqrt(xyz_pos.X()*xyz_pos.X()+xyz_pos.Z()*xyz_pos.Z());
        if (rho < 0.6) y = -0.9;
        else y = -0.8;
 
	double phi;  
        if (xyz_pos.X()>0 && xyz_pos.Z()>0) phi = atan(xyz_pos.Z()/xyz_pos.X())+TMath::Pi()/2;
        else if (xyz_pos.X()>0 && xyz_pos.Z()<0) phi = atan(xyz_pos.X()/-xyz_pos.Z());
        else if (xyz_pos.X()<0 && xyz_pos.Z()<0) phi = 3*TMath::Pi()/2+atan(xyz_pos.Z()/xyz_pos.X());
        else if (xyz_pos.X()<0 && xyz_pos.Z()>0) phi = TMath::Pi()+atan(-xyz_pos.X()/xyz_pos.Z());
        else if (fabs(xyz_pos.X())<0.0001){
          if (xyz_pos.Z()>0) phi = TMath::Pi();
          else if (xyz_pos.Z()<0) phi = 2*TMath::Pi();
        }
        else if (fabs(xyz_pos.Z())<0.0001){
          if (xyz_pos.X()>0) phi = 0.5*TMath::Pi();
          else if (xyz_pos.X()<0) phi = 3*TMath::Pi()/2;
        }
        else phi = 0.;
        if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
        phi-=TMath::Pi();
        if (phi < - TMath::Pi()) phi = -TMath::Pi();
        if (phi<-TMath::Pi() || phi>TMath::Pi())  std::cout <<"Drawing Event: Phi out of bounds! "<<", x= "<<xyz_pos.X()<<", y="<<xyz_pos.Y()<<", z="<<xyz_pos.Z()<<std::endl;
        x=0.5+phi*size_top_drawing;

}

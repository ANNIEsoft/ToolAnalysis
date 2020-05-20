#include "EventDisplay.h"

EventDisplay::EventDisplay():Tool(){}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-------------------------   INIT/ EXEC / FINAL ------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------


bool EventDisplay::Initialise(std::string configfile, DataModel &data){

  //only for debugging memory leaks, otherwise comment out
  /*std::cout <<"List of Objects (beginning of initialise): "<<std::endl;
  gObjectTable->Print();*/

  //--------------------------------------------
  //---------Loading config file----------------
  //--------------------------------------------

  if(configfile!="")  m_variables.Initialise(configfile); 
  m_data= &data;       //assigning transient data pointer

  marker_size = 2.;
  output_format = "root";
  isData = false;
  pmt_Qmax = 100;
  pmt_Qmin = 0;
  pmt_Qbins = 100;
  pmt_Tmin = -10;
  pmt_Tmax = 40;
  pmt_Tbins = 50;
  lappd_Qmax = 60;
  lappd_Qmin = 0;
  lappd_Qbins = 60;
  lappd_Tmin = -10.;
  lappd_Tmax = 40.;
  lappd_Tbins = 50;
  npmtcut = 10;
  histogram_config = "None";
  user_run_number = -1;
  user_run_type = 3;

  //--------------------------------------------
  //-------Get configuration variables----------
  //--------------------------------------------

  m_variables.Get("verbose",verbose);
  m_variables.Get("Event",single_event);
  m_variables.Get("EventList",str_event_list);
  m_variables.Get("SelectedEvents",selected_event);
  m_variables.Get("Mode",mode);
  m_variables.Get("EventType",EventType);
  m_variables.Get("TextBox",text_box);
  m_variables.Get("Threshold_Charge",threshold);
  m_variables.Get("Threshold_ChargeLAPPD",threshold_lappd);
  m_variables.Get("Threshold_TimeLow",threshold_time_low);
  m_variables.Get("Threshold_TimeHigh",threshold_time_high);
  m_variables.Get("Threshold_TimeLowMRD",threshold_time_low_mrd);
  m_variables.Get("Threshold_TimeHighMRD",threshold_time_high_mrd);
  m_variables.Get("LAPPDsSelected",lappds_selected);
  m_variables.Get("LAPPDsFile",lappds_file);
  m_variables.Get("DrawRing",draw_ring);
  m_variables.Get("DrawVertex",draw_vertex);
  m_variables.Get("SavePlots",save_plots);
  m_variables.Get("OutputFormat",output_format);
  m_variables.Get("HistogramPlots",draw_histograms);
  m_variables.Get("UserInput",user_input);    //manually decide when to go to next event
  m_variables.Get("Graphics",use_tapplication);
  m_variables.Get("MarkerSize",marker_size);
  m_variables.Get("OutputFile",out_file);
  m_variables.Get("DetectorConfiguration",detector_config);
  m_variables.Get("IsData",isData);
  m_variables.Get("RunNumber",user_run_number); 
  m_variables.Get("RunType",user_run_type); 
  m_variables.Get("HistogramConfig",histogram_config);
  m_variables.Get("NPMTCut",npmtcut);
  m_variables.Get("DrawClusterPMT",draw_cluster);
  m_variables.Get("DrawClusterMRD",draw_cluster_mrd);
  m_variables.Get("ChargeFormat",charge_format);
  m_variables.Get("SinglePEGains",singlePEgains);
  m_variables.Get("UseFilteredDigits",use_filtered_digits);

  Log("EventDisplay tool: Initialising",v_message,verbose);

  //---------------------------------------------------
  //Check for non-sense input  / set default behavior--
  //---------------------------------------------------

  if (mode !="Charge" && mode!="Time") mode = "Charge";
  if (EventType!="ANNIEEvent" && EventType!="RecoEvent") EventType = "ANNIEEvent";
  if (output_format != "root" && output_format != "image") output_format = "root";
  if (text_box!=true && text_box!=false) text_box=true;
  if (lappds_selected!=0 && lappds_selected!=1) lappds_selected=0;
  if (draw_vertex!=0 && draw_vertex!=1) draw_vertex = 0;
  if (draw_ring!=0 && draw_ring!=1) draw_ring = 0;
  if (save_plots!=0 && save_plots!=1) save_plots=1;
  if (draw_histograms!=0 && draw_histograms !=1) draw_histograms=1;
  if (user_input!=0 && user_input!=1) user_input = 0;
  if (use_tapplication!= 0 && use_tapplication!= 1) use_tapplication = 0;
  if (draw_cluster!=0 && draw_cluster!=1) draw_cluster=0;
  if (draw_cluster_mrd!=0 && draw_cluster_mrd!=1) draw_cluster_mrd=0;
  if (charge_format != "nC" && charge_format != "pe") charge_format = "nC";
  if (str_event_list!="None") single_event = -999;
  if (threshold_time_low == -999) min_cluster_time = 0.;
  else min_cluster_time = threshold_time_low;
  if (threshold_time_high == -999) max_cluster_time = 2000.;
  else max_cluster_time = threshold_time_high;
  if (use_filtered_digits!=0 && use_filtered_digits!=1) use_filtered_digits = 0;
  if (user_input) {
    single_event = -999;
    str_event_list = "None";
  }
  if (isData) {
    //Currently only the true vertex and ring can be drawn, so disable this feature for data files
    draw_ring = false;
    draw_vertex = false;
  }
  if (histogram_config!="None"){
    ifstream file_hist(histogram_config.c_str());
    std::string temp_string;
    double temp_value;
    while (!file_hist.eof()){
      file_hist>>temp_string>>temp_value;
      if(file_hist.eof()) break;
      if (temp_string=="pmt_Qmax") pmt_Qmax = temp_value;
      else if (temp_string=="pmt_Qmin") pmt_Qmin = temp_value;
      else if (temp_string=="pmt_Qbins") pmt_Qbins = temp_value;
      else if (temp_string=="pmt_Tmax") pmt_Tmax = temp_value;
      else if (temp_string=="pmt_Tmin") pmt_Tmin = temp_value;      
      else if (temp_string=="pmt_Tbins") pmt_Tbins = temp_value;      
      else if (temp_string=="lappd_Qmax") lappd_Qmax = temp_value;
      else if (temp_string=="lappd_Qmin") lappd_Qmin = temp_value;
      else if (temp_string=="lappd_Qbins") lappd_Qbins = temp_value;
      else if (temp_string=="lappd_Tmax") lappd_Tmax = temp_value;
      else if (temp_string=="lappd_Tmin") lappd_Tmin = temp_value;
      else if (temp_string=="lappd_Tbins") lappd_Tbins = temp_value;
      else {
        Log("EventDisplay tool: Error while reading in histogram configuration file "+histogram_config+": Cannot parse argument "+temp_string+", please look up the available options!",v_error,verbose);
      }
    }
    file_hist.close();
  }

  //---------------------------------------------------
  //-------Get ev list in case it's specified----------
  //---------------------------------------------------

  if (str_event_list!="None"){
    ifstream infile(str_event_list.c_str());
    int current_ev;
    while (!infile.eof()){
      infile>>current_ev;
      if (!infile.eof()) ev_list.push_back(current_ev);
    }
    infile.close();
  }

  std::string logmessage = "EventDisplay tool: EventList size: "+std::to_string(ev_list.size());
  Log(logmessage,v_message,verbose);

  //---------------------------------------------------
  // ------Print chosen settings for EventDisplay------
  //---------------------------------------------------

  if (verbose > 0){
    std::cout <<"//////////////////////////////////////////////////////////////////////"<<std::endl;
    std::cout <<"----------------------------EVENT DISPLAY ---------------------------"<<std::endl;
    std::cout <<"---------------------- CONFIGURATION PARAMETERS ---------------------"<<std::endl;
    std::cout <<"---------------------------------------------------------------------"<<std::endl;
    std::cout <<"Event Number:"<< single_event<<std::endl;
    std::cout <<"Event List: "<< str_event_list<<std::endl;
    std::cout <<"Display Mode: "<< mode<<std::endl;
    std::cout <<"EventType: "<<EventType<<std::endl;
    std::cout <<"Draw TextBox: "<<text_box<<std::endl;
    std::cout <<"Threshold Charge: "<<threshold<<std::endl;
    std::cout <<"Threshold Charge LAPPDs: "<<threshold_lappd<<std::endl;
    std::cout <<"Threshold Time Low: "<<threshold_time_low<<std::endl;
    std::cout <<"Threshold Time High: "<<threshold_time_high<<std::endl;
    std::cout <<"LAPPDs selected: "<<lappds_selected<<std::endl; 
    std::cout <<"LAPPDs File: "<<lappds_file<<std::endl; 
    std::cout <<"Output File name: "<<out_file<<std::endl;
    std::cout <<"Drawing Interaction Vertex: "<<draw_vertex<<std::endl;
    std::cout <<"Drawing Expected Ring: "<<draw_ring<<std::endl;
    std::cout <<"Launching TApplication: "<<use_tapplication<<std::endl;
    std::cout <<"User Input: "<<user_input<<std::endl;
    std::cout <<"Detector configuration: "<<detector_config<<std::endl;
    std::cout <<"---------------------------------------------------------------------"<<std::endl;
    std::cout <<"/////////////////////////////////////////////////////////////////////"<<std::endl;
  }

  //----------------------------------------------------
  //--Only evaluate active LAPPD file if told to do so--
  //----------------------------------------------------

  if (lappds_selected){ 

    std::string prefix = "./configfiles/EventDisplay/";
    std::string filename_lappd = prefix+lappds_file;
    double temp_lappd;
    ifstream file_lappd(filename_lappd);
    while (!file_lappd.eof()){
      file_lappd>>temp_lappd;
      if (!file_lappd.eof()) {
        active_lappds_user.emplace(temp_lappd,1);
        Log("EventDisplay tool: Emplacing user-specified lappd id "+std::to_string(temp_lappd)+" to active lappd settings.",v_message,verbose);
      }
    }
    file_lappd.close();
  }

  //----------------------------------------------------
  //---------Get geometry of detector-------------------
  //----------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

  tank_radius = geom->GetTankRadius();
  tank_height = geom->GetTankHalfheight();
  tank_height/=2;
  detector_version = geom->GetVersion();
  logmessage = "EventDisplay tool: Using detector version "+std::to_string(detector_version);
  Log(logmessage,v_message,verbose);
  double barrel_compression=0.82;
  if (detector_config == "ANNIEp2v6" && !isData) tank_height*=barrel_compression;            //use compressed barrel radius for ANNIEp2v6 detector configuration (only MC)
  else if (isData) tank_height = 1.2833; 			    //streamline with MC value
  if (tank_radius<1.0 || isData)  tank_radius=1.37504;              //set tank radius to the standard value of old anniev2 configuration(v4/v6 seems to have a very different radius?)

  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  n_lappds = 0;

  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  n_lappds = geom->GetNumDetectorsInSet("LAPPD");
  n_mrd_pmts = geom->GetNumDetectorsInSet("MRD");
  n_veto_pmts = geom->GetNumDetectorsInSet("Veto");

  //Need the following conversion from WCSim IDs because Digit Objects in the RecoEvent store currently still depend on them
  if (!isData){
    m_data->CStore.Get("lappd_tubeid_to_detectorkey",lappd_tubeid_to_detectorkey);
  }
  if (EventType == "RecoEvent"){
    m_data->CStore.Get("pmt_tubeid_to_channelkey",pmt_tubeid_to_channelkey);
  }
  if (verbose > 0) std::cout <<"EventDisplay: Num Tank PMTs: "<<n_tank_pmts<<", num MRD PMTs: "<<n_mrd_pmts<<", num Veto PMTs: "<<n_veto_pmts<<", num LAPPDs: "<<n_lappds<<std::endl;
  std::map<std::string,std::map<unsigned long,Detector*> >* Detectors = geom->GetDetectors();
  if (verbose > 0) std::cout <<"Detectors size: "<<Detectors->size()<<std::endl;
  
  //----------------------------------------------------------------------
  //-----------Read in the LAPPD detkeys into a vector--------------------
  //----------------------------------------------------------------------

  for (std::map<unsigned long,Detector*>::iterator it = Detectors->at("LAPPD").begin();
							it != Detectors->at("LAPPD").end();
							++it){
  	Detector *alappd = it->second;
    logmessage = "EventDisplay tool: Looping over LAPPD detector key "+std::to_string(alappd->GetDetectorID());
  	Log(logmessage,v_debug,verbose);
  	lappd_detkeys.push_back(alappd->GetDetectorID());
   }
   max_num_lappds = lappd_detkeys.size();
   logmessage = "EventDisplay tool: Number of LAPPDs: "+std::to_string(max_num_lappds);
   Log(logmessage,v_message,verbose);

  //----------------------------------------------------------------------
  //-----------Convert user's provided LAPPD indexes to detectorkey-------
  //----------------------------------------------------------------------

  if(lappds_selected){
    
    for(auto&& auserid : active_lappds_user){
      if (!isData){
        //specify wcsim ids in configuration file for use with MC
        if(lappd_tubeid_to_detectorkey.count(auserid.first)==0){
          logmessage = "EventDisplay tool: No detectorkey found for user-provided LAPPDID "+std::to_string(auserid.first)+"!";
          Log(logmessage,v_warning,verbose);
        } else {
          active_lappds.emplace(lappd_tubeid_to_detectorkey.at(auserid.first),1);
        }
      } else {
        //specify detectorkeys in configuration file for use with data
        active_lappds.emplace(auserid.first,1);
      }
    }
    for (int i_lappd = 0; i_lappd < n_lappds; i_lappd++){
      unsigned long detkey = lappd_detkeys.at(i_lappd);

    }
  }

  //----------------------------------------------------------------------
  //-----------Read in PMT x/y/z positions into vectors-------------------
  //----------------------------------------------------------------------

  max_y=-100.;
  min_y=100.;

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin();
                                                    it != Detectors->at("Tank").end();
                                                  ++it){
    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    pmt_detkeys.push_back(detkey);
    unsigned long chankey = apmt->GetChannels()->begin()->first;
    Position position_PMT = apmt->GetDetectorPosition();
    x_pmt.insert(std::pair<int,double>(detkey,position_PMT.X()-tank_center_x));
    y_pmt.insert(std::pair<int,double>(detkey,position_PMT.Y()-tank_center_y));
    z_pmt.insert(std::pair<int,double>(detkey,position_PMT.Z()-tank_center_z));
    logmessage = "EventDisplay tool: DetectorID = "+std::to_string(detkey)+", position = ("+std::to_string(position_PMT.X())+","+std::to_string(position_PMT.Y())+","+std::to_string(position_PMT.Z())+"), rho= "+std::to_string(sqrt(x_pmt.at(detkey)*x_pmt.at(detkey)+z_pmt.at(detkey)*z_pmt.at(detkey)))+", y = "+std::to_string(y_pmt.at(detkey));
    Log(logmessage,v_debug,verbose);

    if (y_pmt[detkey]>max_y && apmt->GetTankLocation()!="OD") max_y = y_pmt.at(detkey);
    if (y_pmt[detkey]<min_y && apmt->GetTankLocation()!="OD") min_y = y_pmt.at(detkey);
  }

  logmessage = "EventDisplay tool: Properties of the detector configuration: "+std::to_string(n_veto_pmts)+" veto PMTs, "+std::to_string(n_mrd_pmts)+" MRD PMTs, "+std::to_string(n_tank_pmts)+" Tank PMTs, and "+std::to_string(n_lappds)+" LAPPDs.";
  Log(logmessage,v_message,verbose);
  logmessage = "EventDisplay tool: Max y (Tank PMTs): "+std::to_string(max_y)+", min y (Tank PMTs): "+std::to_string(min_y);
  Log(logmessage,v_message,verbose);


  //---------------------------------------------------------------
  //----------------Read in MRD x/y/z positions--------------------
  //---------------------------------------------------------------
  
  min_mrd_y = 99999.;
  max_mrd_y = -999999.;
  min_mrd_x = 999999.;
  max_mrd_x = -999999.;
  min_mrd_z = 9999999.;
  max_mrd_z = -999999.;

  Log("EventDisplay tool: Reading in MRD information...",v_message,verbose);

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("MRD").begin();
                                                    it != Detectors->at("MRD").end();
                                                  ++it){
    Detector* amrdpmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = amrdpmt->GetChannels()->begin()->first;
    Paddle *mrdpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);

    double xmin = mrdpaddle->GetXmin();
    double xmax = mrdpaddle->GetXmax();
    double ymin = mrdpaddle->GetYmin();
    double ymax = mrdpaddle->GetYmax();
    double zmin = mrdpaddle->GetZmin();
    double zmax = mrdpaddle->GetZmax();
    int orientation = mrdpaddle->GetOrientation();    //0 is horizontal, 1 is vertical
    int half = mrdpaddle->GetHalf();                  //0 or 1
    int side = mrdpaddle->GetSide();

    Log("EventDisplay tool: Reading in MRD channel, detkey "+std::to_string(detkey)+", x = "+std::to_string(xmin)+"-"+std::to_string(xmax)+", y = "+std::to_string(ymin)+"-"+std::to_string(ymax)+", z = "+std::to_string(zmin)+"-"+std::to_string(zmax)+")",v_debug,verbose);

    std::vector<double> xdim{xmin,xmax};
    std::vector<double> ydim{ymin,ymax};
    std::vector<double> zdim{zmin,zmax};

    mrd_x.emplace(detkey,xdim);
    mrd_y.emplace(detkey,ydim);
    mrd_z.emplace(detkey,zdim);
    mrd_orientation.emplace(detkey,orientation);
    mrd_half.emplace(detkey,half);

    if (xmin < min_mrd_x) min_mrd_x = xmin;
    if (ymin < min_mrd_y) min_mrd_y = ymin;
    if (zmin < min_mrd_z) min_mrd_z = zmin;
    if (xmax > max_mrd_x) max_mrd_x = xmax;
    if (ymax > max_mrd_y) max_mrd_y = ymax;
    if (zmax > max_mrd_z) max_mrd_z = zmax;

    mrd_detkeys.push_back(detkey);

  }

  mrd_diffx = (max_mrd_x - min_mrd_x);
  mrd_diffy = (max_mrd_y - min_mrd_y);
  mrd_diffz = (max_mrd_z - min_mrd_z);

  Log("EventDisplay tool: Read in MRD PMT information: xmin = "+std::to_string(min_mrd_x)+", ymin = "+std::to_string(min_mrd_y)+", zmin = "+std::to_string(min_mrd_z)+", xmax = "+std::to_string(max_mrd_x)+", ymax = "+std::to_string(max_mrd_y)+", zmax = "+std::to_string(max_mrd_z),v_message,verbose);


  //---------------------------------------------------------------
  //---------------Read in single p.e. gains-----------------------
  //---------------------------------------------------------------

  ifstream file_singlepe(singlePEgains.c_str());
  unsigned long temp_chankey;
  double temp_gain;
  while (!file_singlepe.eof()){
    file_singlepe >> temp_chankey >> temp_gain;
    if (file_singlepe.eof()) break;
    pmt_gains.emplace(temp_chankey,temp_gain);
  }
  file_singlepe.close();

  //---------------------------------------------------------------
  //----Initialize TApplication in case of displayed graphics------
  //---------------------------------------------------------------

  if (use_tapplication){
    Log("EventDisplay tool: Initializing TApplication",v_message,verbose);
    int myargc = 0;
    char *myargv[] {(char*) "options"};
    app_event_display = new TApplication("app_event_display",&myargc,myargv);
    Log("EventDisplay tool: TApplication running.",v_message,verbose);
  }

  //---------------------------------------------------------------
  //---------------Set the color palette once----------------------
  //---------------------------------------------------------------
  set_color_palette();

  //---------------------------------------------------------------
  //---------------Initialize canvases ----------------------------
  //---------------------------------------------------------------

  if (output_format == "root") {
    std::stringstream ss_rootfile_name;
    ss_rootfile_name << out_file << ".root";
    root_file = new TFile(ss_rootfile_name.str().c_str(),"RECREATE");
  }

  if (draw_histograms){
    canvas_pmt = new TCanvas("canvas_pmt","Tank PMT histograms",900,600);
    canvas_pmt_supplementary = new TCanvas("canvas_pmt_combined","PMT combined",900,600);
    canvas_lappd = new TCanvas("canvas_lappd","LAPPD histograms",900,600);
  }
  canvas_ev_display=new TCanvas("canvas_ev_display","Event Display",900,900);

  for (int i_lappd=0; i_lappd < max_num_lappds; i_lappd++){
    time_LAPPDs[i_lappd] = nullptr;
    charge_LAPPDs[i_lappd] = nullptr;
  }

  //--------------------------------------------------------------
  //------------ Initialise map of runtypes-----------------------
  //--------------------------------------------------------------
  
  map_runtypes.emplace(0,"Test");
  map_runtypes.emplace(1,"LED");
  map_runtypes.emplace(2,"Ped");
  map_runtypes.emplace(3,"Beam");
  map_runtypes.emplace(4,"AmBe");

  std::string StartTime = "1970/1/1";
  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

  //--------------------------------------------------------------
  //------------Get first of selected events ---------------------
  //--------------------------------------------------------------
  
  if (ev_list.size()>0){
    m_data->CStore.Set("UserEvent",true);
    m_data->CStore.Set("LoadEvNr",ev_list.at(0));
    m_data->CStore.Set("CheckFurtherTriggers",false);
    i_loop=1;
  }

  if (single_event >= 0){
    m_data->CStore.Set("UserEvent",true);
    m_data->CStore.Set("LoadEvNr",single_event);
    m_data->CStore.Set("CheckFurtherTriggers",false);
    i_loop=1;
  }

  return true;
}


bool EventDisplay::Execute(){

  Log("EventDisplay tool: Executing.",v_message,verbose);

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ 
    Log("EventDisplay tool: No ANNIEEvent store!",v_error,verbose); 
    return false;
  }

  //---------------------------------------------------------------
  //---------------Get User EventNumber ---------------------------
  //---------------------------------------------------------------

  if (str_event_list!="None"){
    if (int(ev_list.size()) > i_loop){
      m_data->CStore.Set("UserEvent",true);
      m_data->CStore.Set("LoadEvNr",ev_list.at(i_loop));
      m_data->CStore.Set("CheckFurtherTriggers",false);
      i_loop++;
    } else {
      m_data->CStore.Set("UserEvent",false);
      m_data->vars.Set("StopLoop",1);
    }
  }

  int get_ok;

  //---------------------------------------------------------------
  //---------------Get general objects ----------------------------
  //---------------------------------------------------------------

  get_ok = m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  if(not get_ok) { Log("EventDisplay tool: Error retrieving EventNumber, true from ANNIEEvent!",v_error,verbose); return false;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("RunNumber",runnumber);
  if(not get_ok){ Log("EventDisplay tool: Error retrieving RunNumber, true from ANNIEEvent!",v_error,verbose); return false;}
  //get_ok = m_data->Stores["ANNIEEvent"]->Get("SubRunNumber",subrunnumber);
  //if(not get_ok){ Log("EventDisplay tool: Error retrieving SubRunNumber, true from ANNIEEvent!",v_error,verbose); return false;}
 
  //---------------------------------------------------------------
  //------------------ Get Data related objects -------------------
  //---------------------------------------------------------------

  if (isData) {
    get_ok = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving Hits, true from ANNIEEvent!",v_error,verbose);}
    //For data, it is currently still possible that there does not exist the TDCData or LAPPDHits object. Don't return false in these cases
    get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData_Data);  // a std::map<ChannelKey,vector<TDCHit>>
    if(not get_ok){ Log("EventDisplay tool: Error retrieving TDCData (Data), true from ANNIEEvent!",v_error,verbose);}
    //TODO: Uncomment following two lines once LAPPDs are available in data
    //get_ok = m_data->Stores["ANNIEEvent"]->Get("LAPPDHits",LAPPDHits);
    //if(not get_ok){ Log("EventDisplay tool: Error retrieving LAPPDHits, true from ANNIEEvent!",v_error,verbose);}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("RunType",RunType);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving RunType, true from ANNIEEvent! ",v_error,verbose); return false;}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("EventTimeTank",EventTankTime);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving EventTimeTank, true from ANNIEEvent! ",v_error,verbose); return false;}
  }


  //---------------------------------------------------------------
  //------------------ Get MC related objects ---------------------
  //---------------------------------------------------------------

  if (!isData){
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCParticles",mcparticles); //needed to retrieve true vertex and direction
    if(not get_ok){ Log("EventDisplay tool: Error retrieving MCParticles, true from ANNIEEvent!",v_error,verbose); return false;}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving MCHits, true from ANNIEEvent!",v_error,verbose); return false;}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<ChannelKey,vector<TDCHit>>
    if(not get_ok){ Log("EventDisplay tool: Error retrieving TDCData, true from ANNIEEvent!",v_error,verbose); return false;}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",MCLAPPDHits);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving MCLAPPDHits, true from ANNIEEvent!",v_error,verbose); return false;} 
    
    //EventTime currently not implemented in data
    get_ok = m_data->Stores["ANNIEEvent"]->Get("EventTime",EventTime);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving EventTime, true from ANNIEEvent!",v_error,verbose); return false;}
    
    //Currently TriggerData is not available for data, will be inserted in the future
    //TODO: Load the same information for data once the EventBuilder is fully operational
    get_ok = m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving BeamStatus, true from ANNIEEvent!",v_error,verbose); return false;}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving TriggerData, true from ANNIEEvent!",v_error,verbose); return false;}
  
    //MCEventNum and MCTriggernum only available for MC (for now)
    //TODO: Get the information also for data as soon as the EventBuilder is configured to save the information
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCEventNum",mcevnum);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving MCEventNum, true from ANNIEEvent!",v_error,verbose); return false;}
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCTriggernum",MCTriggernum);
    if(not get_ok){ Log("EventDisplay tool: Error retrieving MCTriggernum, true from ANNIEEvent!",v_error,verbose); return false;}
  
    //Get RecoEvent related variables, related to MC
    //TODO: Add reconstructed versions of vertices and rings 
    get_ok = m_data->Stores.at("RecoEvent")->Get("TrueVertex",TrueVertex);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving TrueVertex,true from RecoEvent!",v_error,verbose); return false; }
    get_ok = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex",TrueStopVertex);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving TrueStopVertex,true from RecoEvent!",v_error,verbose); return false; }
    get_ok = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",TrueMuonEnergy);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving TrueMuonEnergy,true from RecoEvent!",v_error,verbose); return false; }
    get_ok = m_data->Stores.at("RecoEvent")->Get("NRings",nrings);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving NRings, true from RecoEvent!",v_error,verbose); return false; }
    get_ok = m_data->Stores.at("RecoEvent")->Get("IndexParticlesRing",particles_ring);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving IndexParticlesRing, true from RecoEvent!",v_error,verbose); return false; }
  }


  //---------------------------------------------------------------
  //------------- Get RecoEvent related variables - ---------------
  //---------------------------------------------------------------
  
  if (EventType == "RecoEvent"){
    get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit",RecoDigits);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving RecoDigit,true from RecoEvent!",v_error,verbose); return false; }
    get_ok = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",EventCutStatus);
    if(not get_ok){ Log("EventDisplay Tool: Error retrieving EventCutStatus,true from RecoEvent!",v_error,verbose); return false; }
  }

  //---------------------------------------------------------------
  //------------- Get Clustered Event information -----------------
  //---------------------------------------------------------------

  //Get Clustered PMT information (from ClusterFinder tool)
  if (draw_cluster){
    get_ok = m_data->CStore.Get("ClusterMap",m_all_clusters);
    if (not get_ok) { Log("EventDisplay Tool: Error retrieving ClusterMap from CStore, did you run ClusterFinder beforehand?",v_error,verbose); return false; }
    get_ok = m_data->CStore.Get("ClusterMapDetkey",m_all_clusters_detkey);
    if (not get_ok) { Log("EventDisplay Tool: Error retrieving ClusterMapDetkey from CStore, did you run ClusterFinder beforehand?",v_error,verbose); return false; }
  }
  //Get MRD Cluster information (from TimeClustering tool)
  if (draw_cluster_mrd){
    get_ok = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
    if (not get_ok) { Log("EventDisplay Tool: Error retrieving MrdTimeClusters map from CStore, did you run TimeClustering beforehand?",v_error,verbose); return false; }
    if (MrdTimeClusters.size()!=0){
      get_ok = m_data->CStore.Get("MrdDigitTimes",MrdDigitTimes);
      if (not get_ok) { Log("EventDisplay Tool: Error retrieving MrdDigitTimes map from CStore, did you run TimeClustering beforehand?",v_error,verbose); return false; }
      get_ok = m_data->CStore.Get("MrdDigitChankeys",mrddigitchankeysthisevent);
      if (not get_ok) { Log("EventDisplay Tool: Error retrieving MrdDigitChankeys, did you run TimeClustering beforehand",v_error,verbose); return false;}
    }
  }

  std::string logmessage;

  //---------------------------------------------------------------
  //------------- Evaluate Event Date & Triggertype ---------------
  //---------------------------------------------------------------
  
  //Set default run type to beam (if not stored in data)
  if (RunType == -1) RunType = user_run_type;
  if (runnumber == -1) runnumber = user_run_number;
  
  if (!isData){
    for (unsigned int i_trigger=0;i_trigger<TriggerData->size();i_trigger++){
      logmessage = "EventDisplay tool: Trigger name: "+TriggerData->at(i_trigger).GetName()+", Trigger time: "+std::to_string(TriggerData->at(i_trigger).GetTime().GetNs());
      Log(logmessage,v_message,verbose);
      trigger_label = TriggerData->at(i_trigger).GetName();
    }
    string_date_label = "MC/MC/MCMC";
  }
  else if (isData){
    trigger_label = map_runtypes.at(RunType);
    EventTankTime/=1.e6;	//convert EventTankTime to milliseconds
    boost::posix_time::ptime eventtime = *Epoch + boost::posix_time::time_duration(int(EventTankTime/1000/60/60),int(EventTankTime/1000/60)%60,int(EventTankTime/1000)%60,EventTankTime%1000);
    struct tm eventtime_tm = boost::posix_time::to_tm(eventtime);
    std::stringstream ss_date_label;
    ss_date_label << eventtime_tm.tm_year+1900 << "/" << eventtime_tm.tm_mon+1 << "/" << eventtime_tm.tm_mday << "-" << eventtime_tm.tm_hour << ":" << eventtime_tm.tm_min;
    string_date_label = ss_date_label.str();
  }

  //---------------------------------------------------------------
  //---------------Current event to be processed? -----------------
  //---------------------------------------------------------------

  logmessage = "EventDisplay tool: Event number "+std::to_string(evnum);
  Log(logmessage,v_message,verbose);

  if (single_event>=0 && i_loop ==2) return true;
  else i_loop = 2;
  passed_selection_cuts = EventCutStatus;
  if (selected_event && !passed_selection_cuts) return true;
  Log("EventDisplay tool: Event number check passed.",v_debug,verbose);

  //----------------------------------------------------------------------
  //-------------Initialise status variables for event -------------------
  //----------------------------------------------------------------------

  facc_hit=false;
  tank_hit=false;
  mrd_hit=false;
  draw_vertex_temp = false;
  draw_ring_temp = false;
  n_particles_ring.clear();
  current_n_polylines=0;

  //---------------------------------------------------------------
  //---------------Setup strings for this event -------------------
  //---------------------------------------------------------------

  std::string str_runnumber = std::to_string(runnumber);
  std::string str_evnumber = std::to_string(evnum);
  std::string str_trignumber;

  std::string plots = "_plots/";
  std::string str_evdisplay = "EvDisplay_";
  std::string str_png = ".png";
  std::string str_Run="Run";
  std::string str_Ev = "_Ev";
  std::string str_lappds = "_lappd";
  std::string str_pmts = "_pmt";
  std::string str_pmts2D = "_pmt2D";

  std::string out_file_dir = out_file+plots;
  std::string filename_evdisplay = out_file_dir+str_evdisplay+str_Run+str_runnumber+str_Ev+str_evnumber+str_png;
  std::string filename_pmts = out_file_dir+str_evdisplay+str_Run+str_runnumber+str_Ev+str_evnumber+str_pmts+str_png;
  std::string filename_pmts2D = out_file_dir+str_evdisplay+str_Run+str_runnumber+str_Ev+str_evnumber+str_pmts2D+str_png;
  std::string filename_lappds = out_file_dir+str_evdisplay+str_Run+str_runnumber+str_Ev+str_evnumber+str_lappds+str_png;

  //---------------------------------------------------------------
  //-------------clear charge & time containers -------------------
  //---------------------------------------------------------------
  mrddigittimesthisevent.clear();
  
  charge.clear();
  time.clear();
  hits.clear();
  hitpmt_detkeys.clear();
  hitmrd_detkeys.clear();

  time_lappd.clear();
  hits_LAPPDs.clear();
  charge_lappd.clear();

  for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    charge.emplace(detkey,0.);
    time.emplace(detkey,0.);
    hits.emplace(detkey,0);
  }

  for (int i_mrd=0;i_mrd<n_mrd_pmts;i_mrd++){
    unsigned long detkey = mrd_detkeys[i_mrd];
    mrddigittimesthisevent.emplace(detkey,0.);
  }

  //---------------------------------------------------------------
  //------------Get truth information (MCParticles) ---------------
  //---------------------------------------------------------------

  int n_flag0=0;
  if (!isData){

    particles_truevtx.clear();
    particles_truedir.clear();
    particles_pdg.clear();
    particles_color.clear();

    Log("EventDisplay tool: Loop through MCParticles.",v_message,verbose);
    for(unsigned int particlei=0; particlei<mcparticles->size(); particlei++){

      MCParticle aparticle = mcparticles->at(particlei);
      std::string logmessage = "EventDisplay tool: Particle # "+std::to_string(particlei)+", parent ID = "+std::to_string(aparticle.GetParentPdg())+", pdg = "+std::to_string(aparticle.GetPdgCode())+", flag = "+std::to_string(aparticle.GetFlag());
      Log(logmessage,v_message,verbose);

      if (std::find(particles_ring.begin(),particles_ring.end(),particlei)!=particles_ring.end()){
        int pdg_ring = aparticle.GetPdgCode();
        Position vtx_ring = aparticle.GetStartVertex();
        Direction dir_ring = aparticle.GetStartDirection();
        vtx_ring = vtx_ring - geom->GetTankCentre();
        particles_truevtx.push_back(vtx_ring);
        particles_truedir.push_back(dir_ring);
        particles_pdg.push_back(pdg_ring);
        particles_color.push_back(pdg_to_color[pdg_ring]);
        n_particles_ring.push_back(1);
      }
      else {
        particles_truevtx.push_back(aparticle.GetStartVertex());
        particles_truedir.push_back(aparticle.GetStartDirection());
        particles_pdg.push_back(aparticle.GetPdgCode());
        particles_color.push_back(-1);	//won't be plotted, so the color does not need to be sensible
        n_particles_ring.push_back(0);
      }

      if (aparticle.GetParentPdg() !=0 ) continue;
      if (aparticle.GetFlag() !=0 ) continue;

      if ((aparticle.GetPdgCode() == 11 || aparticle.GetPdgCode() == 13)){    //primary particle for Cherenkov tracking should be muon or electron
        n_flag0++;
        draw_vertex_temp = draw_vertex;
        draw_ring_temp = draw_ring;
        truevtx = aparticle.GetStartVertex();
        truedir = aparticle.GetStartDirection();
        truevtx_x = truevtx.X()-tank_center_x;
        truevtx_y = truevtx.Y()-tank_center_y;
        truevtx_z = truevtx.Z()-tank_center_z;
        truedir_x = truedir.X();
        truedir_y = truedir.Y();
        truedir_z = truedir.Z();
        std::string logmessage = "EventDisplay tool: True vtx: ( "+std::to_string(truevtx_x)+", "+std::to_string(truevtx_y)+", "+std::to_string(truevtx_z)+" ), true dir: ("+std::to_string(truedir_x)+", "+std::to_string(truedir_y)+", "+std::to_string(truedir_z)+")";
        Log(logmessage, v_message, verbose);

        //check if event happened outside of the tank volume
        if ((truevtx_y< min_y) || (truevtx_y>max_y) || sqrt(truevtx_x*truevtx_x+truevtx_z*truevtx_z)>tank_radius){
          Log("EventDisplay tool: Event vertex outside of Inner Structure! Don't plot projected vertex and ring...",v_message,verbose);
	  Log("EventDisplay tool: Vtx_x = "+to_string(truevtx_x)+", vtx_y = "+to_string(truevtx_y)+", vtx_z = "+to_string(truevtx_z),v_message,verbose);
          draw_vertex_temp=false;
          draw_ring_temp=false;
        }
        break;
      }
      else continue;
    }
  }
 
  //in case there are no primary particles, don't plot vertex and ring
  if (n_flag0==0) {
    draw_vertex_temp=false;
    draw_ring_temp=false;
  }

  //---------------------------------------------------------------
  //-------------------Iterate over PMTHits ------------------------
  //---------------------------------------------------------------

  // Separate cases: ANNIEEvent Store/ RecoEvent Store
  // The RecoEvent store is used the same way for data & MC (RecoDigits)
  // The ANNIEEvent store is used differently for data & MC (Hits/MCHits)
  // Clustered hits can only be selected in the ANNIEEvent store (RecoDigits should already be clustered)

   if (EventType == "ANNIEEvent"){
    if (!isData){
      int vectsize;
      total_hits_pmts=0;
      if (MCHits){
        vectsize = MCHits->size();
        for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
          unsigned long chankey = apair.first;
          Log("EventDisplay tool: Loop over MCHits: Chankey = "+std::to_string(chankey),v_debug,verbose);
          Detector* thistube = geom->ChannelToDetector(chankey);
          unsigned long detkey = thistube->GetDetectorID();
          Log("EventDisplay tool: Loop over MCHits: Detkey = "+std::to_string(detkey),v_debug,verbose);
          if (thistube->GetDetectorElement()=="Tank"){
	    if(thistube->GetTankLocation()=="OD") continue;		//don't plot OD PMTs (they are not included in the final design of ANNIE)
            std::vector<MCHit>& Hits = apair.second;
            int hits_pmt = 0;
            int wcsim_id;
            for (MCHit &ahit : Hits){
              charge[detkey] += ahit.GetCharge();
              time[detkey] += ahit.GetTime();
              hits_pmt++;
            }
            time[detkey]/=hits_pmt;         //use mean time of all hits on one PMT
            bool passed_lower_time_cut = (threshold_time_low == -999 || time[detkey] >= threshold_time_low);
            bool passed_upper_time_cut = (threshold_time_high == -999 || time[detkey] <= threshold_time_high);
            if (charge[detkey] >= threshold && passed_lower_time_cut && passed_upper_time_cut){
              hitpmt_detkeys.push_back(detkey);
              total_hits_pmts++;
            } else {
              charge[detkey] = 0;
            }
          }
        }
      } else {
        Log("EventDisplay tool: No MCHits!",v_warning,verbose);
        vectsize = 0;
      }
      logmessage = "EventDisplay tool: MCHits size = "+std::to_string(vectsize);
      Log(logmessage,v_message,verbose);

    } else {
    
      //data file case
      int vectsize;
      total_hits_pmts=0;
      if (!draw_cluster && Hits){
        vectsize = Hits->size();
        for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
          unsigned long chankey = apair.first;
          Log("EventDisplay tool: Loop over Hits: Chankey = "+std::to_string(chankey),v_debug,verbose);
          Detector* thistube = geom->ChannelToDetector(chankey);
          unsigned long detkey = thistube->GetDetectorID();
          Log("EventDisplay tool: Loop over Hits: Detkey = "+std::to_string(detkey),v_debug,verbose);
          if (thistube->GetDetectorElement()=="Tank"){
            if(thistube->GetTankLocation()=="OD") continue;             //don't plot OD PMTs (they are not included in the final design of ANNIE)
            std::vector<Hit>& Hits = apair.second;
            int hits_pmt = 0;
            int wcsim_id;
            for (Hit &ahit : Hits){
              double temp_charge = ahit.GetCharge();
              if (charge_format == "pe" && pmt_gains[detkey]>0) temp_charge /= pmt_gains[detkey];
              charge[detkey] += temp_charge;
              time[detkey] += ahit.GetTime();
              hits_pmt++;
            }
            time[detkey]/=hits_pmt;         //use mean time of all hits on one PMT
            bool passed_lower_time_cut = (threshold_time_low == -999 || time[detkey] >= threshold_time_low);
            bool passed_upper_time_cut = (threshold_time_high == -999 || time[detkey] <= threshold_time_high);
            if (charge[detkey] >= threshold && passed_lower_time_cut && passed_upper_time_cut){
              total_hits_pmts++;
              hitpmt_detkeys.push_back(detkey);
            } else {
              charge[detkey] = 0;
            }
          }
        }
      } else if (draw_cluster && m_all_clusters){
        int clustersize = m_all_clusters->size();
        Log("EventDisplay tool: Clustersize of m_all_clusters: "+std::to_string(clustersize),v_message,verbose);
        bool clusters_available = false;
        bool muon_available = false;
        if (clustersize != 0) clusters_available = true;
        if (clusters_available){
	  //determine the main cluster (max charge and in [0 ...2000ns] time window)
	  double max_cluster = 0;
          double max_charge_temp = 0;
          for(std::pair<double,std::vector<Hit>>&& apair : *m_all_clusters){
	    std::vector<Hit>&Hits = apair.second;
            double time_temp = 0;
            double charge_temp = 0;
            int hits_temp=0;
            for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
              hits_temp++;
              time_temp+=Hits.at(i_hit).GetTime();
              charge_temp+=Hits.at(i_hit).GetCharge();
            }
            if (hits_temp>0) time_temp/=hits_temp;
            if (time_temp > max_cluster_time || time_temp < min_cluster_time) continue;	//not a beam muon if not in primary window
	    if (charge_temp > max_charge_temp) {
              muon_available = true;
              max_charge_temp = charge_temp;
              max_cluster = apair.first;
              cluster_time = time_temp;
            }
  	  }
	  if (muon_available){
	  std::vector<Hit>& Hits = m_all_clusters->at(max_cluster);
          std::vector<unsigned long> detkeys = m_all_clusters_detkey->at(max_cluster);
          int hits_pmt = 0;
	  for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
	    Hit ahit = Hits.at(i_hit);
            unsigned long detkey = detkeys.at(i_hit);
            double temp_charge = ahit.GetCharge();
            if (charge_format == "pe" && pmt_gains[detkey]>0) temp_charge /= pmt_gains[detkey];
	    charge[detkey] += temp_charge;
            if (charge[detkey] < threshold) continue;	//don't use hits in cluster below the threshold
	    time[detkey] += ahit.GetTime();
	    hits[detkey]++;
	    if (std::find(hitpmt_detkeys.begin(),hitpmt_detkeys.end(),detkey)==hitpmt_detkeys.end()) hitpmt_detkeys.push_back(detkey);
	  }
	  for (unsigned int i_pmt=0; i_pmt < hitpmt_detkeys.size(); i_pmt++){
	    unsigned long detkey = hitpmt_detkeys.at(i_pmt);
            time[detkey] /= hits[detkey];
            total_hits_pmts++;
	  }
        }
      }
    }else {
      Log("EventDisplay tool: No Hits!",v_warning,verbose);
      vectsize = 0;
    }
  }
  } else if (EventType == "RecoEvent"){

      total_hits_pmts=0;
      total_hits_lappds=0;
      num_lappds_hit=0;
      maximum_lappds = 0;
      min_time_lappds = 99999.;
      maximum_time_lappds = 0;
      double mean_digittime = 0;
      std::vector<unsigned long> temp_lappd_detkeys;

      for (unsigned int i_digit = 0; i_digit < RecoDigits->size(); i_digit++){

        RecoDigit thisdigit = RecoDigits->at(i_digit);
        Position detector_pos = thisdigit.GetPosition();
        detector_pos.UnitToMeter();
        int digittype = thisdigit.GetDigitType();   //0 - PMTs, 1 - LAPPDs  
        double digitQ = thisdigit.GetCalCharge();
        double digitT = thisdigit.GetCalTime();
        bool isfiltered = thisdigit.GetFilterStatus();
        if (digittype == 0){
          int pmtid = thisdigit.GetDetectorID();
	  unsigned long chankey = pmt_tubeid_to_channelkey[pmtid];
	  Detector *det = geom->ChannelToDetector(chankey);
	  unsigned long detkey = det->GetDetectorID();
	  if (det->GetTankLocation()=="OD") continue;		//don't plot OD PMTs (not included in the final design of ANNIE!)
	  if (use_filtered_digits && !isfiltered) continue;     //omit unfiltered entries if specified
	  Log("EventDisplay tool: Reading in RecoEvent data: PMTid = "+std::to_string(pmtid)+", chankey = "+std::to_string(chankey)+", detkey = "+std::to_string(detkey),v_debug,verbose);
	  Log("EventDisplay tool: Reading in RecoEvent data: DigitQ = "+std::to_string(digitQ)+", digitT = "+std::to_string(digitT),v_debug,verbose);
          if (charge_format == "pe" && pmt_gains[detkey] > 0) digitQ /= pmt_gains[detkey];
          bool passed_lower_time_cut = (threshold_time_low == -999 || digitT >= threshold_time_low);
          bool passed_upper_time_cut = (threshold_time_high == -999 || digitT <= threshold_time_high);
          if (digitQ >= threshold && passed_lower_time_cut && passed_upper_time_cut){
            charge[detkey] = digitQ;
            time[detkey] = digitT;
            mean_digittime += digitT;
            hitpmt_detkeys.push_back(detkey);
            total_hits_pmts++;
          }
        } else if (digittype == 1){
          int lappdid = thisdigit.GetDetectorID();
          unsigned long detkey = lappd_tubeid_to_detectorkey[lappdid];
	  if (std::find(temp_lappd_detkeys.begin(),temp_lappd_detkeys.end(),detkey)==temp_lappd_detkeys.end()){
            temp_lappd_detkeys.push_back(detkey);
            num_lappds_hit++;
	  }
          if ((lappds_selected && active_lappds.count(detkey)) || !lappds_selected){
            if(hits_LAPPDs.count(detkey)==0) hits_LAPPDs.emplace(detkey,std::vector<Position>{});
            hits_LAPPDs.at(detkey).push_back(detector_pos);
            charge_lappd[detkey]+=digitQ;
            maximum_lappds++;
            total_hits_lappds++;
            if(time_lappd.count(detkey)==0){ time_lappd.emplace(detkey,std::vector<double>{}); }
            time_lappd[detkey].push_back(digitT);
            if (digitT > maximum_time_lappds) maximum_time_lappds = digitT;
            if (digitT < min_time_lappds) min_time_lappds = digitT;
          }
        }
      }
      if (total_hits_pmts > 0) mean_digittime/=total_hits_pmts; 
      cluster_time = mean_digittime;
    }

   if (num_lappds_hit > 0 || total_hits_pmts > 0) tank_hit = true;
  

  //NHits threshold cut
  int total_hits_pmts_above_thr=0;
  for (unsigned int i_pmt=0; i_pmt < hitpmt_detkeys.size(); i_pmt++){
      unsigned long detkey = hitpmt_detkeys.at(i_pmt);
      if (charge[detkey] > threshold) total_hits_pmts_above_thr++;
  }
  if (total_hits_pmts_above_thr < npmtcut) {
    Log("EventDisplay tool: Only "+std::to_string(total_hits_pmts_above_thr)+" PMTs hit above threshold. Required: "+std::to_string(npmtcut)+". Terminate this execute step.",v_message,verbose);
    return true;
   }
  
  //---------------------------------------------------------------
  //-------------clear charge & time histograms -------------------
  //---------------------------------------------------------------

  if (draw_histograms){
    canvas_pmt->Clear();
    canvas_pmt_supplementary->Clear();
    canvas_lappd->Clear();
  }

  if (charge_PMTs) charge_PMTs->Delete();
  if (time_PMTs) time_PMTs->Delete();
  if (charge_time_PMTs) charge_time_PMTs->Delete();
  for (int i=0;i<max_num_lappds;i++){
    if (charge_LAPPDs[i]) charge_LAPPDs[i]->Delete();
    if (time_LAPPDs[i]) time_LAPPDs[i]->Delete();
  }
  if (leg_charge) delete leg_charge;
  if (leg_time) delete leg_time;

  charge_PMTs = new TH1F("charge_PMTs","Charge of PMTs",pmt_Qbins,pmt_Qmin,pmt_Qmax);
  charge_PMTs->GetXaxis()->SetTitle("charge [p.e.]");
  time_PMTs = new TH1F("time_PMTs","PMTs time response",pmt_Tbins,pmt_Tmin,pmt_Tmax);
  time_PMTs->GetXaxis()->SetTitle("time [ns]");
  charge_time_PMTs = new TH2F("charge_time_PMTs","Charge vs. time PMTs",pmt_Tbins,pmt_Tmin,pmt_Tmax,pmt_Qbins,pmt_Qmin,pmt_Qmax);
  charge_time_PMTs->GetXaxis()->SetTitle("time [ns]");
  charge_time_PMTs->GetYaxis()->SetTitle("charge [p.e.]");

  for (int i=0;i<max_num_lappds;i++){
    std::string str_time_lappds = "time_lappds_det";
    std::string str_charge_lappds = "charge_lappds_det";
    int detkey = lappd_detkeys.at(i);
    std::string lappd_nr = std::to_string(detkey);
    std::string hist_time_lappds = str_time_lappds+lappd_nr;
    std::string hist_charge_lappds = str_charge_lappds+lappd_nr;
    time_LAPPDs[i] = new TH1F(hist_time_lappds.c_str(),"Time of LAPPDs",lappd_Tbins,lappd_Tmin,lappd_Tmax);
    time_LAPPDs[i]->GetXaxis()->SetTitle("time [ns]");
    //time_LAPPDs[i]->GetYaxis()->SetRangeUser(0,200);
    charge_LAPPDs[i] = new TH1F(hist_charge_lappds.c_str(),"Number of hits on LAPPDs",lappd_Qbins,lappd_Qmin,lappd_Qmax);
    charge_LAPPDs[i]->GetXaxis()->SetTitle("hits");
    //charge_LAPPDs[i]->GetYaxis()->SetRangeUser(0,20.);
  }  

  //---------------------------------------------------------------
  //-------------------Fill time hists ----------------------------
  //---------------------------------------------------------------

  double mean_pmt_time = 0;
  for (unsigned int i_pmt=0;i_pmt<hitpmt_detkeys.size();i_pmt++){
    unsigned long detkey = hitpmt_detkeys[i_pmt];
    if (charge[detkey]!=0) charge_PMTs->Fill(charge[detkey]);
    if (time[detkey]!=0) time_PMTs->Fill(time[detkey]);
    mean_pmt_time+=time[detkey];
    if (charge[detkey]!=0) charge_time_PMTs->Fill(time[detkey],charge[detkey]);
  }
  if (hitpmt_detkeys.size()>0) mean_pmt_time/=hitpmt_detkeys.size();


  //---------------------------------------------------------------
  //------------- Determine max+min values ------------------------
  //---------------------------------------------------------------

  maximum_pmts = 0;
  maximum_time_pmts = 0;
  min_time_pmts = 99999.;

  total_charge_pmts = 0;
  for (unsigned int i_pmt=0;i_pmt<hitpmt_detkeys.size();i_pmt++){
    unsigned long detkey = hitpmt_detkeys[i_pmt];
    if (charge[detkey]>maximum_pmts) maximum_pmts = charge[detkey];
    total_charge_pmts+=charge[detkey];
    if (time[detkey]>maximum_time_pmts) maximum_time_pmts = time[detkey];
    if (time[detkey]<min_time_pmts) min_time_pmts = time[detkey];
  }

  //---------------------------------------------------------------
  //-------------------Iterate over LAPPD hits --------------------
  //---------------------------------------------------------------

  if (!isData){ 

    //Look at MC files
    if (EventType == "ANNIEEvent"){
      total_hits_lappds=0;
      num_lappds_hit=0;
      maximum_lappds = 0;
      min_time_lappds = 99999.;
      maximum_time_lappds = 0;

      if(MCLAPPDHits){
        Log("EventDisplay Tool: Size of MCLAPPDhits = "+std::to_string(MCLAPPDHits->size()),v_message,verbose);
        if (!lappds_selected) num_lappds_hit = MCLAPPDHits->size();
        for (std::pair<unsigned long, std::vector<MCLAPPDHit>>&& apair : *MCLAPPDHits){
          unsigned long chankey = apair.first;
          Detector *det = geom->ChannelToDetector(chankey);
          if(det==nullptr){
            Log("EventDisplay Tool: LAPPD Detector not found! (chankey = "+std::to_string(chankey)+")",v_warning,verbose);
            continue;
          }
          int detkey = det->GetDetectorID();
	  if (lappds_selected && active_lappds.count(detkey)) num_lappds_hit++;
	  std::vector<MCLAPPDHit>& hits = apair.second;
          Log("EventDisplay tool: Loop over MCLAPPDHits: Detector Key = "+std::to_string(detkey),v_message,verbose);
          for (MCLAPPDHit& ahit : hits){
            std::vector<double> temp_pos = ahit.GetPosition();
            double x_lappd = temp_pos.at(0)-tank_center_x;
            double y_lappd = temp_pos.at(1)-tank_center_y;
            double z_lappd = temp_pos.at(2)-tank_center_z;
            double lappd_charge = 1.0;
            double t_lappd = ahit.GetTime();
            if ((lappds_selected && active_lappds.count(detkey)) || !lappds_selected){
              Log("EventDisplay tool: Loop over MCLAPPDHits: Detector Key LAPPD = "+std::to_string(detkey),v_debug,verbose);
              Log("EventDisplay tool: Loop over MCLAPPDHits: LAPPD hit at x="+std::to_string(x_lappd)+", y="+std::to_string(y_lappd)+", z="+std::to_string(z_lappd),v_debug,verbose);
              Position lappd_hit(x_lappd,y_lappd,z_lappd);
              if(hits_LAPPDs.count(detkey)==0) hits_LAPPDs.emplace(detkey,std::vector<Position>{});
              hits_LAPPDs.at(detkey).push_back(lappd_hit);
              charge_lappd[detkey]+=1;
              maximum_lappds++;
              total_hits_lappds++;
              if(time_lappd.count(detkey)==0){ time_lappd.emplace(detkey,std::vector<double>{}); }
              time_lappd[detkey].push_back(t_lappd);
              if (t_lappd > maximum_time_lappds) maximum_time_lappds = t_lappd;
              if (t_lappd < min_time_lappds) min_time_lappds = t_lappd;
            }
          }
        }
      } else {
        Log("EventDisplay tool: No MCLAPPDHits!", v_warning, verbose);
        num_lappds_hit = 0;
      }
    }
  } else {
  
    //Look at data files
    total_hits_lappds=0;
    num_lappds_hit=0;
    maximum_lappds = 0;
    min_time_lappds = 99999.;
    maximum_time_lappds = 0;

    if(LAPPDHits){
      Log("EventDisplay Tool: Size of MCLAPPDhits = "+std::to_string(LAPPDHits->size()),v_message,verbose);
      if (!lappds_selected) num_lappds_hit = LAPPDHits->size();
      for (std::pair<unsigned long, std::vector<LAPPDHit>>&& apair : *LAPPDHits){
        unsigned long chankey = apair.first;
        Detector *det = geom->ChannelToDetector(chankey);
        if(det==nullptr){
          Log("EventDisplay Tool: LAPPD Detector not found! (chankey = "+std::to_string(chankey)+")",v_warning,verbose);
          continue;
        }
        int detkey = det->GetDetectorID();
        if (lappds_selected && active_lappds.count(detkey)) num_lappds_hit++;
        std::vector<LAPPDHit>& hits = apair.second;
        Log("EventDisplay tool: Loop over LAPPDHits: Detector Key = "+std::to_string(detkey),v_message,verbose);
        for (LAPPDHit& ahit : hits){
          std::vector<double> temp_pos = ahit.GetPosition();
          double x_lappd = temp_pos.at(0)-tank_center_x;
          double y_lappd = temp_pos.at(1)-tank_center_y;
          double z_lappd = temp_pos.at(2)-tank_center_z;
          double lappd_charge = 1.0;
          double t_lappd = ahit.GetTime();
          if ((lappds_selected && active_lappds.count(detkey)) || !lappds_selected){
            Log("EventDisplay tool: Loop over LAPPDHits: Detector Key LAPPD = "+std::to_string(detkey),v_debug,verbose);
            Log("EventDisplay tool: Loop over LAPPDHits: LAPPD hit at x="+std::to_string(x_lappd)+", y="+std::to_string(y_lappd)+", z="+std::to_string(z_lappd),v_debug,verbose);
            Position lappd_hit(x_lappd,y_lappd,z_lappd);
            if (hits_LAPPDs.count(detkey)==0) hits_LAPPDs.emplace(detkey,std::vector<Position>{});
            hits_LAPPDs.at(detkey).push_back(lappd_hit);
            charge_lappd[detkey]+=1;
            maximum_lappds++;
            total_hits_lappds++;
            if (time_lappd.count(detkey)==0){ time_lappd.emplace(detkey,std::vector<double>{}); }
            time_lappd[detkey].push_back(t_lappd);
            if (t_lappd > maximum_time_lappds) maximum_time_lappds = t_lappd;
            if (t_lappd < min_time_lappds) min_time_lappds = t_lappd;
          }
        }
      }
    } else {
      Log("EventDisplay tool: No LAPPDHits!", v_warning, verbose);
      num_lappds_hit = 0;
    }
  }

  //---------------------------------------------------------------
  //-------------------Fill LAPPD hists ---------------------------
  //---------------------------------------------------------------

  int i_color = 1;
  for (auto&& ahisto : charge_LAPPDs){
    unsigned long histkey = ahisto.first;
    unsigned long detkey = lappd_detkeys.at(histkey); /*int histkey;
    std::vector<unsigned long>::iterator it = std::find(lappd_detkeys.begin(), lappd_detkeys.end(), detkey);
    if (it != lappd_detkeys.end()) histkey = std::distance(lappd_detkeys.begin(), it);
    else {
      Log("EventDisplay tool: Filling LAPPD histograms: Detkey " +std::to_string(detkey) +" not Found in Geometry class for LAPPDs!",v_error,verbose);
      continue;
    }*/
    if ((lappds_selected && active_lappds.count(detkey)==1) || !lappds_selected){
      charge_LAPPDs[histkey]->Fill(charge_lappd[detkey]);
      for (unsigned int i_time=0;i_time<time_lappd[detkey].size();i_time++){
        if (time_lappd[detkey].at(i_time)) time_LAPPDs[histkey]->Fill(time_lappd[detkey].at(i_time));
      } 
    time_LAPPDs[histkey]->SetLineColor(i_color);
    charge_LAPPDs[histkey]->SetLineColor(i_color);
    i_color++;
    }
  }

  //---------------------------------------------------------------
  //------------------ Overall max+min values ---------------------
  //---------------------------------------------------------------

  // for event display
  maximum_time_overall = (maximum_time_pmts>maximum_time_lappds)? maximum_time_pmts : maximum_time_lappds;
  min_time_overall = (min_time_pmts<min_time_lappds)? min_time_pmts : min_time_lappds;

  //for time and charge histograms
  bool max_gzero = false;
  int max_lappd_time=0;
  int max_lappd_charge=0;
  double temp_charge, temp_time;
  temp_charge = charge_LAPPDs[0]->GetMaximum();
  temp_time = time_LAPPDs[0]->GetMaximum();
  if (temp_time > 0.) max_gzero = true;
  for (int i_lappd=1;i_lappd<n_lappds;i_lappd++){
    if (charge_LAPPDs[i_lappd]->GetMaximum()>temp_charge){temp_charge=charge_LAPPDs[i_lappd]->GetMaximum();max_lappd_charge=i_lappd;}
    if (time_LAPPDs[i_lappd]->GetMaximum()>temp_time){temp_time=time_LAPPDs[i_lappd]->GetMaximum();max_lappd_time= i_lappd;max_gzero=true;}
  }

  //---------------------------------------------------------------
  //------------------ Draw histograms ----------------------------
  //---------------------------------------------------------------

  if (draw_histograms){
    canvas_lappd->Divide(2,1);
    std::stringstream ss_lappd_title, ss_lappd_name;
    if (!isData){
      ss_lappd_title << "lappd_ev"<<mcevnum<<"_"<<MCTriggernum;
      ss_lappd_name << "canvas_lappd_ev"<<mcevnum<<"_"<<MCTriggernum;
    } else {
      ss_lappd_title << "lappd_ev"<<evnum;
      ss_lappd_name << "canvas_lappd_ev"<<evnum;
    }
    canvas_lappd->SetTitle(ss_lappd_title.str().c_str());
    canvas_lappd->SetName(ss_lappd_name.str().c_str());
    canvas_lappd->cd(1);
    charge_LAPPDs[max_lappd_charge]->SetStats(0);
    charge_LAPPDs[max_lappd_charge]->Draw();
    leg_charge = new TLegend(0.6,0.6,0.9,0.9);
    leg_charge->SetLineColor(0);
    leg_charge->SetLineWidth(0);
    std::string lappd_str = "LAPPD ";
    std::string lappd_nr = std::to_string(lappd_detkeys[max_lappd_charge]);
    std::string lappd_label = lappd_str+lappd_nr;
    leg_charge->AddEntry(charge_LAPPDs[max_lappd_charge],lappd_label.c_str());
    for (int i_lappd=0;i_lappd<n_lappds;i_lappd++){
      if (i_lappd==max_lappd_charge) continue;
      unsigned long detkey = lappd_detkeys[i_lappd];
      if ((lappds_selected && active_lappds.count(detkey)==1) || !lappds_selected){
        charge_LAPPDs[i_lappd]->Draw("same");
        std::string lappd_nr = std::to_string(lappd_detkeys[i_lappd]);
        std::string lappd_label = lappd_str+lappd_nr;
        leg_charge->AddEntry(time_LAPPDs[i_lappd],lappd_label.c_str());
      }
    }
    leg_charge->Draw();
    canvas_lappd->cd(2);
    bool histogram_drawn=false;
    if (max_gzero){
      histogram_drawn=true;
      time_LAPPDs[max_lappd_time]->SetStats(0);
      time_LAPPDs[max_lappd_time]->Draw();
      leg_time = new TLegend(0.6,0.6,0.9,0.9);
      leg_time->SetLineColor(0);
      leg_time->SetLineWidth(0);
      lappd_nr = std::to_string(lappd_detkeys[max_lappd_time]);
      lappd_label = lappd_str+lappd_nr;
      leg_time->AddEntry(time_LAPPDs[max_lappd_time],lappd_label.c_str());
    } else {
      leg_time = new TLegend(0.6,0.6,0.9,0.9);
      leg_time->SetLineColor(0);
      leg_time->SetLineWidth(0);
    }
    for (int i_lappd=0;i_lappd<n_lappds;i_lappd++){
      if (i_lappd == max_lappd_time) continue;
      unsigned long detkey = lappd_detkeys[i_lappd];
      if ((lappds_selected && active_lappds.count(detkey)==1)|| !lappds_selected){
        if (!histogram_drawn){
          time_LAPPDs[i_lappd]->SetStats(0);
          histogram_drawn=true;
        }
        time_LAPPDs[i_lappd]->Draw("same");
        std::string lappd_nr = std::to_string(lappd_detkeys[i_lappd]);
        std::string lappd_label = lappd_str+lappd_nr;
        leg_time->AddEntry(time_LAPPDs[i_lappd],lappd_label.c_str());
      }
    }
    leg_time->Draw();
  }

  Log("EventDisplay tool: Maximum time LAPPDs: "+std::to_string(maximum_time_lappds)+", minimum time LAPPDs: "+std::to_string(min_time_lappds),v_message,verbose);

  //---------------------------------------------------------------
  //-------------------Iterate over MRD hits ----------------------
  //---------------------------------------------------------------

  maximum_time_mrd = -999999.;
  min_time_mrd = 9999999.;
  if (!isData){
    if(!TDCData){
      Log("EventDisplay tool: No TDC data to plot in Event Display!",v_message,verbose);
    } else {
      if(TDCData->size()==0){
        Log("EventDisplay tool: No TDC hits to plot in Event Display!",v_message,verbose);
      } else {
        Log("EventDisplay tool: Looping over FACC/MRD hits...Size of TDCData hits (MC): "+std::to_string(TDCData->size()),v_message,verbose);
        for(auto&& anmrdpmt : (*TDCData)){
          unsigned long chankey = anmrdpmt.first;
          Detector* thedetector = geom->ChannelToDetector(chankey);
	  unsigned long detkey = thedetector->GetDetectorID();
          if(thedetector->GetDetectorElement()!="MRD") facc_hit=true; // this is a veto hit, not an MRD hit.
          else {
	    mrd_hit = true;
	    double mrdtimes=0.;
	    int mrddigits=0;
            for(auto&& hitsonthismrdpmt : anmrdpmt.second){
              mrdtimes+=hitsonthismrdpmt.GetTime();
	      mrddigits++;
	    }
	    if (mrddigits > 0) mrdtimes/=mrddigits;
            hitmrd_detkeys.push_back(detkey);
            if (mrdtimes < min_time_mrd) min_time_mrd = mrdtimes;
	    if (mrdtimes > maximum_time_mrd) maximum_time_mrd = mrdtimes;
	    mrddigittimesthisevent[detkey] = mrdtimes;
          }
        }
      }
    }
  } else {
    Log("EventDisplay tool: Looping over MRD data",v_message,verbose);
    // Loop over data file
    if(!TDCData_Data){
      Log("EventDisplay tool: No TDC data to plot in Event Display!",v_message,verbose);
    } else {
      if(TDCData_Data->size()==0){
        Log("EventDisplay tool: No TDC hits to plot in Event Display!",v_message,verbose);
      } else if (!draw_cluster_mrd) {
          Log("EventDisplay tool: Looping over FACC/MRD hits...Size of TDCData hits (data): "+std::to_string(TDCData_Data->size()),v_message,verbose);
        for(auto&& anmrdpmt : (*TDCData_Data)){
          unsigned long chankey = anmrdpmt.first;
          Detector* thedetector = geom->ChannelToDetector(chankey);
          unsigned long detkey = thedetector->GetDetectorID();
          if(thedetector->GetDetectorElement()!="MRD") facc_hit=true; // this is a veto hit, not an MRD hit.
          else {
            mrd_hit = true;
            double mrdtimes=0.;
            int mrddigits=0;
            for(auto&& hitsonthismrdpmt : anmrdpmt.second){
              mrdtimes+=hitsonthismrdpmt.GetTime();
              mrddigits++;
            }
            if (mrddigits > 0) mrdtimes/=mrddigits;
            hitmrd_detkeys.push_back(detkey);
            if (mrdtimes < min_time_mrd) min_time_mrd = mrdtimes;
            if (mrdtimes > maximum_time_mrd) maximum_time_mrd = mrdtimes;
            mrddigittimesthisevent[detkey] = mrdtimes;
          }
        }
      } else {
        for(unsigned int thiscluster=0; thiscluster<MrdTimeClusters.size(); thiscluster++){

        std::vector<int> single_mrdcluster = MrdTimeClusters.at(thiscluster);
        int numdigits = single_mrdcluster.size();

        for(int thisdigit=0;thisdigit<numdigits;thisdigit++){

          int digit_value = single_mrdcluster.at(thisdigit);
          unsigned long chankey = mrddigitchankeysthisevent.at(digit_value);
	  Detector *thedetector = geom->ChannelToDetector(chankey);
	  unsigned long detkey = thedetector->GetDetectorID();
	  if (thedetector->GetDetectorElement()!="MRD") facc_hit = true;
	  else {
            mrd_hit = true;
            double mrdtimes=MrdDigitTimes.at(digit_value);
	    hitmrd_detkeys.push_back(detkey);
	    if (mrdtimes < min_time_mrd) min_time_mrd = mrdtimes;
            if (mrdtimes > maximum_time_mrd) maximum_time_mrd = mrdtimes;
            mrddigittimesthisevent[detkey] = mrdtimes;
         }
       }
      }
      //Check also in the cluster plot case whether there was a FMV hit
      if (!isData){
        if (TDCData){
	for(auto&& anmrdpmt : (*TDCData)){
          unsigned long chankey = anmrdpmt.first;
          Detector* thedetector = geom->ChannelToDetector(chankey);
          unsigned long detkey = thedetector->GetDetectorID();
          if(thedetector->GetDetectorElement()!="MRD") facc_hit=true;
        }
       }
      } else {
        if (TDCData_Data){
        for(auto&& anmrdpmt : (*TDCData_Data)){
          unsigned long chankey = anmrdpmt.first;
          Detector* thedetector = geom->ChannelToDetector(chankey);
          unsigned long detkey = thedetector->GetDetectorID();
          if(thedetector->GetDetectorElement()!="MRD") facc_hit=true;
        }
       }
     }
    }
   }
  }


  //Calculate mean MRD cluster time
  double mean_mrd_time = 0;
  for (unsigned int i = 0; i< hitmrd_detkeys.size(); i++){
    mean_mrd_time+=mrddigittimesthisevent[hitmrd_detkeys.at(i)];
  }
  if (hitmrd_detkeys.size()>0) mean_mrd_time/=hitmrd_detkeys.size();
  if (hitmrd_detkeys.size()>0 && hitpmt_detkeys.size()>0){
    //Use mean_mrd_time for something?
  }


  //---------------------------------------------------------------
  //------------------ Draw Event Display -------------------------
  //---------------------------------------------------------------

  Log("EventDisplay tool: Drawing Event Display.",v_message,verbose);

  canvas_ev_display->Clear();
  canvas_ev_display->Divide(1,1);
  make_gui();
  draw_event();
  std::stringstream ss_evdisplay_title, ss_evdisplay_name;
  if (!isData){
    ss_evdisplay_title << "evdisplay_ev"<<mcevnum<<"_"<<MCTriggernum;
    ss_evdisplay_name << "canvas_evdisplay_ev"<<mcevnum<<"_"<<MCTriggernum;
  } else {
    ss_evdisplay_title <<"evdisplay_ev"<<evnum;
    ss_evdisplay_name << "canvas_evdisplay_ev"<<evnum;
  }
  canvas_ev_display->SetTitle(ss_evdisplay_title.str().c_str());
  canvas_ev_display->SetName(ss_evdisplay_name.str().c_str());
  canvas_ev_display->Modified();
  canvas_ev_display->Update();

  if (draw_histograms){
    canvas_pmt->Divide(2,1);
    std::stringstream ss_pmt_name, ss_pmt_title, ss_pmt2D_title, ss_pmt2D_name;
    if (!isData){
      ss_pmt_title << "pmt_ev"<<mcevnum<<"_"<<MCTriggernum;
      ss_pmt_name << "canvas_pmt_ev"<<mcevnum<<"_"<<MCTriggernum;
    } else {
      ss_pmt_title << "pmt_ev"<<evnum;
      ss_pmt_name << "canvas_pmt_ev"<<evnum;
    }
    canvas_pmt->SetTitle(ss_pmt_title.str().c_str());
    canvas_pmt->SetName(ss_pmt_name.str().c_str());
    canvas_pmt->cd(1);
    charge_PMTs->SetStats(0);
    charge_PMTs->Draw();
    canvas_pmt->cd(2);
    time_PMTs->SetStats(0);
    time_PMTs->Draw();
    canvas_pmt->Modified();
    canvas_pmt->Update();
    canvas_pmt_supplementary->cd();
    if (!isData){
      ss_pmt2D_title << "pmt2D_ev"<<mcevnum<<"_"<<MCTriggernum;
      ss_pmt2D_name << "canvas_pmt2D_ev"<<mcevnum<<"_"<<MCTriggernum;
    } else {
      ss_pmt2D_title << "pmt2D_ev"<<evnum;
      ss_pmt2D_name << "canvas_pmt2D_ev"<<evnum;
    }
    canvas_pmt_supplementary->SetTitle(ss_pmt2D_title.str().c_str());
    canvas_pmt_supplementary->SetName(ss_pmt2D_name.str().c_str());
    charge_time_PMTs->SetStats(0);
    charge_time_PMTs->Draw("colz");
    canvas_pmt_supplementary->Modified();
    canvas_pmt_supplementary->Update();
    canvas_lappd->Modified();
    canvas_lappd->Update();
  }

  if (save_plots){
    if (output_format == "image") canvas_ev_display->SaveAs(filename_evdisplay.c_str());
    else {
      Log("EventDisplay tool: Saving canvas to root-file",v_message,verbose);
      root_file->cd();
      canvas_ev_display->Write();
    }
    if (draw_histograms){
      if (output_format == "image"){
        canvas_pmt->SaveAs(filename_pmts.c_str());
        canvas_pmt_supplementary->SaveAs(filename_pmts2D.c_str());
        canvas_lappd->SaveAs(filename_lappds.c_str());
      } else {
        root_file->cd();
        canvas_pmt->Write();
        canvas_pmt_supplementary->Write();
        canvas_lappd->Write();
      }
    }
  }

  if (user_input){
    std::string in_string;
    if (!isData){
      std::cout <<"End of current event ("<<mcevnum<<"). Action? ----> "<<std::endl;
      std::cout <<"P: EventDisplay for Previous Event (previous event: "<<mcevnum-1<<")"<<std::endl;
      std::cout <<"N: EventDisplay for Next Event (next event: "<<mcevnum+1<<")"<<std::endl;
    } else {
      std::cout <<"End of current event ("<<evnum<<"). Action? ----> "<<std::endl;
      std::cout <<"P: EventDisplay for Previous Event (previous event: "<<evnum-1<<")"<<std::endl;
      std::cout <<"N: EventDisplay for Next Event (next event: "<<evnum+1<<")"<<std::endl;
    }
    std::cout <<"Exxxx: EventDisplay for Event xxxx"<<std::endl;
    std::cout <<"Q: Quit EventDisplay ToolChain"<<std::endl;
    std::cin >> in_string;
    ParseUserInput(in_string);
  }

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (end of execute): "<<std::endl;
  //gObjectTable->Print();

  return true;
}


bool EventDisplay::Finalise(){

  root_file->cd();

  //-----------------------------------------------------------------
  //---------------- Delete remaining objects -----------------------
  //-----------------------------------------------------------------

  delete_canvas_contents();
  Log("EventDisplay tool: Clear canvas",v_debug,verbose);
  canvas_ev_display->Clear();
  Log("EventDisplay tool: Close canvas",v_debug,verbose);
  canvas_ev_display->Close();

  Log("EventDisplay tool: Terminate & Delete TApplication",v_debug,verbose);
  if (use_tapplication) {
    app_event_display->Terminate();
    delete app_event_display;
  }
  Log("EventDisplay tool: Delete additional histograms",v_debug,verbose);
  if (draw_histograms){
    delete leg_charge;
    delete leg_time;
  }
  Log("EventDisplay tool: Delete canvases",v_debug,verbose);
  
  delete canvas_ev_display;
  if (draw_histograms){
    delete canvas_pmt;
    delete canvas_pmt_supplementary;
    delete canvas_lappd;
  }

  Log("EventDisplay tool: Delete root file",v_debug,verbose);
  if (output_format == "root"){
    root_file->Close();
    delete root_file;
  }

  delete Epoch;

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (end of finalise): "<<std::endl;
  //gObjectTable->Print();

  return true;

}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-------------------------   HELPER FUNCTIONS --------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

void EventDisplay::make_gui(){

    //setup GUI for plotting Event Displays

    delete_canvas_contents();

    Log("EventDisplay tool: Make gui, Bird index = "+std::to_string(Bird_Idx),v_debug,verbose);

    //draw top circle
    p1 = (TPad*) canvas_ev_display->cd(1);
    canvas_ev_display->Draw();
    gPad->SetFillColor(0);
    p1->Range(0,0,1,1);
    top_circle = new TEllipse(0.5,0.5+(max_y/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
    top_circle->SetFillColor(1);
    top_circle->SetLineColor(1);
    top_circle->SetLineWidth(1);
    top_circle->Draw();
    
    //draw bulk
    //box = new TBox(0.5-TMath::Pi()*size_top_drawing,0.5-tank_height/tank_radius*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,0.5+tank_height/tank_radius*size_top_drawing);
    box = new TBox(0.5-TMath::Pi()*size_top_drawing,0.5-fabs(min_y)/tank_radius*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,0.5+max_y/tank_radius*size_top_drawing);
    box->SetFillColor(1);
    box->SetLineColor(1);
    box->SetLineWidth(1);
    box->Draw();

    //draw lower circle
    bottom_circle = new TEllipse(0.5,0.5-(fabs(min_y)/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
    bottom_circle->SetFillColor(1);
    bottom_circle->SetLineColor(1);
    bottom_circle->SetLineWidth(1);
    bottom_circle->Draw();

    //draw MRD schematic
    double xmin_mrd_top = 0.65;
    double xmin_mrd_side = 0.1;
    double ymin_mrd = 0.05;
    double ydiff_mrd = 0.25;
    double xdiff_mrd = 0.25;
    double shift = 0.005;

    for (unsigned int i_mrd = 0; i_mrd < mrd_detkeys.size(); i_mrd++){

      unsigned long detkey = mrd_detkeys.at(i_mrd);
      int orientation = mrd_orientation[detkey];
      int half = mrd_half[detkey];

      std::vector<double> mrd_Depth, mrd_Height;
      mrd_Depth = mrd_z[detkey];
      mrd_Height = (orientation == 1)? mrd_x[detkey] : mrd_y[detkey];
      if (half==1) {
        mrd_Depth[0] += 8*shift;
        mrd_Depth[1] += 12*shift;
      } else mrd_Depth[1] += 4*shift;
      double xmin_mrd = (orientation == 1)? xmin_mrd_top : xmin_mrd_side;
      double min_mrd_height = (orientation == 1)? min_mrd_x : min_mrd_y;
      double mrd_height_difference = (orientation == 1)? mrd_diffx : mrd_diffy;

      double x1 = xmin_mrd + (mrd_Depth[0]-min_mrd_z)*(xdiff_mrd - 2*shift)/mrd_diffz + shift;
      double y1 = ymin_mrd + (mrd_Height[0]-min_mrd_height)*(ydiff_mrd - 2*shift)/mrd_height_difference + shift;
      double x2 = xmin_mrd + shift + (mrd_Depth[1]-min_mrd_z)*(xdiff_mrd - 2*shift)/(mrd_diffz);
      double y2 = ymin_mrd + shift + (mrd_Height[1]-min_mrd_height)*(ydiff_mrd - 2*shift)/(mrd_height_difference);

      TBox *mrd_paddle = new TBox(x1,y1,x2,y2);
      mrd_paddle->SetLineColor(1);
      mrd_paddle->SetLineWidth(1);
      mrd_paddle->SetFillColor(0);
      mrd_paddle->Draw();
      mrd_paddles.emplace(detkey,mrd_paddle);

      TBox *box_mrd_paddle = new TBox(x1,y1,x2,y2);
      box_mrd_paddle->SetLineColor(1);
      box_mrd_paddle->SetLineWidth(1);
      box_mrd_paddle->SetFillStyle(0);
      box_mrd_paddle->Draw();
      box_mrd_paddles.emplace(detkey,box_mrd_paddle);


    }
    
    title_mrd_top = new TText(0.75,0.32,"MRD Top view");
    title_mrd_top->SetTextSize(0.015);
    title_mrd_top->Draw();
    title_mrd_side = new TText(0.19,0.32,"MRD Side view");
    title_mrd_side->SetTextSize(0.015);
    title_mrd_side->Draw();

    //update Event Display canvas
    canvas_ev_display->Modified();
    canvas_ev_display->Update();

}

void EventDisplay::draw_event(){

    marker_pmts_top.clear();
    marker_pmts_bottom.clear();
    marker_pmts_wall.clear();
    marker_lappds.clear();

    //draw everything necessary for event

    canvas_ev_display->cd(1);
    draw_event_PMTs();
    Log("EventDisplay tool: Drawing PMTs finished.",v_message,verbose);

    draw_event_LAPPDs();
    Log("EventDisplay tool: Drawing LAPPDs finished.",v_message,verbose);

    draw_event_MRD();
    Log("EventDisplay tool: Drawing MRD finished.",v_message,verbose);

    if (text_box) draw_event_box();
    Log("EventDisplay tool: Drawing event box finished.",v_message,verbose);

    draw_pmt_legend();
    Log("EventDisplay tool: Drawing PMT legend finished.",v_message,verbose);

    if (!isData) draw_lappd_legend();
    Log("EventDisplay tool: Drawing lappd legend finished.",v_message,verbose);

    draw_mrd_legend();
    Log("EventDisplay tool: Drawing mrd legend finished.",v_message,verbose);

    draw_schematic_detector();
    Log("EventDisplay tool: Drawing schematic detector finished.",v_message,verbose);

    if (!isData) draw_particle_legend();
    Log("EventDisplay tool: Drawing particle legend finished.",v_message,verbose);

    if (draw_vertex_temp) {
      for (unsigned int i_particle = 0; i_particle < n_particles_ring.size(); i_particle++){
      if (n_particles_ring.at(i_particle)==1) {
        draw_true_vertex(particles_truevtx.at(i_particle),particles_truedir.at(i_particle),particles_color.at(i_particle));
      }
    }
        Log("EventDisplay tool: Drawing true vertices finished.",v_message,verbose);
    }

    if (draw_ring_temp) {
      for (unsigned int i_particle = 0; i_particle < n_particles_ring.size(); i_particle++){
	if (n_particles_ring.at(i_particle)==1) {
	  Log("EventDisplay tool: Drawing ring: particle # "+std::to_string(i_particle+1)+", truevtx = ("+std::to_string(particles_truevtx.at(i_particle).X())+","+std::to_string(particles_truevtx.at(i_particle).Y())+","+std::to_string(particles_truevtx.at(i_particle).Z())+"), true dir = ("+std::to_string(particles_truedir.at(i_particle).X())+","+std::to_string(particles_truedir.at(i_particle).Y())+","+std::to_string(particles_truedir.at(i_particle).Z())+"), color = "+std::to_string(particles_color.at(i_particle)),v_debug,verbose);
	  draw_true_ring(particles_truevtx.at(i_particle),particles_truedir.at(i_particle),particles_color.at(i_particle));
        }  
      }
    }
    else current_n_polylines=0;

    //need to commit tool to calculate mean properties of events (later commit)
    //Position pmtBaryQ;
    //m_data->CStore.Get("pmtBaryQ",pmtBaryQ);
    //draw_barycenter(pmtBaryQ);
    //Log("EventDisplay tool: Drawing charge barycenter finished.",v_message,verbose);

    Log("EventDisplay tool: Drawing true ring finished.",v_message,verbose);

}

void EventDisplay::draw_event_box(){

    //draw text box with general information about event

    text_event_info = new TPaveText(0.03,0.7,0.35,0.97);
    text_event_info->AddText("ANNIE Phase II");
    ((TText*)text_event_info->GetListOfLines()->Last())->SetTextColor(kViolet+2);
    std::string annie_event = "ANNIE Event: ";
    std::string annie_run = "ANNIE Run: ";
    std::string pmts_str = "PMTs: ";
    std::string lappds_str = "LAPPDs: ";
    std::string modules_str = " module(s) / ";
    std::string hits2_str = " hits";
    std::string hits_str = " hits / ";
    std::string cluster_time_str = "Cluster Time: ";
    std::string charge_str;
    if (charge_format == "pe") charge_str = " p.e.";
    else charge_str = " nC";
    //std::string annie_subrun = "ANNIE Subrun: ";
    std::string annie_time = "Trigger Time: ";
    std::string annie_time_unit = " [ns]";
    std::string cluster_time_unit = " ns";
    std::string annie_date = "Date: ";
    std::string trigger_str = "Trigger: ";
    std::string annie_run_number = std::to_string(runnumber);
    std::string annie_event_number = std::to_string(evnum);
    std::string cluster_time_number = std::to_string(cluster_time);
    std::string total_charge_str;
    if (charge_format == "pe") total_charge_str = std::to_string(int(total_charge_pmts));
    else total_charge_str = std::to_string(round(total_charge_pmts*10000.)/10000.);
    std::string total_hits_str = std::to_string(total_hits_pmts);
    //std::string annie_subrun_number = std::to_string(subrunnumber);
    std::string annie_time_number, annie_time_label, cluster_time_label;
    if (!isData) annie_time_number = std::to_string(EventTime->GetNs()); //TODO: Remove MC restriction once EventTime is available for data files
    std::string lappd_hits_number = std::to_string(total_hits_lappds);
    std::string lappd_numbers_str = std::to_string(num_lappds_hit);
    std::string annie_run_label = annie_run+annie_run_number;
    std::string annie_event_label = annie_event+annie_event_number;
    std::string pmts_label = pmts_str+total_hits_str+hits_str+total_charge_str+charge_str;
    //std::string annie_subrun_label = annie_subrun+annie_subrun_number;
    if (!isData) annie_time_label = annie_time+annie_time_number+annie_time_unit;
    if (draw_cluster) cluster_time_label = cluster_time_str+cluster_time_number+cluster_time_unit;
    std::string lappd_hits_label = lappds_str+lappd_numbers_str+modules_str+lappd_hits_number+hits2_str;
    std::string trigger_text_label = trigger_str+trigger_label;
    std::string date_text_label = annie_date + string_date_label;
    text_event_info->AddText(date_text_label.c_str());         //TEMPORARY: get date/time stamp from somewhere (TriggerData, as soon as implemented)
    if (!isData) text_event_info->AddText(annie_time_label.c_str());
    text_event_info->AddText(annie_run_label.c_str());
    //text_event_info->AddText(annie_subrun_label.c_str());
    text_event_info->AddText(annie_event_label.c_str());
    text_event_info->AddText(pmts_label.c_str());
    text_event_info->AddText(lappd_hits_label.c_str());
    if (draw_cluster) text_event_info->AddText(cluster_time_label.c_str());
    text_event_info->AddText(trigger_text_label.c_str());
    //((TText*)text_event_info->GetListOfLines()->Last())->SetTextColor(kRed+1);
    text_event_info->SetTextFont(40);  //helvetica-medium-r-normal arial.ttf
    text_event_info->SetBorderSize(1);
    text_event_info->SetFillColor(0);
    text_event_info->SetLineWidth(0);
    text_event_info->SetLineColor(0);
    text_event_info->Draw();
    
  }

  void EventDisplay::draw_pmt_legend(){

    //draw pmt legend on the left side

    if (mode == "Charge") pmt_title = new TPaveLabel(0.05,0.5+max_y/tank_radius*size_top_drawing-0.03,0.15,0.5+max_y/tank_radius*size_top_drawing,"charge [PMTs]","l");
    else if (mode == "Time") pmt_title = new TPaveLabel(0.05,0.5+max_y/tank_radius*size_top_drawing-0.03,0.15,0.5+max_y/tank_radius*size_top_drawing,"time [PMTs]","l");
    pmt_title->SetTextFont(40); 
    pmt_title->SetFillColor(0);
    pmt_title->SetTextColor(1);
    pmt_title->SetBorderSize(0);
    pmt_title->SetTextAlign(11);
    pmt_title->Draw();
    for (int co=0; co<255; co++)
    {
        float yc = 0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*co/255;
        TMarker *colordot = new TMarker(0.08,yc,21);
        colordot->SetMarkerColor(Bird_Idx+co);
        colordot->SetMarkerSize(3.);
        colordot->Draw();
        vector_colordot.push_back(colordot);
    }
    std::string max_charge_pre, pe_string;
    if (charge_format == "pe"){
      max_charge_pre = std::to_string(int(maximum_pmts));  //TEMP
      pe_string = " p.e.";
    } else {
      max_charge_pre = std::to_string(round(maximum_pmts*10000.)/10000.);
      pe_string = " nC";
    }
    std::string max_charge = max_charge_pre+pe_string;
    std::string max_time_pre = std::to_string(int(maximum_time_overall));
    std::string time_string = " ns";
    std::string max_time;
    if (threshold_time_high==-999) max_time = max_time_pre+time_string;
    else max_time = std::to_string(int(threshold_time_high))+time_string;
    if (mode == "Charge") max_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_charge.c_str(),"L");
    else if (mode == "Time") max_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_time.c_str(),"L");
    max_text->SetFillColor(0);
    max_text->SetTextColor(1);
    max_text->SetTextFont(40);
    max_text->SetBorderSize(0);
    max_text->SetTextAlign(11);
    max_text->Draw();

    //std::string min_charge_pre = (threshold==-999)? "0" : std::to_string(int(threshold));
    std::string min_charge_pre;
    if (threshold == -999) min_charge_pre = "0";
    else if (charge_format == "pe") min_charge_pre = std::to_string(int(threshold));
    else min_charge_pre = std::to_string(round(threshold*10000.)/10000.);
    std::string min_charge = min_charge_pre+pe_string;
    std::string min_time_pre = std::to_string(int(min_time_overall));
    std::string min_time;
    if (threshold_time_low==-999) min_time = min_time_pre+time_string;
    else min_time = std::to_string(int(threshold_time_low))+time_string;
    if (mode == "Charge") min_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,min_charge.c_str(),"L");
    else if (mode == "Time") min_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,min_time.c_str(),"L");
    min_text->SetFillColor(0);
    min_text->SetTextColor(1);
    min_text->SetTextFont(40);
    min_text->SetBorderSize(0);
    min_text->SetTextAlign(11);
    min_text->Draw();

  }

  void EventDisplay::draw_lappd_legend(){

    //draw LAPPD legend on the right side of the event display

    if (mode == "Charge") lappd_title = new TPaveLabel(0.85,0.5+max_y/tank_radius*size_top_drawing-0.03,0.95,0.5+max_y/tank_radius*size_top_drawing,"charge [LAPPDs]","l");
    else if (mode == "Time") lappd_title = new TPaveLabel(0.85,0.5+max_y/tank_radius*size_top_drawing-0.03,0.95,0.5+max_y/tank_radius*size_top_drawing,"time [LAPPDs]","l");
    lappd_title->SetTextFont(40);
    lappd_title->SetFillColor(0);
    lappd_title->SetTextColor(1);
    lappd_title->SetBorderSize(0);
    lappd_title->SetTextAlign(11);
    lappd_title->Draw();
    for (int co=0; co<255; co++)
    {
        float yc = 0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*co/255;
        TMarker *colordot = new TMarker(0.85,yc,21);
        colordot->SetMarkerColor(Bird_Idx+co);
        colordot->SetMarkerSize(3.);
        colordot->Draw();
        vector_colordot_lappd.push_back(colordot);
    }
    
    std::string max_time_pre = std::to_string(int(maximum_time_overall));
    std::string time_string = " ns";
    std::string max_time_lappd;
    if (threshold_time_high==-999) max_time_lappd = max_time_pre+time_string;
    else max_time_lappd = std::to_string(int(threshold_time_high))+time_string;
    if (mode == "Charge") max_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,"1 hit","L");
    else if (mode == "Time") max_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_time_lappd.c_str(),"L");
    max_lappd->SetFillColor(0);
    max_lappd->SetTextColor(1);
    max_lappd->SetTextFont(40);
    max_lappd->SetBorderSize(0);
    max_lappd->SetTextAlign(11);
    max_lappd->Draw();

    //std::string min_time_pre = std::to_string(int(min_time_lappds));
    std::string min_time_lappd;
    std::string min_time_pre = std::to_string(int(min_time_overall));
    if (threshold_time_low==-999) min_time_lappd = min_time_pre+time_string;
    else min_time_lappd = std::to_string(int(threshold_time_low))+time_string; 
    if (mode == "Charge") min_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,"0 hits","L");
    else if (mode == "Time") min_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,min_time_lappd.c_str(),"L");
    min_lappd->SetFillColor(0);
    min_lappd->SetTextColor(1);
    min_lappd->SetTextFont(40);
    min_lappd->SetBorderSize(0);
    min_lappd->SetTextAlign(11);
    min_lappd->Draw();

  }

  void EventDisplay::draw_mrd_legend(){


   //draw MRD legend on the right side of the top view of the MRD
   //
    mrd_title = new TPaveLabel(0.90,0.27,0.98,0.30,"time [MRD]","l");
    mrd_title->SetTextFont(40);
    mrd_title->SetFillColor(0);
    mrd_title->SetTextColor(1);
    mrd_title->SetBorderSize(0);
    mrd_title->SetTextAlign(11);
    mrd_title->Draw();

    //draw color coding
    for (int co=0; co<255; co++)
    {
        float yc = 0.1 + 0.15*co/255;
        TMarker *colordot = new TMarker(0.92,yc,21);
        colordot->SetMarkerColor(Bird_Idx+co);
        colordot->SetMarkerSize(2.);
        colordot->Draw();
        vector_colordot_mrd.push_back(colordot);
    }

    std::string max_time_pre = std::to_string(int(maximum_time_mrd));
    std::string time_string = " ns";
    std::string max_time_mrd;
    if (threshold_time_high_mrd==-999) max_time_mrd = max_time_pre+time_string;
    else max_time_mrd = std::to_string(int(threshold_time_high_mrd))+time_string;
    max_mrd = new TPaveLabel(0.94,0.235,0.98,0.25,max_time_mrd.c_str(),"L");
    max_mrd->SetFillColor(0);
    max_mrd->SetTextColor(1);
    max_mrd->SetTextFont(40);
    max_mrd->SetBorderSize(0);
    max_mrd->SetTextAlign(11);
    max_mrd->Draw();

    std::string minimum_time_mrd;
    std::string min_time_pre = std::to_string(int(min_time_mrd));
    if (threshold_time_low_mrd==-999) minimum_time_mrd = min_time_pre+time_string;
    else minimum_time_mrd = std::to_string(int(threshold_time_low_mrd))+time_string;
    min_mrd = new TPaveLabel(0.94,0.10,0.98,0.115,minimum_time_mrd.c_str(),"L");
    min_mrd->SetFillColor(0);
    min_mrd->SetTextColor(1);
    min_mrd->SetTextFont(40);
    min_mrd->SetBorderSize(0);
    min_mrd->SetTextAlign(11);
    min_mrd->Draw();

  }

  void EventDisplay::draw_event_PMTs(){

    //draw PMT event markers

    Log("EventDisplay tool: Draw PMT event markers.",v_message,verbose)
    ;
    //clear already existing marker vectors
    marker_pmts_top.clear();
    marker_pmts_bottom.clear();
    marker_pmts_wall.clear();

    //calculate marker coordinates
    for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){

      unsigned long detkey = pmt_detkeys[i_pmt];
      double x[1];
      double y[1];
      if (charge[detkey]<threshold || ( threshold_time_high!=-999 && time[detkey]>threshold_time_high) || (threshold_time_low!=-999 && time[detkey]<threshold_time_low) || std::find(hitpmt_detkeys.begin(),hitpmt_detkeys.end(),detkey)==hitpmt_detkeys.end())  continue;       // if PMT does not have the charge required by threhold, discard
      if (fabs(y_pmt[detkey]-max_y)<0.01){

        //draw PMTs on the top of tank
        x[0]=0.5-size_top_drawing*x_pmt[detkey]/tank_radius;
        y[0]=0.5+(max_y/tank_radius+1)*size_top_drawing-size_top_drawing*z_pmt[detkey]/tank_radius;
        TPolyMarker *marker_top = new TPolyMarker(1,x,y,"");
        if (mode == "Charge") color_marker = Bird_Idx+int((charge[detkey]-threshold)/(maximum_pmts-threshold)*254);
        else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(maximum_time_overall-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high != -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(threshold_time_high-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low != -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(maximum_time_overall-threshold_time_low)*254);
        else if (mode == "Time") color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
        marker_top->SetMarkerColor(color_marker);
        marker_top->SetMarkerStyle(8);
        marker_top->SetMarkerSize(0.8*marker_size);
        marker_pmts_top.push_back(marker_top);
      }  else if (fabs(y_pmt[detkey]-min_y)<0.01 || fabs(y_pmt[detkey]+1.30912)<0.01){

        //draw PMTs on the bottom of tank
        x[0]=0.5-size_top_drawing*x_pmt[detkey]/tank_radius;
        y[0]=0.5-(fabs(min_y)/tank_radius+1)*size_top_drawing+size_top_drawing*z_pmt[detkey]/tank_radius;
        TPolyMarker *marker_bottom = new TPolyMarker(1,x,y,"");
        if (mode == "Charge") color_marker = Bird_Idx+int((charge[detkey]-threshold)/(maximum_pmts-threshold)*254);
        else if (mode == "Time" && threshold_time_high == -999 && threshold_time_low == -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(maximum_time_overall-min_time_overall)*254); 
        else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high != -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(threshold_time_high-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low != -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(maximum_time_overall-threshold_time_low)*254);
        else if (mode == "Time") color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
        marker_bottom->SetMarkerColor(color_marker);
        marker_bottom->SetMarkerStyle(8);
        marker_bottom->SetMarkerSize(0.8*marker_size);
        marker_pmts_bottom.push_back(marker_bottom);
      } else {

        //draw PMTs on the tank bulk
        double phi;
        if (x_pmt[detkey]>0 && z_pmt[detkey]>0) phi = atan(z_pmt[detkey]/x_pmt[detkey])+TMath::Pi()/2;
        else if (x_pmt[detkey]>0 && z_pmt[detkey]<0) phi = atan(x_pmt[detkey]/-z_pmt[detkey]);
        else if (x_pmt[detkey]<0 && z_pmt[detkey]<0) phi = 3*TMath::Pi()/2+atan(z_pmt[detkey]/x_pmt[detkey]);
        else if (x_pmt[detkey]<0 && z_pmt[detkey]>0) phi = TMath::Pi()+atan(-x_pmt[detkey]/z_pmt[detkey]);
        else phi = 0.;
        if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
        phi-=TMath::Pi();
        if (phi < - TMath::Pi()) phi = -TMath::Pi();
        if (phi<-TMath::Pi() || phi>TMath::Pi())  Log("EventDisplay tool: Drawing Event: Phi out of bounds! X= "+std::to_string(x_pmt[detkey])+", y="+std::to_string(y_pmt[detkey])+", z="+std::to_string(z_pmt[detkey]),v_warning,verbose);
        x[0]=0.5+phi*size_top_drawing;
        y[0]=0.5+y_pmt[detkey]/tank_height*tank_height/tank_radius*size_top_drawing;
        TPolyMarker *marker_bulk = new TPolyMarker(1,x,y,"");
        if (mode == "Charge") color_marker = Bird_Idx+int((charge[detkey]-threshold)/(maximum_pmts-threshold)*254);
        else if (mode == "Time" && threshold_time_high == -999 && threshold_time_low == -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(maximum_time_overall-min_time_overall)*254);
	else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high != -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(threshold_time_high-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low != -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(maximum_time_overall-threshold_time_low)*254);
        else if (mode == "Time") color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
        marker_bulk->SetMarkerColor(color_marker);
        marker_bulk->SetMarkerStyle(8);
        marker_bulk->SetMarkerSize(marker_size);
        marker_pmts_wall.push_back(marker_bulk);
      }
    }

    //draw all PMT markers
    for (unsigned int i_marker=0;i_marker<marker_pmts_top.size();i_marker++){
      marker_pmts_top.at(i_marker)->Draw();
    }
    for (unsigned int i_marker=0;i_marker<marker_pmts_bottom.size();i_marker++){
      marker_pmts_bottom.at(i_marker)->Draw();
    }
    for (unsigned int i_marker=0;i_marker<marker_pmts_wall.size();i_marker++){
      marker_pmts_wall.at(i_marker)->Draw();
    }
  }

  void EventDisplay::draw_event_LAPPDs(){

      //draw LAPPD markers on Event Display
      Log("EventDisplay tool: Drawing LAPPD event markers.",v_message,verbose);

      marker_lappds.clear();
      double phi;
      double x[1];
      double y[1];
      double charge_single=1.;        //FIXME when charge is implemented in LoadWCSimLAPPD
      bool draw_lappd_markers=false;
      std::map<unsigned long,std::vector<Position>>::iterator lappd_hit_pos_it = hits_LAPPDs.begin();
      for(auto&& these_lappd_hit_positions : hits_LAPPDs){
        unsigned long detkey = these_lappd_hit_positions.first;
        for (unsigned int i_single_lappd=0; i_single_lappd<these_lappd_hit_positions.second.size(); i_single_lappd++){
          double time_lappd_single = time_lappd.at(detkey).at(i_single_lappd);
          if (charge_single<threshold_lappd || (threshold_time_high!=-999 && time_lappd_single>threshold_time_high) || (threshold_time_low!=-999 && time_lappd_single<threshold_time_low)) continue; 
          draw_lappd_markers=true;
          
          double x_lappd = hits_LAPPDs.at(detkey).at(i_single_lappd).X();
          double y_lappd = hits_LAPPDs.at(detkey).at(i_single_lappd).Y();
          double z_lappd = hits_LAPPDs.at(detkey).at(i_single_lappd).Z();
          if (x_lappd>0 && z_lappd>0) phi = atan(z_lappd/x_lappd)+TMath::Pi()/2;
          else if (x_lappd>0 && z_lappd<0) phi = atan(x_lappd/-z_lappd);
          else if (x_lappd<0 && z_lappd<0) phi = 3*TMath::Pi()/2+atan(z_lappd/x_lappd);
          else if (x_lappd<0 && z_lappd>0) phi = TMath::Pi()+atan(-x_lappd/z_lappd);
          else phi = 0.;
          if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
          phi-=TMath::Pi();
          if (phi < - TMath::Pi()) phi = -TMath::Pi();
          if (phi < - TMath::Pi() || phi>TMath::Pi())  Log("EventDisplay tool: Drawing LAPPD event: ERROR: Phi out of bounds! X= "+std::to_string(x_lappd)+", y="+std::to_string(y_lappd)+", z="+std::to_string(z_lappd),v_warning,verbose);
          x[0]=0.5+phi*size_top_drawing;
          y[0]=0.5+y_lappd/tank_height*tank_height/tank_radius*size_top_drawing;
          TPolyMarker *marker_lappd = new TPolyMarker(1,x,y,"");
          if (mode == "Charge") color_marker = Bird_Idx+254;      //FIXME in case of actual charge information for LAPPDs
          else if (mode == "Time" && threshold_time_high==-999 && threshold_time_low==-999) {
            if (time_lappd_single > maximum_time_overall || time_lappd_single < min_time_overall) Log("EventDisplay tool: Error: LAPPD hit time out of bounds! LAPPD time = "+std::to_string(time_lappd_single)+", min time: "+std::to_string(min_time_overall)+", max time: "+std::to_string(maximum_time_overall),v_error,verbose);
            color_marker = Bird_Idx+int((time_lappd_single-min_time_overall)/(maximum_time_overall-min_time_overall)*254);
          }
          else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high != -999) color_marker = Bird_Idx+int((time_lappd_single-min_time_overall)/(threshold_time_high-min_time_overall)*254);
          else if (mode == "Time" && threshold_time_low != -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time_lappd_single-threshold_time_low)/(maximum_time_overall-threshold_time_low)*254);
          else if (mode == "Time") color_marker = Bird_Idx+int((time_lappd_single-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
          marker_lappd->SetMarkerColor(color_marker);
          marker_lappd->SetMarkerStyle(8);
          marker_lappd->SetMarkerSize(0.4);
          marker_lappds.push_back(marker_lappd);
        }
      }

      //draw all LAPPD markers
      if (draw_lappd_markers){
        for (unsigned int i_draw=0; i_draw<marker_lappds.size();i_draw++){
          marker_lappds.at(i_draw)->Draw();
        }
      }
  }

  void EventDisplay::draw_event_MRD(){

    //draw MRD hits on Event Display

    Log("EventDisplay tool: Drawing MRD hits.",v_message,verbose);
    for (unsigned int i_mrd = 0; i_mrd < hitmrd_detkeys.size(); i_mrd++){
        unsigned long detkey = hitmrd_detkeys[i_mrd];
        int color_marker;
	if (threshold_time_high_mrd == -999 && threshold_time_low_mrd == -999) color_marker = Bird_Idx+int((mrddigittimesthisevent[detkey]-min_time_mrd)/(maximum_time_mrd-min_time_mrd)*254);
        else if (threshold_time_low_mrd == -999 && threshold_time_high_mrd != -999) color_marker = Bird_Idx+int((mrddigittimesthisevent[detkey]-min_time_mrd)/(threshold_time_high_mrd-min_time_mrd)*254);
        else if (threshold_time_low_mrd != -999 && threshold_time_high_mrd == -999) color_marker = Bird_Idx+int((mrddigittimesthisevent[detkey]-threshold_time_low_mrd)/(maximum_time_mrd-threshold_time_low_mrd)*254);
        else color_marker = Bird_Idx+int((mrddigittimesthisevent[detkey]-threshold_time_low_mrd)/(threshold_time_high_mrd-threshold_time_low_mrd)*254);
        mrd_paddles[detkey]->SetFillColor(color_marker);
	}
  }

  void EventDisplay::draw_true_vertex(Position trueVtx, Direction trueDir, int ring_color){

    //draw interaction vertex --> projection on the walls
    

    find_projected_xyz(trueVtx.X(), trueVtx.Y(), trueVtx.Z(), trueDir.X(), trueDir.Y(), trueDir.Z(), vtxproj_x, vtxproj_y, vtxproj_z);

    std::string logmessage = "EventDisplay tool: Projected true vertex on wall: ("+std::to_string(vtxproj_x)+", "+std::to_string(vtxproj_y)+", "+std::to_string(vtxproj_z)+"). Drawing vertex on EventDisplay.";
    Log(logmessage,v_message,verbose);

    double xTemp, yTemp, phiTemp;
    int status_temp;
    translate_xy(vtxproj_x,vtxproj_y,vtxproj_z,xTemp,yTemp, status_temp, phiTemp);
    double x_vtx[1] = {xTemp};
    double y_vtx[1] = {yTemp};

    marker_vtx = new TPolyMarker(1,x_vtx,y_vtx,"");
    marker_vtx->SetMarkerColor(ring_color);
    marker_vtx->SetMarkerStyle(22);
    marker_vtx->SetMarkerSize(1.);
    marker_vtx->Draw(); 

  }


  void EventDisplay::draw_barycenter(Position posBary){

    Direction dirBary(posBary.X(),posBary.Y(),posBary.Z());
    double projBaryX, projBaryY, projBaryZ;
    find_projected_xyz(posBary.X(), posBary.Y(), posBary.Z(),dirBary.X(),dirBary.Y(),dirBary.Z(),projBaryX,projBaryY,projBaryZ);

    double xTemp,yTemp,phiTemp;
    int status_temp;

    translate_xy(projBaryX,projBaryY,projBaryZ,xTemp,yTemp, status_temp, phiTemp);
    double x_vtx[1] = {xTemp};
    double y_vtx[1] = {yTemp};

    marker_bary = new TPolyMarker(1,x_vtx,y_vtx,"");
    marker_bary->SetMarkerColor(3);
    marker_bary->SetMarkerStyle(29);
    marker_bary->SetMarkerSize(2.);
    marker_bary->Draw(); 


  }


  //quaternion structs as help objects for rotating vectors around vectors

  struct quaternion {             //for rotations of vectors around vectors

    double q0, q1, q2, q3;

  };

  quaternion multiplication(quaternion Q1, quaternion Q2){

    quaternion Q_Result;
    Q_Result.q0 = Q1.q0*Q2.q0-(Q1.q1*Q2.q1+Q1.q2*Q2.q2+Q1.q3*Q2.q3);
    Q_Result.q1 = Q1.q0*Q2.q1+Q1.q1*Q2.q0+Q1.q2*Q2.q3-Q1.q3*Q2.q2;
    Q_Result.q2 = Q1.q0*Q2.q2+Q1.q2*Q2.q0+Q1.q3*Q2.q1-Q1.q1*Q2.q3;
    Q_Result.q3 = Q1.q0*Q2.q3+Q1.q3*Q2.q0+Q1.q1*Q2.q2-Q1.q2*Q2.q1;
    return Q_Result;

  }

  quaternion define_rotationVector(double angle, double x, double y, double z){

    quaternion Q_Rotation;
    Q_Rotation.q0 = cos(angle/2.);
    Q_Rotation.q1 = sin(angle/2.)*x;
    Q_Rotation.q2 = sin(angle/2.)*y;
    Q_Rotation.q3 = sin(angle/2.)*z;
    return Q_Rotation;

  }

  quaternion invert_quaternion(quaternion Q1){

    quaternion Q_Invert;
    Q_Invert.q0 = Q1.q0;
    Q_Invert.q1 = -Q1.q1;
    Q_Invert.q2 = -Q1.q2;
    Q_Invert.q3 = -Q1.q3;
    return Q_Invert;

  }

  void EventDisplay::draw_true_ring(Position trueVtx, Direction trueDir, int ring_color){

    //draw the true ring projected from the interaction vertex at the Cherenkov angle w.r.t. true travel direction

    double truevtxX = trueVtx.X();
    double truevtxY = trueVtx.Y();
    double truevtxZ = trueVtx.Z();
    double truedirX = trueDir.X();
    double truedirY = trueDir.Y();
    double truedirZ = trueDir.Z();

    double phi_ring;
    double ringdir_x, ringdir_y, ringdir_z;
    double vtxproj_x2, vtxproj_y2, vtxproj_z2;
    double vtxproj_xtest, vtxproj_ytest, vtxproj_ztest;
    double vtxproj_xtest2, vtxproj_ytest2, vtxproj_ztest2;

    //create vector that has simply a lower z component and is located at the Cherenkov angle to the true direction vector
    double a = 1 - truedirZ*truedirZ/cos(thetaC)/cos(thetaC);
    double b = -2*(truedirX*truedirX+truedirY*truedirY)*truedirZ/cos(thetaC)/cos(thetaC);
    double c = (truedirX*truedirX+truedirY*truedirY)*(1-(truedirX*truedirX+truedirY*truedirY)/cos(thetaC)/cos(thetaC));
    double z_new1 = (-b+sqrt(b*b-4*a*c))/(2*a);
    double z_new2 = (-b-sqrt(b*b-4*a*c))/(2*a);
    double z_new;
    if ((z_new1 < -tank_radius) || (z_new1 > tank_radius)) z_new=z_new2;
    else z_new = z_new1;
    //create vector that has simply a lower y component and is located at the Cherenkov angle to the true direction vector
    double a2 = 1 - truedirY*truedirY/cos(thetaC)/cos(thetaC);
    double b2 = -2*(truedirX*truedirX+truedirZ*truedirZ)*truedirY/cos(thetaC)/cos(thetaC);
    double c2 = (truedirX*truedirX+truedirZ*truedirZ)*(1-(truedirX*truedirX+truedirZ*truedirZ)/cos(thetaC)/cos(thetaC));
    double y_new1 = (-b2+sqrt(b2*b2-4*a2*c2))/(2*a2);
    double y_new2 = (-b2-sqrt(b2*b2-4*a2*c2))/(2*a2);
    double y_new;
    if ((y_new1 < min_y) || (y_new1 > max_y)) y_new=y_new2;
    else y_new = y_new1;
    //create vector that has simply a lower x component and is located at the Cherenkov angle to the true direction vector
    double a3 = 1 - truedirX*truedirX/cos(thetaC)/cos(thetaC);
    double b3 = -2*(truedirZ*truedirZ+truedirY*truedirY)*truedirX/cos(thetaC)/cos(thetaC);
    double c3 = (truedirZ*truedirZ+truedirY*truedirY)*(1-(truedirZ*truedirZ+truedirY*truedirY)/cos(thetaC)/cos(thetaC));
    double x_new1 = (-b3+sqrt(b3*b3-4*a3*c3))/(2*a3);
    double x_new2 = (-b3-sqrt(b3*b3-4*a3*c3))/(2*a3);
    double x_new;
    if ((x_new1 < -tank_radius) || (x_new1 > tank_radius)) x_new=x_new2;
    else x_new = x_new1;

    //decide which axis to rotate around (axis should not be too close to particle direction)
    int status=-1;
    if (fabs(truedirX)<fabs(truedirY)){
      if (fabs(truedirZ)<fabs(truedirX)){
        status=3;
        find_projected_xyz(truevtxX, truevtxY, truevtxZ, truedirX, truedirY, z_new, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }
      else {
        status = 1;
        find_projected_xyz(truevtxX, truevtxY, truevtxZ, x_new, truedirY, truedirZ, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }
    }else {
      if (fabs(truedirZ)<fabs(truedirY)){
          status = 3;
          find_projected_xyz(truevtxX, truevtxY, truevtxZ, truedirX, truedirY, z_new, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }else{status = 2;
        find_projected_xyz(truevtxX, truevtxY, truevtxZ, truedirX, y_new, truedirZ, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }
    }

    if (verbose > v_debug){
      std::cout <<"Rotated direction: ( "<<truedirX<<", "<<truedirY<<", "<<z_new<<" )"<<std::endl;
      std::cout <<"Angle (rotated direction - original direction) : "<<acos((truedirX*truedirX+truedirY*truedirY+truedirZ*z_new)/(sqrt(truedirX*truedirX+truedirY*truedirY+z_new*z_new)))*180./TMath::Pi()<<std::endl;
      std::cout <<"Rotated direction, projection in z: ( "<<vtxproj_x2<< " , "<<vtxproj_y2<<" , "<<vtxproj_z2<< " )"<<std::endl;
      std::cout <<"Drawing first point of Cherenkov ring .... "<<std::endl;
    }

    int status_temp;
    double xTemp, yTemp, phiTemp;
    translate_xy(vtxproj_x2,vtxproj_y2,vtxproj_z2,xTemp,yTemp,status_temp, phiTemp);
    double x_vtx[1] = {xTemp};
    double y_vtx[1] = {yTemp};

    //debug drawing --> where would rotation in other direction end up?
    /*

    TPolyMarker *marker_vtx = new TPolyMarker(1,x_vtx,y_vtx,"");
    marker_vtx->SetMarkerColor(2);
    marker_vtx->SetMarkerStyle(20);
    marker_vtx->SetMarkerSize(1.);
    marker_vtx->Draw(); 
    find_projected_xyz(truevtxX, truevtxY, truevtxZ, truedirX, y_new, truedirZ, vtxproj_xtest, vtxproj_ytest, vtxproj_ztest);
    translate_xy(vtxproj_xtest,vtxproj_ytest,vtxproj_ztest,xTemp,yTemp,status_temp,phiTemp);
    double x_test_vtx[1] = {xTemp};
    double y_test_vtx[1] = {yTemp};
    TPolyMarker *marker_test_vtx = new TPolyMarker(1,x_test_vtx,y_test_vtx,"");
    marker_test_vtx->SetMarkerColor(2);
    marker_test_vtx->SetMarkerStyle(21);
    marker_test_vtx->SetMarkerSize(1.);
    marker_test_vtx->Draw(); 
    find_projected_xyz(truevtxX, truevtxY, truevtxZ, x_new, truedirY, truedirZ, vtxproj_xtest2, vtxproj_ytest2, vtxproj_ztest2);
    translate_xy(vtxproj_xtest2,vtxproj_ytest2,vtxproj_ztest2,xTemp,yTemp,status_temp,phiTemp);
    double x_test2_vtx[1] = {xTemp};
    double y_test2_vtx[1] = {yTemp};
    TPolyMarker *marker_test2_vtx = new TPolyMarker(1,x_test2_vtx,y_test2_vtx,"");
    marker_test2_vtx->SetMarkerColor(2);
    marker_test2_vtx->SetMarkerStyle(29);
    marker_test2_vtx->SetMarkerSize(1.);
    marker_test2_vtx->Draw(); 
    */

    quaternion Q_RingStart;
    Q_RingStart.q0 = 0.;

    if (status == 1){
      Q_RingStart.q1 = x_new;
      Q_RingStart.q2 = truedirY;
      Q_RingStart.q3 = truedirZ;
    }else if (status == 2){
      Q_RingStart.q1 = truedirX;
      Q_RingStart.q2 = y_new;
      Q_RingStart.q3 = truedirZ;
    }else if (status == 3){
      Q_RingStart.q1 = truedirX;
      Q_RingStart.q2 = truedirY;
      Q_RingStart.q3 = z_new;
    }else {
      //if status variable was not filled for some reason, use rotation around z as best guess
      Q_RingStart.q1 = truedirX;
      Q_RingStart.q2 = truedirY;
      Q_RingStart.q3 = z_new;
    }

    //introduce status variables to keep track of the number of sub-areas the ring encounters (transitions wall - top / bottom)
    int status_pre = -1;
    int status_ev = 0;
    double phi_pre = 0.;
    double phi_post = 0.;
    double phi_calc = 0.;
    int status_phi_deriv;       //derivative: does phi get smaller or bigger?
    int i_ring_area=0;
    int i_angle=0;
    int i_valid_points=0;
    int points_area[10]={0};
    int firstev_area[10];

    for (int i_ring=0;i_ring<num_ring_points;i_ring++){
      phi_ring = i_ring*2*TMath::Pi()/double(num_ring_points);
      quaternion Q_RotAxis = define_rotationVector(phi_ring, truedirX, truedirY, truedirZ);
      quaternion Q_RotAxis_Inverted = invert_quaternion(Q_RotAxis);
      quaternion Q_Intermediate = multiplication(Q_RotAxis, Q_RingStart);
      quaternion Q_Rotated = multiplication(Q_Intermediate, Q_RotAxis_Inverted);
      double dir_sum = sqrt(Q_Rotated.q1*Q_Rotated.q1+Q_Rotated.q2*Q_Rotated.q2+Q_Rotated.q3*Q_Rotated.q3);
      find_projected_xyz(truevtxX, truevtxY, truevtxZ, Q_Rotated.q1/dir_sum , Q_Rotated.q2/dir_sum , Q_Rotated.q3/dir_sum , vtxproj_x2, vtxproj_y2, vtxproj_z2);
      Log("EventDisplay tool: Drawing Ring: Ring point # "+std::to_string(i_ring)+", projected x/y/z = "+std::to_string(vtxproj_x2)+"/"+std::to_string(vtxproj_y2)+"/"+std::to_string(vtxproj_z2),v_message,verbose);
      translate_xy(vtxproj_x2,vtxproj_y2,vtxproj_z2,xTemp,yTemp, status_ev, phi_calc);

      if (i_valid_points == 0 ){
        status_pre = status_ev;
        firstev_area[0] = 0;
      }
      else if (status_pre != status_ev){
        status_pre = status_ev;
        i_ring_area++;
        firstev_area[i_ring_area] = i_valid_points;
      }
      if (status_ev == 4) {     //don't calculate (x,y) pair in case projected vertex is out of bounds
        continue;
      }

      if (status_pre == status_ev){
          if (status_ev == 3) {       //for wall part need to take care of boundary events
            if (i_angle == 0) {
              phi_pre = phi_calc;
              i_angle++;
            } else {
              phi_post = phi_calc;
              Log("EventDisplay tool: Drawing Ring: Phi pre = "+std::to_string(phi_pre)+", phi post = "+std::to_string(phi_post),v_message,verbose);
              if (fabs(phi_post-phi_pre) > TMath::Pi()/2.) {
                i_ring_area++;
                firstev_area[i_ring_area] = i_valid_points;
              }  
              Log("EventDisplay tool: Drawing Ring: Ring area # "+std::to_string(i_ring_area),v_message,verbose);
              phi_pre = phi_post;
            }
          }
          points_area[i_ring_area]++;  
          xring[i_ring_area][i_valid_points-firstev_area[i_ring_area]] = xTemp;
          yring[i_ring_area][i_valid_points-firstev_area[i_ring_area]] = yTemp;
          i_valid_points++;
      }else {
        Log("EventDisplay tool: Drawing Ring: Ring area # "+std::to_string(i_ring_area),v_message,verbose);
      }
    }

    for (int i_area = 0; i_area<= i_ring_area;i_area++){
        ring_visual[i_area] = new TPolyLine(points_area[i_area],xring[i_area],yring[i_area]);
        ring_visual[i_area]->SetLineColor(ring_color);
        ring_visual[i_area]->SetLineWidth(2);
        ring_visual[i_area]->Draw();
    }

    current_n_polylines = i_ring_area+1;

  }

  void EventDisplay::find_projected_xyz(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double &projected_x, double &projected_y, double &projected_z){

    //find projection of particle vertex onto the cylindrical ANNIE detector surface, given a particle direction

    double time_top, time_wall, time_wall1, time_wall2, time_min;
    double a,b,c;         //for calculation of time_wall
    time_top = (dirY > 0)? (max_y-vtxY)/dirY : (min_y - vtxY)/dirY;
    Log("EventDisplay tool: Find projected xyz: dirY = "+std::to_string(dirY)+", max_y = "+std::to_string(max_y)+", min_y = "+std::to_string(min_y)+", vtxY = "+std::to_string(vtxY)+", vtxX = "+std::to_string(vtxX)+", dirX = "+std::to_string(dirX)+", vtxZ = "+std::to_string(vtxZ)+",dirZ = "+std::to_string(dirZ),v_debug,verbose);
    if (time_top < 0) time_top = 999.;    //rule out unphysical times (negative)
    a = dirX*dirX + dirZ*dirZ;
    b = 2*vtxX*dirX + 2*vtxZ*dirZ;
    c = vtxX*vtxX+vtxZ*vtxZ-tank_radius*tank_radius;
    Log("EventDisplay tool: Find projected xyz: a= "+std::to_string(a)+", b= "+std::to_string(b)+", c= "+std::to_string(c),v_debug,verbose);
    time_wall1 = (-b+sqrt(b*b-4*a*c))/(2*a);
    time_wall2 = (-b-sqrt(b*b-4*a*c))/(2*a);
    if (time_wall2>=0 && time_wall1>=0) time_wall = (time_wall1<time_wall2) ? time_wall1 : time_wall2;
    else if (time_wall2 < 0 ) time_wall = time_wall1;
    else if (time_wall1 < 0 ) time_wall = time_wall2;
    else time_wall = 0;
    if ((vtxY+dirY*time_wall > max_y || vtxY+dirY*time_wall < min_y)) time_wall = 999;
    time_min = (time_wall < time_top && time_wall >=0 )? time_wall : time_top;

    Log("EventDisplay tool: Find projected xyz: Time_min: "+std::to_string(time_min)+", time wall1: "+std::to_string(time_wall1)+", time wall2: "+std::to_string(time_wall2)+", time wall: "+std::to_string(time_wall)+", time top: "+std::to_string(time_top),v_debug,verbose);

    projected_x = vtxX+dirX*time_min;
    projected_y = vtxY+dirY*time_min;
    projected_z = vtxZ+dirZ*time_min;

  }

  void EventDisplay::translate_xy(double vtxX, double vtxY, double vtxZ, double &xWall, double &yWall, int &status_hit, double &phi_calc){

    //translate 3D position in the ANNIE coordinate frame into 2D xy positions in the cylindrical detection plane

    if (fabs(vtxY-max_y)<0.01){            //draw vtx projection on the top of tank
      xWall=0.5-size_top_drawing*vtxX/tank_radius;
      yWall=0.5+(max_y/tank_radius+1)*size_top_drawing-size_top_drawing*vtxZ/tank_radius;
      status_hit = 1;
      phi_calc = -999;
      if (sqrt(vtxX*vtxX+vtxZ*vtxZ)>tank_radius) status_hit = 4;
    } else if (fabs(vtxY-min_y)<0.01){            //draw vtx projection on the top of tank
      xWall=0.5-size_top_drawing*vtxX/tank_radius;
      yWall=0.5-(fabs(min_y)/tank_radius+1)*size_top_drawing+size_top_drawing*vtxZ/tank_radius;
      status_hit = 2;
      phi_calc = -999;
      if (sqrt(vtxX*vtxX+vtxZ*vtxZ)>tank_radius) status_hit = 4;
    }else {
      double phi;
      if (vtxX>0 && vtxZ>0) phi = atan(vtxZ/vtxX)+TMath::Pi()/2;
      else if (vtxX>0 && vtxZ<0) phi = atan(vtxX/-vtxZ);
      else if (vtxX<0 && vtxZ<0) phi = 3*TMath::Pi()/2+atan(vtxZ/vtxX);
      else if (vtxX<0 && vtxZ>0) phi = TMath::Pi()+atan(-vtxX/vtxZ);
      else phi = TMath::Pi();
      if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
      phi-=TMath::Pi();
      if (phi < - TMath::Pi()) phi = -TMath::Pi();
      xWall=0.5+phi*size_top_drawing;
      yWall=0.5+vtxY/tank_height*tank_height/tank_radius*size_top_drawing;
      status_hit = 3;
      phi_calc = phi;
      if (vtxY>max_y && vtxY<min_y) status_hit = 4;
    }

  }


  void EventDisplay::delete_canvas_contents(){

    //delete all objects that were drawn on the canvas so far

    Log("EventDisplay tool: Delete canvas contents.",v_message,verbose);
    if (text_event_info) delete text_event_info;
    if (pmt_title) delete pmt_title;
    if (lappd_title) delete lappd_title;
    if (mrd_title) delete mrd_title;
    if (schematic_tank) delete schematic_tank;
    if (schematic_facc) delete schematic_facc;
    if (schematic_mrd) delete schematic_mrd;
    if (schematic_tank_label) delete schematic_tank_label;
    if (schematic_facc_label) delete schematic_facc_label;
    if (schematic_mrd_label) delete schematic_mrd_label;
    if (border_schematic_mrd) delete border_schematic_mrd;
    if (border_schematic_facc) delete border_schematic_facc;
    if (top_circle) delete top_circle;
    if (bottom_circle) delete bottom_circle;
    if (box) delete box;
    if (legend_mu) delete legend_mu;
    if (legend_e) delete legend_e;
    if (legend_pi) delete legend_pi;
    if (legend_pi0) delete legend_pi0;
    if (legend_k) delete legend_k;
    if (title_mrd_side) delete title_mrd_side;
    if (title_mrd_top) delete title_mrd_top;

    if (max_text) delete max_text;
    if (min_text) delete min_text;
    if (max_lappd) delete max_lappd;
    if (min_lappd) delete min_lappd;
    if (min_mrd) delete min_mrd;
    if (max_mrd) delete max_mrd;

    for (unsigned int i_color=0; i_color < vector_colordot.size();i_color++){
      delete vector_colordot.at(i_color);
    }    
    for (unsigned int i_color=0; i_color < vector_colordot_lappd.size();i_color++){
      delete vector_colordot_lappd.at(i_color);
    }
    for (unsigned int i_color=0; i_color < vector_colordot_mrd.size();i_color++){
      delete vector_colordot_mrd.at(i_color);
    }
    vector_colordot.clear();
    vector_colordot_lappd.clear();
    vector_colordot_mrd.clear();

    for (unsigned int i_mrd = 0; i_mrd < mrd_detkeys.size(); i_mrd++){
      unsigned long detkey = mrd_detkeys.at(i_mrd);
      delete mrd_paddles[detkey];
      delete box_mrd_paddles[detkey];
    }
    mrd_paddles.clear();
    box_mrd_paddles.clear();

    //markers, lines are automatically deleted when calling TCanvas::Clear
    //calling delete again would hence cause a segmentation fault

    Log("EventDisplay tool: Current number of polylines (to be deleted): "+std::to_string(current_n_polylines),v_message,verbose);
    for (int i_ring=0; i_ring < current_n_polylines; i_ring++){
      if (ring_visual[i_ring]) delete ring_visual[i_ring];
    }


  }

  void EventDisplay::draw_schematic_detector(){

    //draw the schematic subdetectors on the top right of the picture

    //facc
    schematic_facc = new TBox(0.73,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.74,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    schematic_facc_label = new TText(0.72,0.73,"FMV");
    schematic_facc_label->SetTextSize(0.02);
    if (facc_hit) schematic_facc->SetFillColor(kOrange+1);
    else schematic_facc->SetFillColor(0);
    schematic_facc->Draw();
    schematic_facc_label->Draw();
    border_schematic_facc= new TBox(0.73,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.74,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    border_schematic_facc->SetFillStyle(0);
    border_schematic_facc->SetLineColor(1);
    border_schematic_facc->SetLineWidth(1);
    border_schematic_facc->Draw();

    //tank
    schematic_tank = new TEllipse(0.80,0.75+0.5*tank_height/tank_radius*size_top_drawing,size_schematic_drawing,size_schematic_drawing);
    schematic_tank_label = new TText(0.78,0.73,"Tank");
    schematic_tank_label->SetTextSize(0.02);
    if (tank_hit) schematic_tank->SetFillColor(kOrange+1);
    else schematic_tank->SetFillColor(0);
    schematic_tank->SetLineColor(1);
    schematic_tank->SetLineWidth(1);
    schematic_tank->Draw();
    schematic_tank_label->Draw();

    //mrd
    schematic_mrd = new TBox(0.85,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.85+1.6/2.7*2*size_schematic_drawing,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    schematic_mrd_label = new TText(0.86,0.73,"MRD");
    schematic_mrd_label->SetTextSize(0.02);
    if (mrd_hit) schematic_mrd->SetFillColor(kOrange+1);
    else schematic_mrd->SetFillColor(0);
    schematic_mrd->Draw();
    schematic_mrd_label->Draw();
    border_schematic_mrd = new TBox(0.85,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.85+1.6/2.7*2*size_schematic_drawing,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    border_schematic_mrd->SetFillStyle(0);
    border_schematic_mrd->SetLineColor(1);
    border_schematic_mrd->SetLineWidth(1);
    border_schematic_mrd->Draw();

}

void EventDisplay::draw_particle_legend(){

    legend_mu = new TLatex(0.73,0.9,"#mu^{+/-}");
    legend_mu->SetTextSize(0.03);
    legend_mu->SetTextColor(2);
    legend_mu->Draw();

    legend_e = new TLatex(0.77,0.9,"e^{+/-}");
    legend_e->SetTextSize(0.03);
    legend_e->SetTextColor(4);
    legend_e->Draw();

    legend_pi = new TLatex(0.81,0.9,"#pi^{+/-}");
    legend_pi->SetTextSize(0.03);
    legend_pi->SetTextColor(8);
    legend_pi->Draw();
    
    legend_pi0 = new TLatex(0.85,0.9,"#pi^{0}");
    legend_pi0->SetTextSize(0.03);
    legend_pi0->SetTextColor(1);
    legend_pi0->Draw();
    
    legend_k = new TLatex(0.89,0.9,"K^{+/-}");
    legend_k->SetTextSize(0.03);
    legend_k->SetTextColor(9);
    legend_k->Draw();
}

void EventDisplay::set_color_palette(){

    //calculate the numbers of the color palette

    Bird_Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
    for (int i_color=0;i_color<n_colors;i_color++){
      Bird_palette[i_color]=Bird_Idx+i_color;
    }

    //Set color scheme for rings depending on pdg code
    pdg_to_color.emplace(13,2);                 // MUONS: Red
    pdg_to_color.emplace(-13,2);
    pdg_to_color.emplace(11,4);                 // ELECTRONS: Blue
    pdg_to_color.emplace(-11,3);
    pdg_to_color.emplace(111,1);                // Pi0: Black
    pdg_to_color.emplace(211,8);                // Pip, Pim: Green
    pdg_to_color.emplace(-211,8);
    pdg_to_color.emplace(321,9);                // Kp, Km: Purple
    pdg_to_color.emplace(-321,9);
}

void EventDisplay::ParseUserInput(std::string user_string){

  int new_evnum;
  char firstChar = user_string.front();
  if (firstChar == 'P') {
    if (!isData) new_evnum = mcevnum - 1;
    else new_evnum = evnum - 1;
    if (new_evnum < 0) {
      Log("EventDisplay tool: Already at the first event! Cannot select previous event",v_error,verbose);
      m_data->CStore.Set("UserEvent",false);
    } else {
      Log("EventDisplay tool: Going to event nr "+std::to_string(new_evnum),v_message,verbose);
      m_data->CStore.Set("UserEvent",true);
      m_data->CStore.Set("LoadEvNr",new_evnum);
      m_data->CStore.Set("CheckFurtherTriggers",false);
    }
  }
  else if (firstChar == 'N') {
    if (!isData) new_evnum = mcevnum + 1;        //In this case we do not know if we already reached the last event, needs to be checked by LoadWCSim
    else new_evnum = evnum + 1;
    Log("EventDisplay tool: Going to event nr "+std::to_string(new_evnum),v_message,verbose);
    m_data->CStore.Set("UserEvent",true);
    m_data->CStore.Set("LoadEvNr",new_evnum);
    if (!isData){
      m_data->CStore.Set("CheckFurtherTriggers",true);
      m_data->CStore.Set("CurrentTriggernum",MCTriggernum); 
    }
 }
  else if (firstChar == 'E') {
    std::string str_evnum = user_string.substr(user_string.find("E") + 1);
    new_evnum = std::stoi(str_evnum,nullptr,10);
    Log("EventDisplay tool: Conversion "+user_string+"--> Event Number "+std::to_string(new_evnum),v_debug,verbose);
    Log("EventDisplay tool: Going to event nr "+std::to_string(new_evnum),v_message,verbose);
    m_data->CStore.Set("UserEvent",true);
    m_data->CStore.Set("LoadEvNr",new_evnum);
    m_data->CStore.Set("CheckFurtherTriggers",false);
  }
  else if (firstChar == 'Q') {
    Log("EventDisplay tool: Quitting EventDisplay toolchain.",v_message,verbose);
    m_data->CStore.Set("UserEvent",false);
    m_data->vars.Set("StopLoop",1);
  }
  else {
    Log("EventDisplay tool: User Input "+user_string+" not recognized. Please make sure you specified everything in the right format.",v_error,verbose);
    Log("EventDisplay tool: Getting next event instead",v_error,verbose);
    m_data->CStore.Set("UserEvent",false);
  }

  return;

}

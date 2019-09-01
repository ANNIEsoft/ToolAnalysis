#include "EventDisplay.h"

EventDisplay::EventDisplay():Tool(){}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-------------------------   INIT/ EXEC / FINAL ------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------


bool EventDisplay::Initialise(std::string configfile, DataModel &data){

  if (verbose > 0) std::cout <<"Initialising tool: EventDisplay"<<std::endl;

  //only for debugging memory leaks, otherwise comment out
  /*std::cout <<"List of Objects (beginning of initialise): "<<std::endl;
  gObjectTable->Print();*/

  //--------------------------------------------
  //---------loading config file----------------
  //--------------------------------------------

  if(configfile!="")  m_variables.Initialise(configfile); 
  m_data= &data;       //assigning transient data pointer

  //--------------------------------------------
  //-------get configuration variables----------
  //--------------------------------------------

  m_variables.Get("verbose",verbose);
  m_variables.Get("Event",single_event);
  m_variables.Get("EventList",str_event_list);
  m_variables.Get("Mode",mode);
  m_variables.Get("Format",format);
  m_variables.Get("TextBox",text_box);
  m_variables.Get("Threshold_Charge",threshold);
  m_variables.Get("Threshold_ChargeLAPPD",threshold_lappd);
  m_variables.Get("Threshold_TimeLow",threshold_time_low);
  m_variables.Get("Threshold_TimeHigh",threshold_time_high);
  m_variables.Get("LAPPDsSelected",lappds_selected);
  m_variables.Get("LAPPDsFile",lappds_file);
  m_variables.Get("DrawRing",draw_ring);
  m_variables.Get("DrawVertex",draw_vertex);
  m_variables.Get("SavePlots",save_plots);
  m_variables.Get("HistogramPlots",draw_histograms);
  m_variables.Get("UserInput",user_input);    //manually decide when to go to next event
  m_variables.Get("Graphics",use_tapplication);
  m_variables.Get("OutputFile",out_file);
  m_variables.Get("DetectorConfiguration",detector_config);

  //---------------------------------------------------
  //check for non-sense input  / set default behavior--
  //---------------------------------------------------

  if (mode !="Charge" && mode!="Time") mode = "Charge";
  if (format!="Simulation" && format!="Reco") format = "Simulation";
  if (text_box!=true && text_box!=false) text_box=true;
  if (lappds_selected!=0 && lappds_selected!=1) lappds_selected=0;
  if (draw_vertex!=0 && draw_vertex!=1) draw_vertex = 0;
  if (draw_ring!=0 && draw_ring!=1) draw_ring = 0;
  if (save_plots!=0 && save_plots!=1) save_plots=1;
  if (draw_histograms!=0 && draw_histograms !=1) draw_histograms=1;
  if (user_input!=0 && user_input!=1) user_input = 0;
  if (use_tapplication!= 0 && use_tapplication!= 1) use_tapplication = 0;

  //---------------------------------------------------
  //-------get ev list in case it's specified----------
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

  if (verbose > 0) std::cout <<"Event List size: "<<ev_list.size()<<std::endl;

  //---------------------------------------------------
  // ------print chosen settings for EventDisplay------
  //---------------------------------------------------

  std::cout <<"//////////////////////////////////////////////////////////////////////"<<std::endl;
  std::cout <<"----------------------------EVENT DISPLAY ---------------------------"<<std::endl;
  std::cout <<"---------------------- CONFIGURATION PARAMETERS ---------------------"<<std::endl;
  std::cout <<"---------------------------------------------------------------------"<<std::endl;
  std::cout <<"Event Number:"<< single_event<<std::endl;
  std::cout <<"Event List: "<< str_event_list<<std::endl;
  std::cout <<"Display Mode: "<< mode<<std::endl;
  std::cout <<"Data Format: "<<format<<std::endl;
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

  //----------------------------------------------------
  //--only evaluate active LAPPD file if told to do so--
  //----------------------------------------------------

  if (lappds_selected){ 

    std::string prefix = "supplementary/";
    std::string filename_lappd = prefix+lappds_file;
    double temp_lappd;
    ifstream file_lappd(filename_lappd);
    while (!file_lappd.eof()){
      file_lappd>>temp_lappd;
      active_lappds_user.emplace(temp_lappd,1);
    }
    file_lappd.close();
  }

  //----------------------------------------------------
  //---------get geometry of detector-------------------
  //----------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

  tank_radius = geom->GetTankRadius();
  tank_height = geom->GetTankHalfheight();
  tank_height/=2;
  detector_version = geom->GetVersion();
  std::cout <<"Detector version: "<<detector_version<<std::endl;
  double barrel_compression=0.82;
  if (detector_config == "ANNIEp2v6") tank_height*=barrel_compression;            //use compressed barrel radius for ANNIEp2v6 detector configuration
  if (verbose > 2) std::cout <<"tank radius before adjusting: "<<tank_radius<<std::endl;
  if (tank_radius<1.0)  tank_radius=1.37504;              //set tank radius to the standard value of old anniev2 configuration(v4/v6 seems to have a very different radius?)
  if (verbose > 2){ 
    std::cout <<"Tank radius: "<<tank_radius<<std::endl;
    std::cout <<"tank half height: "<<tank_height<<std::endl;
  }

  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  n_lappds = geom->GetNumDetectorsInSet("LAPPD");
  n_mrd_pmts = geom->GetNumDetectorsInSet("MRD");
  n_veto_pmts = geom->GetNumDetectorsInSet("Veto");
  m_data->CStore.Get("lappdid_to_detectorkey",lappdid_to_detectorkey);
  m_data->CStore.Get("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);
  m_data->CStore.Get("channelkey_to_faccpmtid",channelkey_to_faccpmtid);
  std::cout <<"EventDisplay: Num Tank PMTs: "<<n_tank_pmts<<", num MRD PMTs: "<<n_mrd_pmts<<", num Veto PMTs: "<<n_veto_pmts<<", num LAPPDs: "<<n_lappds<<std::endl;
  std::map<std::string,std::map<unsigned long,Detector*> >* Detectors = geom->GetDetectors();
  std::cout <<"Detectors size: "<<Detectors->size()<<std::endl;
  
  //----------------------------------------------------------------------
  //-----------read in the LAPPD detkeys into a vector--------------------
  //----------------------------------------------------------------------

  for (std::map<unsigned long,Detector*>::iterator it = Detectors->at("LAPPD").begin();
							it != Detectors->at("LAPPD").end();
							++it){
	Detector *alappd = it->second;
	std::cout <<"LAPPD detector key: "<<alappd->GetDetectorID()<<std::endl;
	lappd_detkeys.push_back(alappd->GetDetectorID());
   }
   max_num_lappds = lappd_detkeys.size();
   std::cout <<"Number of LAPPDs: "<<max_num_lappds<<std::endl;  


  //----------------------------------------------------------------------
  //-----------convert user's provided LAPPD indexes to detectorkey-------
  //----------------------------------------------------------------------
  if(lappds_selected){
    
    for(auto&& auserid : active_lappds_user){
      if(lappdid_to_detectorkey.count(auserid.first)==0){
        std::cerr<<"No detectorkey found for user-provided LAPPDID "<<auserid.first<<"!"<<std::endl;
      } else {
        active_lappds.emplace(lappdid_to_detectorkey.at(auserid.first),1);
      }
    }
  }

  //----------------------------------------------------------------------
  //-----------read in PMT x/y/z positions into vectors-------------------
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
    if (verbose > 2) std::cout <<"detkey: "<<detkey<<std::endl;
    if (verbose > 2) std::cout <<"chankey: "<<chankey<<std::endl;
    x_pmt.insert(std::pair<int,double>(detkey,position_PMT.X()-tank_center_x));
    y_pmt.insert(std::pair<int,double>(detkey,position_PMT.Y()-tank_center_y));
    z_pmt.insert(std::pair<int,double>(detkey,position_PMT.Z()-tank_center_z));
    if (verbose > 2) std::cout <<"Detector ID: "<<detkey<<", position: ("<<position_PMT.X()<<","<<position_PMT.Y()<<","<<position_PMT.Z()<<")"<<std::endl;
    if (verbose > 2) std::cout <<"rho PMT "<<detkey<<": "<<sqrt(x_pmt.at(detkey)*x_pmt.at(detkey)+z_pmt.at(detkey)*z_pmt.at(detkey))<<std::endl;
    if (verbose > 2) std::cout <<"y PMT: "<<y_pmt.at(detkey)<<std::endl;
    if (y_pmt[detkey]>max_y) max_y = y_pmt.at(detkey);
    if (y_pmt[detkey]<min_y) min_y = y_pmt.at(detkey);
  }

  if (verbose > 0) std::cout <<"Properties of the detector configuration: "<<n_veto_pmts<<" veto PMTs, "<<n_mrd_pmts<<" MRD PMTs, "<<n_tank_pmts<<" Tank PMTs, and "<<n_lappds<<" LAPPDs."<<std::endl;
  if (verbose > 0) std::cout <<"Max y of Tank PMTs: "<<max_y<<", min y of tank PMTs: "<<min_y<<std::endl;

  //---------------------------------------------------------------
  //----initialize TApplication in case of displayed graphics------
  //---------------------------------------------------------------

  if (use_tapplication){
    if (verbose > 0) std::cout <<"initialize TApplication"<<std::endl;
    int myargc = 0;
    char *myargv[] {(char*) "options"};
    app_event_display = new TApplication("app_event_display",&myargc,myargv);
    if (verbose > 0) std::cout <<"TApplication running."<<std::endl;
  }

  //---------------------------------------------------------------
  //---------------set the color palette once----------------------
  //---------------------------------------------------------------
  set_color_palette();

  //---------------------------------------------------------------
  //---------------initialize canvases ----------------------------
  //---------------------------------------------------------------

  if (draw_histograms){
    canvas_pmt = new TCanvas("canvas_pmt","Tank PMT histograms",900,600);
    canvas_pmt_supplementary = new TCanvas("canvas_pmt_supplementary","PMT combined",900,600);
    canvas_lappd = new TCanvas("canvas_lappd","LAPPD histograms",900,600);
  }
  canvas_ev_display=new TCanvas("canvas_ev_display","Event Display",900,900);

  for (int i_lappd=0; i_lappd < max_num_lappds; i_lappd++){
    time_LAPPDs[i_lappd] = nullptr;
    charge_LAPPDs[i_lappd] = nullptr;
  }

  return true;
}


bool EventDisplay::Execute(){

  if (verbose > 0) std::cout <<"Executing EventDisplay..."<<std::endl;

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl; /*return false;*/};

  //---------------------------------------------------------------
  //---------------Get ANNIEEvent objects -------------------------
  //---------------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Get("MCParticles",mcparticles); //needed to retrieve true vertex and direction
  m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",runnumber);
  m_data->Stores["ANNIEEvent"]->Get("SubRunNumber",subrunnumber);
  m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<ChannelKey,vector<TDCHit>>
  m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",MCLAPPDHits);
  m_data->Stores["ANNIEEvent"]->Get("EventTime",EventTime);
  m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);
  m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);

  for (int i_trigger=0;i_trigger<TriggerData->size();i_trigger++){
    if (verbose > 0) std::cout<<"Trigger Name: "<<TriggerData->at(i_trigger).GetName()<<std::endl;
    if (verbose > 0) std::cout <<"Trigger Time: "<<std::to_string(TriggerData->at(i_trigger).GetTime().GetNs())<<std::endl;
    trigger_label = TriggerData->at(i_trigger).GetName();
  }

  //---------------------------------------------------------------
  //---------------Current event to be processed? -----------------
  //---------------------------------------------------------------

  if (verbose > 2){
    std::cout <<"before evnum check..."<<std::endl;
    std::cout <<"Single_event: "<<single_event<<std::endl;
    std::cout <<"evnum: "<<evnum<<std::endl;
  }
  if (str_event_list=="None" && (evnum!=single_event && single_event!=-999)) return true;
  if (str_event_list!="None" && !(std::find(ev_list.begin(),ev_list.end(),evnum) != ev_list.end())) return true;      //if continuous mode not enabled, need specific ev number to proceed
  if (verbose > 2) std::cout <<"Evnum check passed...."<<std::endl;

  //----------------------------------------------------------------------
  //-------------Initialise status variables for event -------------------
  //----------------------------------------------------------------------

  facc_hit=false;
  tank_hit=false;
  mrd_hit=false;
  draw_vertex_temp = false;
  draw_ring_temp = false;

  //---------------------------------------------------------------
  //---------------Setup strings for this event -------------------
  //---------------------------------------------------------------

  std::string str_runnumber = std::to_string(runnumber);
  std::string str_evnumber = std::to_string(evnum);
  std::string str_thr = std::to_string(int(threshold));
  std::string str_thrLAPPD = std::to_string(int(threshold_lappd));
  std::string str_time_lower = std::to_string(int(threshold_time_low));
  std::string str_time_higher = std::to_string(int(threshold_time_high));

  std::string underscore = "_";
  std::string str_evdisplay = "_event_display";
  std::string str_lappds = "_lappds";
  std::string str_pmts = "_pmts";
  std::string str_pmts_supp = "_pmts_supp";
  std::string str_png = ".png";
  std::string str_Run="Run";
  std::string str_Ev = "_Ev";
  std::string str_Thr = "_ChargeThr";
  std::string str_ThrLAPPD = "_ThrLAPPD";
  std::string str_time = "_Time_";

  std::string filename_evdisplay = out_file+underscore+str_Run+str_runnumber+str_Ev+str_evnumber+underscore+mode+str_Thr+str_thr+str_ThrLAPPD+str_thrLAPPD+str_time+str_time_lower+underscore+str_time_higher+str_evdisplay+str_png;
  std::string filename_pmts = out_file+underscore+str_Run+str_runnumber+str_Ev+str_evnumber+underscore+mode+str_Thr+str_thr+str_ThrLAPPD+str_thrLAPPD+str_time+str_time_lower+underscore+str_time_higher+str_pmts+str_png;
  std::string filename_pmts_supplementary = out_file+underscore+str_Run+str_runnumber+str_Ev+str_evnumber+underscore+mode+str_Thr+str_thr+str_ThrLAPPD+str_thrLAPPD+str_time+str_time_lower+underscore+str_time_higher+str_pmts_supp+str_png;
  std::string filename_lappds = out_file+underscore+str_Run+str_runnumber+str_Ev+str_evnumber+underscore+mode+str_Thr+str_thr+str_ThrLAPPD+str_thrLAPPD+str_time+str_time_lower+underscore+str_time_higher+str_lappds+str_png;

  //---------------------------------------------------------------
  //-------------clear charge & time containers -------------------
  //---------------------------------------------------------------

  mrddigittimesthisevent.clear();
  mrddigitpmtsthisevent.clear();
  mrddigitchargesthisevent.clear();

  charge.clear();
  time.clear();
  hitpmt_detkeys.clear();

  time_lappd.clear();
  hits_LAPPDs.clear();
  charge_lappd.clear();
  
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

  charge_PMTs = new TH1F("charge_PMTs","Charge of PMTs",100,0,100);
  charge_PMTs->GetXaxis()->SetTitle("charge [p.e.]");
  time_PMTs = new TH1F("time_PMTs","PMTs time response",50,0,50);
  time_PMTs->GetXaxis()->SetTitle("time [ns]");
  charge_time_PMTs = new TH2F("charge_time_PMTs","Charge vs. time PMTs",50,0,50,100,0,100);
  charge_time_PMTs->GetXaxis()->SetTitle("time [ns]");
  charge_time_PMTs->GetYaxis()->SetTitle("charge [p.e.]");

  for (int i=0;i<max_num_lappds;i++){
    std::string str_time_lappds = "time_lappds_det";
    std::string str_charge_lappds = "charge_lappds_det";
    int detkey = lappd_detkeys.at(i);
    std::string lappd_nr = std::to_string(detkey);
    std::string hist_time_lappds = str_time_lappds+lappd_nr;
    std::string hist_charge_lappds = str_charge_lappds+lappd_nr;
    time_LAPPDs[i] = new TH1F(hist_time_lappds.c_str(),"Time of LAPPDs",100,0,50);
    time_LAPPDs[i]->GetXaxis()->SetTitle("time [ns]");
    //time_LAPPDs[i]->GetYaxis()->SetRangeUser(0,200);
    charge_LAPPDs[i] = new TH1F(hist_charge_lappds.c_str(),"Number of hits of LAPPDs",60,0,60);
    charge_LAPPDs[i]->GetXaxis()->SetTitle("hits");
    //charge_LAPPDs[i]->GetYaxis()->SetRangeUser(0,20.);
    time_LAPPDs[i]->SetLineColor(i+1);
    charge_LAPPDs[i]->SetLineColor(i+1);
  }

  for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    charge.emplace(detkey,0.);
    time.emplace(detkey,0.);
  }

  //---------------------------------------------------------------
  //------------Get truth information (MCParticles) ---------------
  //---------------------------------------------------------------
  int n_flag0=0;
  if (verbose > 1) std::cout <<"Loop through MCParticles..."<<std::endl;
  for(int particlei=0; particlei<mcparticles->size(); particlei++){
    MCParticle aparticle = mcparticles->at(particlei);
    if (verbose > 1) std::cout <<"particle "<<particlei<<std::endl;
    if (verbose > 1) std::cout <<"Parent ID: "<<aparticle.GetParentPdg()<<std::endl;
    if (verbose > 1) std::cout <<"PDG code: "<<aparticle.GetPdgCode()<<std::endl;
    if (verbose > 1) std::cout <<"Flag: "<<aparticle.GetFlag()<<std::endl;
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
      if (verbose > 1) std::cout <<"True vtx: ( "<<truevtx_x<<" , "<<truevtx_y<<" , "<<truevtx_z<<" )"<<std::endl;
      if (verbose > 1) std::cout <<"True dir: ("<<truedir_x<<" , "<<truedir_y<<" , "<<truedir_z<<" )"<<std::endl;
      if ((truevtx_y< min_y) || (truevtx_y>max_y) || sqrt(truevtx_x*truevtx_x+truevtx_z*truevtx_z)>tank_radius){
        std::cout <<"Event vertex outside of Inner Structure! Don't plot projected vertex and ring..."<<std::endl;
        draw_vertex_temp=false;
        draw_ring_temp=false;
      }
      break;
    }
    else continue;
  }
 
  //in case there are no primary particles, don't plot vertex and ring
  if (n_flag0==0) {
    draw_vertex_temp=false;
    draw_ring_temp=false;
  }

  //---------------------------------------------------------------
  //-------------------Iterate over MCHits ------------------------
  //---------------------------------------------------------------

  int vectsize = MCHits->size();
  if (verbose > 0) std::cout <<"MCHits size: "<<vectsize<<std::endl; 
  if (vectsize!=0) tank_hit = true;
  total_hits_pmts=0;
  for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
    unsigned long chankey = apair.first;
    if (verbose > 3) std::cout <<"chankey: "<<chankey<<std::endl;
    Detector* thistube = geom->ChannelToDetector(chankey);
    unsigned long detkey = thistube->GetDetectorID();
    if (verbose > 3) std::cout <<"detkey: "<<detkey<<std::endl;
    if (thistube->GetDetectorElement()=="Tank"){
      hitpmt_detkeys.push_back(detkey);
      std::vector<MCHit>& Hits = apair.second;
      int hits_pmt = 0;
      int wcsim_id;
      for (MCHit &ahit : Hits){
        charge[detkey] += ahit.GetCharge();
        time[detkey] += ahit.GetTime();
        hits_pmt++;
      }
      time[detkey]/=hits_pmt;         //use mean time of all hits on one PMT
      total_hits_pmts++;
    }
  }

  //---------------------------------------------------------------
  //-------------------Fill time hists ----------------------------
  //---------------------------------------------------------------

  for (int i_pmt=0;i_pmt<hitpmt_detkeys.size();i_pmt++){
    unsigned long detkey = hitpmt_detkeys[i_pmt];
    if (charge[detkey]!=0) charge_PMTs->Fill(charge[detkey]);
    if (time[detkey]!=0) time_PMTs->Fill(time[detkey]);
    if (charge[detkey]!=0) charge_time_PMTs->Fill(time[detkey],charge[detkey]);
  }

  //---------------------------------------------------------------
  //------------- Determine max+min values ------------------------
  //---------------------------------------------------------------

  maximum_pmts = 0;
  maximum_lappds = 0;
  maximum_time_pmts = 0;
  maximum_time_lappds = 0;
  min_time_pmts = 999.;
  min_time_lappds = 999.;
  total_charge_pmts = 0;
  for (int i_pmt=0;i_pmt<hitpmt_detkeys.size();i_pmt++){
    unsigned long detkey = hitpmt_detkeys[i_pmt];
    if (charge[detkey]>maximum_pmts) maximum_pmts = charge[detkey];
    total_charge_pmts+=charge[detkey];
    if (time[detkey]>maximum_time_pmts) maximum_time_pmts = time[detkey];
    if (time[detkey]<min_time_pmts) min_time_pmts = time[detkey];
  }

  //---------------------------------------------------------------
  //-------------------Iterate over LAPPD hits --------------------
  //---------------------------------------------------------------

  total_hits_lappds=0;
  num_lappds_hit=0;
  if (verbose > 0) std::cout <<"Size of MCLAPPDhits: "<<MCLAPPDHits->size()<<std::endl;
  if(MCLAPPDHits){
    for (std::pair<unsigned long, std::vector<MCLAPPDHit>>&& apair : *MCLAPPDHits){
      unsigned long chankey = apair.first;
      Detector *det = geom->ChannelToDetector(chankey);
      if(det==nullptr){
        if (verbose > 0) std::cout <<"EventDisplay Tool: LAPPD Detector not found! "<<std::endl;;
        continue;
      }
      int detkey = det->GetDetectorID();
      std::vector<MCLAPPDHit>& hits = apair.second;
      if (verbose > 2) std::cout <<"detector key: "<<detkey<<std::endl;
      for (MCLAPPDHit& ahit : hits){
        std::vector<double> temp_pos = ahit.GetPosition();
        double x_lappd = temp_pos.at(0)-tank_center_x;
        double y_lappd = temp_pos.at(1)-tank_center_y;
        double z_lappd = temp_pos.at(2)-tank_center_z;
        double lappd_charge = 1.0;
        double t_lappd = ahit.GetTime();
        if ((lappds_selected && active_lappds.count(detkey)) || !lappds_selected){
          if (verbose > 2) std::cout <<"Detector Key LAPPD: "<<detkey<<std::endl;
          if (verbose > 2) std::cout <<"LAPPD hit at x="<<x_lappd<<", y="<<y_lappd<<", z="<<z_lappd<<std::endl;
          Position lappd_hit(x_lappd,y_lappd,z_lappd);
          if(hits_LAPPDs.count(detkey)==0) hits_LAPPDs.emplace(detkey,std::vector<Position>{});
          hits_LAPPDs.at(detkey).push_back(lappd_hit);
          //std::cout <<"Charge of LAPPD hit: "<<ahit.GetCharge()<<std::endl;         //not implemented yet
          charge_lappd[detkey]+=1;
          maximum_lappds++;
          total_hits_lappds++;
          if(time_lappd.count(detkey)==0){ time_lappd.emplace(detkey,std::vector<double>{}); }
          time_lappd[detkey].push_back(t_lappd);
          if (t_lappd > maximum_time_lappds) maximum_time_lappds = t_lappd;
          if (t_lappd < min_time_lappds) min_time_lappds = t_lappd;
        }
      }
      total_hits_lappds++;
    }
  } else {
    if (verbose > 0) std::cout<<"No MCLAPPDHits"<<std::endl;
  }
  num_lappds_hit = MCLAPPDHits->size();


  //---------------------------------------------------------------
  //-------------------Fill LAPPD hists ---------------------------
  //---------------------------------------------------------------

  for (auto&& ahisto : charge_LAPPDs){
    unsigned long detkey = ahisto.first;
    int histkey;
    std::vector<unsigned long>::iterator it = std::find(lappd_detkeys.begin(), lappd_detkeys.end(), detkey);
    if (it != lappd_detkeys.end()) histkey = std::distance(lappd_detkeys.begin(), it);
    else {
	std::cout << "Detkey " << detkey <<" Not Found in Geometry class for LAPPDs!" << std::endl;
	continue;
    }
    if ((lappds_selected && active_lappds_user.count(detkey)==1) || !lappds_selected){
      charge_LAPPDs[histkey]->Fill(charge_lappd[detkey]);
      for (int i_time=0;i_time<time_lappd[detkey].size();i_time++){
        if (time_lappd[detkey].at(i_time)) time_LAPPDs[histkey]->Fill(time_lappd[detkey].at(i_time));
      }
    }
  }

  //---------------------------------------------------------------
  //------------------ Overall max+min values ---------------------
  //---------------------------------------------------------------

  // for event display
  maximum_time_overall = (maximum_time_pmts>maximum_time_lappds)? maximum_time_pmts : maximum_time_lappds;
  min_time_overall = (min_time_pmts<min_time_lappds)? min_time_pmts : min_time_lappds;

  //for time and charge histograms
  int max_lappd_time=0;
  int max_lappd_charge=0;
  double temp_charge, temp_time;
  temp_charge = charge_LAPPDs[0]->GetMaximum();
  temp_time = time_LAPPDs[0]->GetMaximum(); 
  for (int i_lappd=1;i_lappd<n_lappds;i_lappd++){
    if (charge_LAPPDs[i_lappd]->GetMaximum()>temp_charge){temp_charge=charge_LAPPDs[i_lappd]->GetMaximum();max_lappd_charge= i_lappd;}
    if (time_LAPPDs[i_lappd]->GetMaximum()>temp_time){temp_time=time_LAPPDs[i_lappd]->GetMaximum();max_lappd_time= i_lappd;}
  }

  //---------------------------------------------------------------
  //------------------ Draw histograms ----------------------------
  //---------------------------------------------------------------

  if (draw_histograms){
    canvas_lappd->Divide(2,1);
    canvas_lappd->cd(1);
    charge_LAPPDs[max_lappd_charge]->SetStats(0);
    charge_LAPPDs[max_lappd_charge]->Draw();
    leg_charge = new TLegend(0.6,0.6,0.9,0.9);
    leg_charge->SetLineColor(0);
    std::string lappd_str = "LAPPD ";
    std::string lappd_nr = std::to_string(lappd_detkeys[max_lappd_charge]);
    std::string lappd_label = lappd_str+lappd_nr;
    leg_charge->AddEntry(charge_LAPPDs[max_lappd_charge],lappd_label.c_str());
    for (int i_lappd=0;i_lappd<n_lappds;i_lappd++){
      if (i_lappd==max_lappd_charge) continue;
      if ((lappds_selected && active_lappds_user.count(i_lappd)==1) || !lappds_selected){
        charge_LAPPDs[i_lappd]->Draw("same");
        std::string lappd_nr = std::to_string(lappd_detkeys[i_lappd]);
        std::string lappd_label = lappd_str+lappd_nr;
        leg_charge->AddEntry(time_LAPPDs[i_lappd],lappd_label.c_str());
      }
    }
    leg_charge->Draw();
    canvas_lappd->cd(2);
    time_LAPPDs[max_lappd_time]->SetStats(0);
    time_LAPPDs[max_lappd_time]->Draw();
    leg_time = new TLegend(0.6,0.6,0.9,0.9);
    //leg_time->SetNColumns(5);
    leg_time->SetLineColor(0);
    lappd_nr = std::to_string(lappd_detkeys[max_lappd_time]);
    lappd_label = lappd_str+lappd_nr;
    leg_time->AddEntry(time_LAPPDs[max_lappd_time],lappd_label.c_str());
    for (int i_lappd=1;i_lappd<n_lappds;i_lappd++){
      if (i_lappd == max_lappd_time) continue;
      if ((lappds_selected && active_lappds_user.count(i_lappd)==1)|| !lappds_selected){
        time_LAPPDs[i_lappd]->Draw("same");
        std::string lappd_nr = std::to_string(lappd_detkeys[i_lappd]);
        std::string lappd_label = lappd_str+lappd_nr;
        leg_time->AddEntry(time_LAPPDs[i_lappd],lappd_label.c_str());
      }
    }
    leg_time->Draw();
  }

  if (verbose > 0) std::cout <<"Maximum time LAPPDs: "<<maximum_time_lappds<<", minimum time LAPPDs: "<<min_time_lappds<<std::endl;

  //---------------------------------------------------------------
  //-------------------Iterate over MRD hits ----------------------
  //---------------------------------------------------------------

  if (verbose > 0) std::cout << "Looping over FACC/MRD hits...Size of FACC hits: "<<TDCData->size()<<std::endl;

  if(!TDCData){
    if (verbose > 0) std::cout<<"No TDC data to plot in Event Display!"<<std::endl;
  } else {
    if(TDCData->size()==0){
      if (verbose > 0) std::cout<<"No TDC hits to plot in Event Display!"<<std::endl;
    } else {
      for(auto&& anmrdpmt : (*TDCData)){
        unsigned long chankey = anmrdpmt.first;
        Detector* thedetector = geom->ChannelToDetector(chankey);
        if(thedetector->GetDetectorElement()!="MRD") facc_hit=true; // this is a veto hit, not an MRD hit.
        else mrd_hit = true;
        /*for(auto&& hitsonthismrdpmt : anmrdpmt.second){
          mrddigitpmtsthisevent.push_back(wcsimtubeid-1);
          //cout<<"recording MRD hit at time "<<hitsonthismrdpmt.GetTime()<<endl;
          mrddigittimesthisevent.push_back(hitsonthismrdpmt.GetTime());
          mrddigitchargesthisevent.push_back(hitsonthismrdpmt.GetCharge());
        }*/
      }
      int numdigits = mrddigittimesthisevent.size();
    }
  }

  //---------------------------------------------------------------
  //------------------ Draw Event Display -------------------------
  //---------------------------------------------------------------

  if (verbose > 0) std::cout <<"Drawing Event Display..." <<std::endl;

  //delete_canvas_contents();
  //delete canvas_ev_display;
  //canvas_ev_display=new TCanvas("canvas_ev_display","Event Display",900,900);
  //canvas_ev_display->Divide(1,1);

  canvas_ev_display->Clear();
  canvas_ev_display->Divide(1,1);
  make_gui();
  draw_event();
  canvas_ev_display->Modified();
  canvas_ev_display->Update();

  if (draw_histograms){
    canvas_pmt->Divide(2,1);
    canvas_pmt->cd(1);
    charge_PMTs->SetStats(0);
    charge_PMTs->Draw();
    canvas_pmt->cd(2);
    time_PMTs->SetStats(0);
    time_PMTs->Draw();
    canvas_pmt->Modified();
    canvas_pmt->Update();
    canvas_pmt_supplementary->cd();
    charge_time_PMTs->SetStats(0);
    charge_time_PMTs->Draw("colz");
    canvas_pmt_supplementary->Modified();
    canvas_pmt_supplementary->Update();
    canvas_lappd->Modified();
    canvas_lappd->Update();
  }

  if (save_plots){
    canvas_ev_display->SaveAs(filename_evdisplay.c_str());
    if (draw_histograms){
      canvas_pmt->SaveAs(filename_pmts.c_str());
      canvas_pmt_supplementary->SaveAs(filename_pmts_supplementary.c_str());
      canvas_lappd->SaveAs(filename_lappds.c_str());
    }
  }

  if (user_input){
    std::string in_string;
    std::cout <<"End of event. Next event? [y/n]"<<std::endl;
    std::cin >> in_string;
    double dummy;
    if (in_string == "y") {std::cout <<"Proceeding with next event."<<std::endl;}
    else if (in_string == "n") {std::cout <<"Not doing anything. Please escape the application...[Ctrl+C]"<<std::endl; std::cin>>dummy;}
    else {std::cout <<"No valid input format. Proceeding with next event."<<std::endl;}
  }

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (end of execute): "<<std::endl;
  //gObjectTable->Print();

  return true;
}


bool EventDisplay::Finalise(){

  //-----------------------------------------------------------------
  //---------------- Delete remaining objects -----------------------
  //-----------------------------------------------------------------

  canvas_ev_display->Clear();
  canvas_ev_display->Close();
  delete_canvas_contents();

  if (use_tapplication) {
    app_event_display->Terminate();
    delete app_event_display;
  }
  if (draw_histograms){
    delete leg_charge;
    delete leg_time;
  }
  delete canvas_pmt;
  delete canvas_pmt_supplementary;
  delete canvas_lappd;
  delete canvas_ev_display;

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

    if (verbose > 2) std::cout <<"Bird index: "<<Bird_Idx<<std::endl;

    //draw top circle
    p1 = (TPad*) canvas_ev_display->cd(1);
    canvas_ev_display->Draw();
    gPad->SetFillColor(0);
    p1->Range(0,0,1,1);
    top_circle = new TEllipse(0.5,0.5+(tank_height/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
    top_circle->SetFillColor(1);
    top_circle->SetLineColor(1);
    top_circle->SetLineWidth(1);
    top_circle->Draw();
    
    //draw bulk
    box = new TBox(0.5-TMath::Pi()*size_top_drawing,0.5-tank_height/tank_radius*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,0.5+tank_height/tank_radius*size_top_drawing);
    box->SetFillColor(1);
    box->SetLineColor(1);
    box->SetLineWidth(1);
    box->Draw();

    //draw lower circle
    bottom_circle = new TEllipse(0.5,0.5-(tank_height/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
    bottom_circle->SetFillColor(1);
    bottom_circle->SetLineColor(1);
    bottom_circle->SetLineWidth(1);
    bottom_circle->Draw();

    //update Event Display canvas
    canvas_ev_display->Modified();
    canvas_ev_display->Update();

}

void EventDisplay::draw_event(){

/*
    for (int i_marker=0;i_marker<marker_pmts_top.size();i_marker++){
      delete marker_pmts_top.at(i_marker);
    }
    for (int i_marker=0;i_marker<marker_pmts_bottom.size();i_marker++){
      delete marker_pmts_bottom.at(i_marker);
    }
    for (int i_marker=0;i_marker<marker_pmts_wall.size();i_marker++){
      delete marker_pmts_wall.at(i_marker);
    }
*/
    /*
   for (int i_marker=0;i_marker<marker_lappds.size();i_marker++){
      delete marker_lappds.at(i_marker);
    }*/
    //canvas_ev_display->Clear();

    marker_pmts_top.clear();
    marker_pmts_bottom.clear();
    marker_pmts_wall.clear();
    marker_lappds.clear();

    //draw everything necessary for event

    canvas_ev_display->cd(1);
    draw_event_PMTs();
    if (verbose > 2) std::cout <<"Drawing event PMTs finished"<<std::endl;

    draw_event_LAPPDs();
    if (verbose >2) std::cout <<"Drawing event LAPPDs finished."<<std::endl;

    //if (draw_mrd) draw_event_MRD();  //not implemented yet

    if (text_box) draw_event_box();
    if (verbose > 2) std::cout <<"Drawing event box finished."<<std::endl;

    draw_pmt_legend();
    if (verbose > 2) std::cout <<"Drawing pmt legend finished."<<std::endl;

    draw_lappd_legend();
    if (verbose > 2) std::cout <<"Drawing lappd legend finished." << std::endl;

    draw_schematic_detector();
    if (verbose > 2) std::cout <<"Drawing schematic detector finished."<<std::endl;

    if (draw_vertex_temp) draw_true_vertex();
    if (verbose > 2) std::cout <<"Drawing true vertex finished."<<std::endl;

    if (draw_ring_temp) draw_true_ring();
    else current_n_polylines=0;

    if (verbose > 2) std::cout <<"Drawing true ring finished."<<std::endl;

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
    std::string charge_str = " p.e.";
    //std::string annie_subrun = "ANNIE Subrun: ";
    std::string annie_time = "Trigger Time: ";
    std::string annie_time_unit = " [ns]";
    std::string annie_date = "Date: ";
    std::string trigger_str = "Trigger: ";
    std::string annie_run_number = std::to_string(runnumber);
    std::string annie_event_number = std::to_string(evnum);
    std::string total_charge_str = std::to_string(int(total_charge_pmts));
    std::string total_hits_str = std::to_string(total_hits_pmts);
    //std::string annie_subrun_number = std::to_string(subrunnumber);
    std::string annie_time_number = std::to_string(EventTime->GetNs());
    std::string lappd_hits_number = std::to_string(total_hits_lappds);
    std::string lappd_numbers_str = std::to_string(num_lappds_hit);
    std::string annie_run_label = annie_run+annie_run_number;
    std::string annie_event_label = annie_event+annie_event_number;
    std::string pmts_label = pmts_str+total_hits_str+hits_str+total_charge_str+charge_str;
    //std::string annie_subrun_label = annie_subrun+annie_subrun_number;
    std::string annie_time_label = annie_time+annie_time_number+annie_time_unit;
    std::string lappd_hits_label = lappds_str+lappd_numbers_str+modules_str+lappd_hits_number+hits2_str;
    std::string trigger_text_label = trigger_str+trigger_label;
    text_event_info->AddText("Date: 04/10/2019");         //FIXME: get date/time stamp from somewhere
    //text_event_info->AddText("Time: ");
    text_event_info->AddText(annie_time_label.c_str());
    text_event_info->AddText(annie_run_label.c_str());
    //text_event_info->AddText(annie_subrun_label.c_str());
    text_event_info->AddText(annie_event_label.c_str());
    text_event_info->AddText(pmts_label.c_str());
    text_event_info->AddText(lappd_hits_label.c_str());
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

    if (mode == "Charge") pmt_title = new TPaveLabel(0.05,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.15,0.5+tank_height/tank_radius*size_top_drawing,"charge [PMTs]","l");
    else if (mode == "Time") pmt_title = new TPaveLabel(0.05,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.15,0.5+tank_height/tank_radius*size_top_drawing,"time [PMTs]","l");
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

    std::string max_charge_pre = std::to_string(int(maximum_pmts));
    std::string pe_string = " p.e.";
    std::string max_charge = max_charge_pre+pe_string;
    std::string max_time_pre = std::to_string(int(maximum_time_overall));
    std::string time_string = " ns";
    std::string max_time;
    if (threshold_time_high==-999) max_time = max_time_pre+time_string;
    else max_time = std::to_string(int(threshold_time_high))+time_string;
    //std::cout <<"max time: "<<max_time<<std::endl;
    if (mode == "Charge") max_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_charge.c_str(),"L");
    else if (mode == "Time") max_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_time.c_str(),"L");
    max_text->SetFillColor(0);
    max_text->SetTextColor(1);
    max_text->SetTextFont(40);
    max_text->SetBorderSize(0);
    max_text->SetTextAlign(11);
    max_text->Draw();

    std::string min_charge_pre = (threshold==-999)? "0" : std::to_string(int(threshold));
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

    if (mode == "Charge") lappd_title = new TPaveLabel(0.85,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.95,0.5+tank_height/tank_radius*size_top_drawing,"charge [LAPPDs]","l");
    else if (mode == "Time") lappd_title = new TPaveLabel(0.85,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.95,0.5+tank_height/tank_radius*size_top_drawing,"time [LAPPDs]","l");
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
    //std::cout <<"max time lappd: "<<max_time_lappd<<std::endl;
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

  void EventDisplay::draw_event_PMTs(){

    //draw PMT event markers

    if (verbose > 0) std::cout <<"Drawing PMT hits..."<<std::endl;
    //clear already existing marker vectors
    marker_pmts_top.clear();
    marker_pmts_bottom.clear();
    marker_pmts_wall.clear();

    //calculate marker coordinates
    for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){

      unsigned long detkey = pmt_detkeys[i_pmt];
      double x[1];
      double y[1];
      if (charge[detkey]<threshold || ( threshold_time_high!=-999 && time[detkey]>threshold_time_high) || (threshold_time_low!=-999 && time[detkey]<threshold_time_low))  continue;       // if PMT does not have the charge required by threhold, discard
      if (fabs(y_pmt[detkey]-max_y)<0.01){

        //draw PMTs on the top of tank
        x[0]=0.5-size_top_drawing*x_pmt[detkey]/tank_radius;
        y[0]=0.5+(tank_height/tank_radius+1)*size_top_drawing-size_top_drawing*z_pmt[detkey]/tank_radius;
        TPolyMarker *marker_top = new TPolyMarker(1,x,y,"");
        if (mode == "Charge") color_marker = Bird_Idx+int((charge[detkey]-threshold)/(maximum_pmts-threshold)*254);
        else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(maximum_time_overall-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high != -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(threshold_time_high-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low != -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(maximum_time_overall-threshold_time_low)*254);
        else if (mode == "Time") color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
        marker_top->SetMarkerColor(color_marker);
        marker_top->SetMarkerStyle(8);
        marker_top->SetMarkerSize(1);
        marker_pmts_top.push_back(marker_top);
      }  else if (fabs(y_pmt[detkey]-min_y)<0.01 || fabs(y_pmt[detkey]+1.30912)<0.01){

        //draw PMTs on the bottom of tank
        x[0]=0.5-size_top_drawing*x_pmt[detkey]/tank_radius;
        y[0]=0.5-(tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*z_pmt[detkey]/tank_radius;
        TPolyMarker *marker_bottom = new TPolyMarker(1,x,y,"");
        if (mode == "Charge") color_marker = Bird_Idx+int((charge[detkey]-threshold)/(maximum_pmts-threshold)*254);
        else if (mode == "Time" && threshold_time_high == -999 && threshold_time_low == -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(maximum_time_overall-min_time_overall)*254); 
        else if (mode == "Time" && threshold_time_low == -999 && threshold_time_high != -999) color_marker = Bird_Idx+int((time[detkey]-min_time_overall)/(threshold_time_high-min_time_overall)*254);
        else if (mode == "Time" && threshold_time_low != -999 && threshold_time_high == -999) color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(maximum_time_overall-threshold_time_low)*254);
        else if (mode == "Time") color_marker = Bird_Idx+int((time[detkey]-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
        //std::cout <<"time: "<<time[i_pmt]<<", color: "<<color_marker<<std::endl;
        marker_bottom->SetMarkerColor(color_marker);
        marker_bottom->SetMarkerStyle(8);
        marker_bottom->SetMarkerSize(1);
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
        if (phi<-TMath::Pi() || phi>TMath::Pi())  std::cout <<"Drawing Event: Phi out of bounds! "<<", x= "<<x_pmt[detkey]<<", y="<<y_pmt[detkey]<<", z="<<z_pmt[detkey]<<std::endl;
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
        marker_bulk->SetMarkerSize(1);
        marker_pmts_wall.push_back(marker_bulk);
      }
    }

    //draw all PMT markers
    for (int i_marker=0;i_marker<marker_pmts_top.size();i_marker++){
      marker_pmts_top.at(i_marker)->Draw();
    }
    for (int i_marker=0;i_marker<marker_pmts_bottom.size();i_marker++){
      marker_pmts_bottom.at(i_marker)->Draw();
    }
    for (int i_marker=0;i_marker<marker_pmts_wall.size();i_marker++){
      marker_pmts_wall.at(i_marker)->Draw();
    }
  }

  void EventDisplay::draw_event_LAPPDs(){

      //draw LAPPD markers on Event Display
      if (verbose > 0) std::cout <<"Drawing LAPPD hits..."<<std::endl;

      marker_lappds.clear();
      double phi;
      double x[1];
      double y[1];
      double charge_single=1.;        //FIXME when charge is implemented in LoadWCSimLAPPD
      bool draw_lappd_markers=false;
      std::map<unsigned long,std::vector<Position>>::iterator lappd_hit_pos_it = hits_LAPPDs.begin();
      for(auto&& these_lappd_hit_positions : hits_LAPPDs){
        unsigned long detkey = these_lappd_hit_positions.first;
        for (int i_single_lappd=0; i_single_lappd<these_lappd_hit_positions.second.size(); i_single_lappd++){
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
          if (phi < - TMath::Pi() || phi>TMath::Pi())  std::cout <<"Drawing LAPPD event: ERROR: Phi out of bounds! "<<", x= "<<x_lappd<<", y="<<y_lappd<<", z="<<z_lappd<<std::endl;
          x[0]=0.5+phi*size_top_drawing;
          y[0]=0.5+y_lappd/tank_height*tank_height/tank_radius*size_top_drawing;
          TPolyMarker *marker_lappd = new TPolyMarker(1,x,y,"");
          if (mode == "Charge") color_marker = Bird_Idx+254;      //FIXME in case of actual charge information for LAPPDs
          else if (mode == "Time" && threshold_time_high==-999 && threshold_time_low==-999) {
            if (time_lappd_single > maximum_time_overall || time_lappd_single < min_time_overall) std::cout <<"Error: LAPPD hit time out of bounds! LAPPD time: "<<time_lappd_single<<", min time: "<<min_time_overall<<", max time: "<<maximum_time_overall<<std::endl;
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
        for (int i_draw=0; i_draw<marker_lappds.size();i_draw++){
          marker_lappds.at(i_draw)->Draw();
        }
      }
  }

  void EventDisplay::draw_event_MRD(){

    //draw MRD hits on Event Display

    //PLACEHOLDER
    if (verbose > 0) std::cout <<"Drawing MRD hits..."<<std::endl;
    marker_mrd.clear();
    for (int i_mrd = 0; i_mrd < n_mrd_pmts; i_mrd++){
        double x[1];
        double y[1];
    }
  }

  void EventDisplay::draw_true_vertex(){

    //draw interaction vertex --> projection on the walls

    find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, truedir_x, truedir_y, truedir_z, vtxproj_x, vtxproj_y, vtxproj_z);

    if (verbose > 0){
      std::cout <<"Projected true vtx on wall: ( "<< vtxproj_x<<" , "<<vtxproj_y<<" , "<<vtxproj_z<< " )"<<std::endl;
      std::cout <<"Drawing vertex on EventDisplay.... "<<std::endl;
    }  
    double xTemp, yTemp, phiTemp;
    int status_temp;
    translate_xy(vtxproj_x,vtxproj_y,vtxproj_z,xTemp,yTemp, status_temp, phiTemp);
    double x_vtx[1] = {xTemp};
    double y_vtx[1] = {yTemp};

    marker_vtx = new TPolyMarker(1,x_vtx,y_vtx,"");
    marker_vtx->SetMarkerColor(2);
    marker_vtx->SetMarkerStyle(22);
    marker_vtx->SetMarkerSize(1.);
    marker_vtx->Draw(); 

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

  void EventDisplay::draw_true_ring(){

    //draw the true ring projected from the interaction vertex at the Cherenkov angle w.r.t. true travel direction

    double phi_ring;
    double ringdir_x, ringdir_y, ringdir_z;
    double vtxproj_x2, vtxproj_y2, vtxproj_z2;
    double vtxproj_xtest, vtxproj_ytest, vtxproj_ztest;
    double vtxproj_xtest2, vtxproj_ytest2, vtxproj_ztest2;

    //create vector that has simply a lower z component and is located at the Cherenkov angle to the true direction vector
    double a = 1 - truedir_z*truedir_z/cos(thetaC)/cos(thetaC);
    double b = -2*(truedir_x*truedir_x+truedir_y*truedir_y)*truedir_z/cos(thetaC)/cos(thetaC);
    double c = (truedir_x*truedir_x+truedir_y*truedir_y)*(1-(truedir_x*truedir_x+truedir_y*truedir_y)/cos(thetaC)/cos(thetaC));
    double z_new1 = (-b+sqrt(b*b-4*a*c))/(2*a);
    double z_new2 = (-b-sqrt(b*b-4*a*c))/(2*a);
    double z_new;
    if ((z_new1 < -tank_radius) || (z_new1 > tank_radius)) z_new=z_new2;
    else z_new = z_new1;
    //create vector that has simply a lower y component and is located at the Cherenkov angle to the true direction vector
    double a2 = 1 - truedir_y*truedir_y/cos(thetaC)/cos(thetaC);
    double b2 = -2*(truedir_x*truedir_x+truedir_z*truedir_z)*truedir_y/cos(thetaC)/cos(thetaC);
    double c2 = (truedir_x*truedir_x+truedir_z*truedir_z)*(1-(truedir_x*truedir_x+truedir_z*truedir_z)/cos(thetaC)/cos(thetaC));
    double y_new1 = (-b2+sqrt(b2*b2-4*a2*c2))/(2*a2);
    double y_new2 = (-b2-sqrt(b2*b2-4*a2*c2))/(2*a2);
    double y_new;
    if ((y_new1 < min_y) || (y_new1 > max_y)) y_new=y_new2;
    else y_new = y_new1;
    //create vector that has simply a lower x component and is located at the Cherenkov angle to the true direction vector
    double a3 = 1 - truedir_x*truedir_x/cos(thetaC)/cos(thetaC);
    double b3 = -2*(truedir_z*truedir_z+truedir_y*truedir_y)*truedir_x/cos(thetaC)/cos(thetaC);
    double c3 = (truedir_z*truedir_z+truedir_y*truedir_y)*(1-(truedir_z*truedir_z+truedir_y*truedir_y)/cos(thetaC)/cos(thetaC));
    double x_new1 = (-b3+sqrt(b3*b3-4*a3*c3))/(2*a3);
    double x_new2 = (-b3-sqrt(b3*b3-4*a3*c3))/(2*a3);
    double x_new;
    if ((x_new1 < -tank_radius) || (x_new1 > tank_radius)) x_new=x_new2;
    else x_new = x_new1;

    //decide which axis to rotate around (axis should not be too close to particle direction)
    int status=-1;
    if (fabs(truedir_x)<fabs(truedir_y)){
      if (fabs(truedir_z)<fabs(truedir_x)){
        status=3;
        find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, truedir_x, truedir_y, z_new, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }
      else {
        status = 1;
        find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, x_new, truedir_y, truedir_z, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }
    }else {
      if (fabs(truedir_z)<fabs(truedir_y)){
          status = 3;
          find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, truedir_x, truedir_y, z_new, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }else{status = 2;
        find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, truedir_x, y_new, truedir_z, vtxproj_x2, vtxproj_y2, vtxproj_z2);
      }
    }

    if (verbose > 1){
      std::cout <<"Rotated direction: ( "<<truedir_x<<", "<<truedir_y<<", "<<z_new<<" )"<<std::endl;
      std::cout <<"Angle (rotated direction - original direction) : "<<acos((truedir_x*truedir_x+truedir_y*truedir_y+truedir_z*z_new)/(sqrt(truedir_x*truedir_x+truedir_y*truedir_y+z_new*z_new)))*180./TMath::Pi()<<std::endl;
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
    find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, truedir_x, y_new, truedir_z, vtxproj_xtest, vtxproj_ytest, vtxproj_ztest);
    translate_xy(vtxproj_xtest,vtxproj_ytest,vtxproj_ztest,xTemp,yTemp,status_temp,phiTemp);
    double x_test_vtx[1] = {xTemp};
    double y_test_vtx[1] = {yTemp};
    TPolyMarker *marker_test_vtx = new TPolyMarker(1,x_test_vtx,y_test_vtx,"");
    marker_test_vtx->SetMarkerColor(2);
    marker_test_vtx->SetMarkerStyle(21);
    marker_test_vtx->SetMarkerSize(1.);
    marker_test_vtx->Draw(); 
    find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, x_new, truedir_y, truedir_z, vtxproj_xtest2, vtxproj_ytest2, vtxproj_ztest2);
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
      Q_RingStart.q2 = truedir_y;
      Q_RingStart.q3 = truedir_z;
    }else if (status == 2){
      Q_RingStart.q1 = truedir_x;
      Q_RingStart.q2 = y_new;
      Q_RingStart.q3 = truedir_z;
    }else if (status == 3){
      Q_RingStart.q1 = truedir_x;
      Q_RingStart.q2 = truedir_y;
      Q_RingStart.q3 = z_new;
    }else {
      //if status variable was not filled for some reason, use rotation around z as best guess
      Q_RingStart.q1 = truedir_x;
      Q_RingStart.q2 = truedir_y;
      Q_RingStart.q3 = z_new;
    }

    //introduce status variables to keep track of the number of sub-areas the ring encounters (transitions wall - top / bottom)
    int status_pre;
    int status_ev;
    double phi_pre, phi_post, phi_calc;
    int status_phi_deriv;       //derivative: does phi get smaller or bigger?
    int i_ring_area=0;
    int i_angle=0;
    int i_valid_points=0;
    int points_area[10]={0};
    int firstev_area[10];

    for (int i_ring=0;i_ring<num_ring_points;i_ring++){
      phi_ring = i_ring*2*TMath::Pi()/double(num_ring_points);
      quaternion Q_RotAxis = define_rotationVector(phi_ring, truedir_x, truedir_y, truedir_z);
      quaternion Q_RotAxis_Inverted = invert_quaternion(Q_RotAxis);
      quaternion Q_Intermediate = multiplication(Q_RotAxis, Q_RingStart);
      quaternion Q_Rotated = multiplication(Q_Intermediate, Q_RotAxis_Inverted);
      double dir_sum = sqrt(Q_Rotated.q1*Q_Rotated.q1+Q_Rotated.q2*Q_Rotated.q2+Q_Rotated.q3*Q_Rotated.q3);
      //std::cout <<"Direction squared sum: "<<sqrt(Q_Rotated.q1*Q_Rotated.q1+Q_Rotated.q2*Q_Rotated.q2+Q_Rotated.q3*Q_Rotated.q3)<<std::endl;
      find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, Q_Rotated.q1/dir_sum , Q_Rotated.q2/dir_sum , Q_Rotated.q3/dir_sum , vtxproj_x2, vtxproj_y2, vtxproj_z2);
      if (verbose > 2) std::cout <<"Ring point # "<<i_ring<<", projected x/y/z = "<<vtxproj_x2<<"/"<<vtxproj_y2<<"/"<<vtxproj_z2<<std::endl;
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
            }else {
              phi_post = phi_calc;
              if (verbose > 2) std::cout <<"Phi pre: "<<phi_pre<<", phi post: "<<phi_post<<std::endl;
              if (fabs(phi_post-phi_pre) > TMath::Pi()/2.) {
                i_ring_area++;
                firstev_area[i_ring_area] = i_valid_points;
              }  
              if (verbose > 2) std::cout << "Ring area #: "<<i_ring_area<<std::endl;
              phi_pre = phi_post;
            }
          }
          points_area[i_ring_area]++;  
          xring[i_ring_area][i_valid_points-firstev_area[i_ring_area]] = xTemp;
          yring[i_ring_area][i_valid_points-firstev_area[i_ring_area]] = yTemp;
          i_valid_points++;
      }else {
        if (verbose > 2) std::cout <<"Ring area #: "<<i_ring_area<<std::endl;
      }
    }

    for (int i_area = 0; i_area<= i_ring_area;i_area++){
        ring_visual[i_area] = new TPolyLine(points_area[i_area],xring[i_area],yring[i_area]);
        ring_visual[i_area]->SetLineColor(2);
        ring_visual[i_area]->SetLineWidth(2);
        ring_visual[i_area]->Draw();
    }

    current_n_polylines = i_ring_area+1;


  }

  void EventDisplay::find_projected_xyz(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double &projected_x, double &projected_y, double &projected_z){

    //find projection of particle vertex onto the cylindrical ANNIE detector surface, given a particle direction

    double time_top, time_wall, time_wall1, time_wall2, time_min;
    double a,b,c; //for calculation of time_wall
    time_top = (dirY > 0)? (max_y-vtxY)/dirY : (min_y - vtxY)/dirY;
    if (verbose > 2) std::cout <<"dirY: "<<dirY<<", max_y: "<<max_y<<", min_y: "<<min_y<<", vtxY: "<<vtxY<<", vtxX: "<<vtxX<<", dirX: "<<dirX<<", vtxZ: "<<vtxZ<<",dirZ: "<<dirZ<<std::endl;
    if (time_top < 0) time_top = 999.;    //rule out unphysical times (negative)
    a = dirX*dirX + dirZ*dirZ;
    b = 2*vtxX*dirX + 2*vtxZ*dirZ;
    c = vtxX*vtxX+vtxZ*vtxZ-tank_radius*tank_radius;
    if (verbose > 2) std::cout <<"a: "<<a<<", b: "<<b<<", c: "<<c<<std::endl;
    time_wall1 = (-b+sqrt(b*b-4*a*c))/(2*a);
    time_wall2 = (-b-sqrt(b*b-4*a*c))/(2*a);
    if (time_wall2>=0 && time_wall1>=0) time_wall = (time_wall1<time_wall2) ? time_wall1 : time_wall2;
    else if (time_wall2 < 0 ) time_wall = time_wall1;
    else if (time_wall1 < 0 ) time_wall = time_wall2;
    else time_wall = 0;
    if ((vtxY+dirY*time_wall > max_y || vtxY+dirY*time_wall < min_y)) time_wall = 999;
    time_min = (time_wall < time_top && time_wall >=0 )? time_wall : time_top;

    if (verbose > 2) std::cout <<"time_min: "<<time_min<<", time wall1: "<<time_wall1<<", time wall2: "<<time_wall2<<", time wall: "<<time_wall<<", time top: "<<time_top<<std::endl;

    projected_x = vtxX+dirX*time_min;
    projected_y = vtxY+dirY*time_min;
    projected_z = vtxZ+dirZ*time_min;

  }

  void EventDisplay::translate_xy(double vtxX, double vtxY, double vtxZ, double &xWall, double &yWall, int &status_hit, double &phi_calc){

    //translate 3D position in the ANNIE coordinate frame into 2D xy positions in the cylindrical detection plane

    if (fabs(vtxY-max_y)<0.01){            //draw vtx projection on the top of tank
      xWall=0.5-size_top_drawing*vtxX/tank_radius;
      yWall=0.5+(tank_height/tank_radius+1)*size_top_drawing-size_top_drawing*vtxZ/tank_radius;
      status_hit = 1;
      phi_calc = -999;
      if (sqrt(vtxX*vtxX+vtxZ*vtxZ)>tank_radius) status_hit = 4;
    } else if (fabs(vtxY-min_y)<0.01){            //draw vtx projection on the top of tank
      xWall=0.5-size_top_drawing*vtxX/tank_radius;
      yWall=0.5-(tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*vtxZ/tank_radius;
      status_hit = 2;
      phi_calc = -999;
      if (sqrt(vtxX*vtxX+vtxZ*vtxZ)>tank_radius) status_hit = 4;
    }else {
      double phi;
      if (vtxX>0 && vtxZ>0) phi = atan(vtxZ/vtxX)+TMath::Pi()/2;
      else if (vtxX>0 && vtxZ<0) phi = atan(vtxX/-vtxZ);
      else if (vtxX<0 && vtxZ<0) phi = 3*TMath::Pi()/2+atan(vtxZ/vtxX);
      else if (vtxX<0 && vtxZ>0) phi = TMath::Pi()+atan(-vtxX/vtxZ);
      else phi = 0.;
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

    if (verbose > 3) std::cout <<"Delete canvas contents..."<<std::endl;
    if (text_event_info) delete text_event_info;
    if (pmt_title) delete pmt_title;
    if (lappd_title) delete lappd_title;
    if (schematic_tank) delete schematic_tank;
    if (schematic_facc) delete schematic_facc;
    if (schematic_mrd) delete schematic_mrd;
    if (border_schematic_mrd) delete border_schematic_mrd;
    if (border_schematic_facc) delete border_schematic_facc;
    if (top_circle) delete top_circle;
    if (bottom_circle) delete bottom_circle;
    if (box) delete box;

    if (max_text) delete max_text;
    if (min_text) delete min_text;
    if (max_lappd) delete max_lappd;
    if (min_lappd) delete min_lappd;

    for (int i_color=0; i_color < vector_colordot.size();i_color++){
      delete vector_colordot.at(i_color);
    }    
    for (int i_color=0; i_color < vector_colordot_lappd.size();i_color++){
      delete vector_colordot_lappd.at(i_color);
    }
    vector_colordot.clear();
    vector_colordot_lappd.clear();

    /*

    //markers, lines are automatically deleted when calling TCanvas::Clear
    std::cout <<"Delete marker pmts (top)"<<std::endl;
    for (int i_top = 0; i_top< marker_pmts_top.size(); i_top++){
      delete marker_pmts_top.at(i_top);
    }
    std::cout <<"Delete marker pmts (bottom)"<<std::endl;
    for (int i_bottom = 0; i_bottom< marker_pmts_bottom.size(); i_bottom++){
      delete marker_pmts_bottom.at(i_bottom);
    }
    std::cout <<"Delete marker pmts (wall)"<<std::endl;
    for (int i_wall = 0; i_wall< marker_pmts_wall.size(); i_wall++){
      delete marker_pmts_wall.at(i_wall);
    }
    std::cout <<"Delete marker lappds"<<std::endl;
    for (int i_lappd=0; i_lappd < marker_lappds.size(); i_lappd++){
      delete marker_lappds.at(i_lappd);
    }
    std::cout <<"Delete marker vtx"<<std::endl;
    if (marker_vtx) delete marker_vtx;
    std::cout <<"Delete ring visual polylines"<<std::endl;*/

    std::cout <<"current n polylines: "<<current_n_polylines<<std::endl;
    for (int i_ring=0; i_ring < current_n_polylines; i_ring++){
      if (ring_visual[i_ring]) delete ring_visual[i_ring];
    }


  }

  void EventDisplay::draw_schematic_detector(){

    //draw the schematic subdetectors on the top right of the picture

    //facc
    schematic_facc = new TBox(0.73,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.74,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    if (facc_hit) schematic_facc->SetFillColor(kOrange+1);
    else schematic_facc->SetFillColor(0);
    schematic_facc->Draw();
    border_schematic_facc= new TBox(0.73,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.74,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    border_schematic_facc->SetFillStyle(0);
    border_schematic_facc->SetLineColor(1);
    border_schematic_facc->SetLineWidth(1);
    border_schematic_facc->Draw();

    //tank
    schematic_tank = new TEllipse(0.80,0.75+0.5*tank_height/tank_radius*size_top_drawing,size_schematic_drawing,size_schematic_drawing);
    if (tank_hit) schematic_tank->SetFillColor(kOrange+1);
    else schematic_tank->SetFillColor(0);
    schematic_tank->SetLineColor(1);
    schematic_tank->SetLineWidth(1);
    schematic_tank->Draw();

    //mrd
    schematic_mrd = new TBox(0.85,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.85+1.6/2.7*2*size_schematic_drawing,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    if (mrd_hit) schematic_mrd->SetFillColor(kOrange+1);
    else schematic_mrd->SetFillColor(0);
    schematic_mrd->Draw();
    border_schematic_mrd = new TBox(0.85,0.75+0.5*tank_height/tank_radius*size_top_drawing-size_schematic_drawing,0.85+1.6/2.7*2*size_schematic_drawing,0.75+0.5*tank_height/tank_radius*size_top_drawing+size_schematic_drawing);
    border_schematic_mrd->SetFillStyle(0);
    border_schematic_mrd->SetLineColor(1);
    border_schematic_mrd->SetLineWidth(1);
    border_schematic_mrd->Draw();

  }

void EventDisplay::set_color_palette(){

    //calculate the numbers of the color palette

    Bird_Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
    for (int i_color=0;i_color<n_colors;i_color++){
      Bird_palette[i_color]=Bird_Idx+i_color;
    }
}

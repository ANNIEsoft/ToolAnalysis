#include "MonitorMRDEventDisplay.h"

MonitorMRDEventDisplay::MonitorMRDEventDisplay():Tool(){}


bool MonitorMRDEventDisplay::Initialise(std::string configfile, DataModel &data){

  //-------------------------------------------------------------------------------
  //----------------------- Useful header -----------------------------------------
  //-------------------------------------------------------------------------------

  if(configfile!="")  m_variables.Initialise(configfile);
  m_data= &data;

  //-------------------------------------------------------
  //-----------------Get Configuration---------------------
  //-------------------------------------------------------

  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutputPath",outpath);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("InActiveChannels",inactive_channels);

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Initialising"<<std::endl;

  if (outpath == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity > 1) std::cout <<"Output path for plots is "<<outpath<<std::endl;


  //get objects with allocated memory in ROOT
  //std::cout <<"List of Objects (Start of Initialise)"<<std::endl;
  //gObjectTable->Print();

  //-------------------------------------------------------
  //-----------------Get active channels-------------------
  //-------------------------------------------------------

  ifstream file(active_slots.c_str());
  int temp_crate, temp_slot;
  int loopnum=0;
  num_active_slots=0;
  num_active_slots_cr1=0;
  num_active_slots_cr2=0;

  while (!file.eof()){
    file>>temp_crate>>temp_slot;
    loopnum++;
    if (verbosity > 2) std::cout<<loopnum<<" , "<<temp_crate<<" , "<<temp_slot<<std::endl;
    if (file.eof()) break;
    //if (loopnum!=13){
    if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
      std::cout <<"ERROR (MonitorMRDEventDisplay): Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_slot<1 || temp_slot>num_slots){
      std::cout <<"ERROR (MonitorMRDEventDisplay): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
      continue;
    }
    active_channel[temp_crate-min_crate][temp_slot-1]=1;		//slot start with number 1 instead of 0, crates with 7
    nr_slot.push_back((temp_crate-min_crate)*100+(temp_slot)); 
    num_active_slots++;
    if (temp_crate-min_crate==0) {num_active_slots_cr1++;active_slots_cr1.push_back(temp_slot);}
    if (temp_crate-min_crate==1) {num_active_slots_cr2++;active_slots_cr2.push_back(temp_slot);}
    //}
  }
  file.close();

  //-------------------------------------------------------
  //-------------Get inactive channels---------------------
  //-------------------------------------------------------

  ifstream file_inactive(inactive_channels.c_str());
  int temp_channel;

  while (!file_inactive.eof()){
    file_inactive>>temp_crate>>temp_slot>>temp_channel;
    if (verbosity > 2) std::cout<<temp_crate<<" , "<<temp_slot<<" , "<<temp_channel<<std::endl;
    if (file_inactive.eof()) break;

    if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
      std::cout <<"ERROR (MonitorMRDEventDisplay): Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_slot<1 || temp_slot>num_slots){
      std::cout <<"ERROR (MonitorMRDEventDisplay): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_channel<0 || temp_channel > num_channels){
      std::cout <<"ERROR (MonitorMRDEventDisplay): Specified channel out of range [0...31]. Continue with next entry."<<std::endl;
      continue;
    }

    if (temp_crate == min_crate){
      inactive_ch_crate1.push_back(temp_channel);
      inactive_slot_crate1.push_back(temp_slot);
    } else if (temp_crate == min_crate+1){
      inactive_ch_crate2.push_back(temp_channel);
      inactive_slot_crate2.push_back(temp_slot);
    } else {
      std::cout <<"ERROR (MonitorMRDEventDisplay): Crate # out of range, entry ("<<temp_crate<<"/"<<temp_slot<<"/"<<temp_channel<<") not added to inactive channel configuration." <<std::endl;
    }
  }
  file_inactive.close();

  //-------------------------------------------------------
  //-----------------Load ANNIE geometry-------------------
  //-------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  m_data->CStore.Get("CrateSpaceToChannelNumMap",CrateSpaceToChannelNumMap);
  Position center_position = geom->GetTankCentre();
  tank_center_x = center_position.X();
  tank_center_y = center_position.Y();
  tank_center_z = center_position.Z();

  //-------------------------------------------------------
  //----------Initialize Event display histograms----------
  //-------------------------------------------------------

  hist_mrd_top = new TH2Poly("hist_mrd_top","Top View MRD paddles",1.6,3.,-2.,2);
  hist_mrd_side = new TH2Poly("hist_mrd_side","Side View MRD paddles",1.6,3.,-2.,2);
  hist_mrd_top->GetXaxis()->SetTitle("z [m]");
  hist_mrd_top->GetYaxis()->SetTitle("x [m]");
  hist_mrd_side->GetXaxis()->SetTitle("z [m]");
  hist_mrd_side->GetYaxis()->SetTitle("y [m]");
  hist_mrd_top->GetZaxis()->SetTitle("PMT # (vert)");
  hist_mrd_side->GetZaxis()->SetTitle("PMT # (hor)");
  hist_mrd_top->SetStats(0);
  hist_mrd_side->SetStats(0);

  // Set custom bin shapes for the histograms

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("MRD").begin();
                                                    it != Detectors->at("MRD").end();
                                                  ++it){
    Detector* amrdpmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = apmt->GetChannels()->begin()->first;
    std::cout <<"Setting up MRD paddles... Detkey: "<<detkey<<", chankey: "<<chankey<<std::endl;
    Paddle *mrdpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);
    std::cout <<"mrdpaddle pointer address: "<<mrdpaddle<<std::endl;

    double xmin = mrdpaddle->GetXmin();
    double xmax = mrdpaddle->GetXmax();
    double ymin = mrdpaddle->GetYmin();
    double ymax = mrdpaddle->GetYmax();
    double zmin = mrdpaddle->GetZmin();
    double zmax = mrdpaddle->GetZmax();
    int orientation = mrdpaddle->GetOrientation();    //0 is horizontal, 1 is vertical
    int half = mrdpaddle->GetHalf();                  //0 or 1

    std::cout <<"xmin: "<<xmin<<", xmax: "<<xmax<<", ymin: "<<ymin<<", zmin: "<<zmin<<", zmax: "<<zmax<<", orientation: "<<orientation<<", half: "<<half<<std::endl;

    if (orientation == 0){
      //horizontal layers --> side representation
      if (half==0) hist_mrd_side->AddBin(zmin-tank_center_z-enlargeBoxes,ymin-tank_center_y,zmax-tank_center_z+enlargeBoxes,ymax-tank_center_y);
      else hist_mrd_side->AddBin(zmin+shiftSecRow-tank_center_z-enlargeBoxes,ymin-tank_center_y,zmax+shiftSecRow-tank_center_z+enlargeBoxes,ymax-tank_center_y);
    } else {
      //vertical layers --> top representation
      if (half==0) hist_mrd_top->AddBin(zmin-tank_center_z-enlargeBoxes,xmin-tank_center_x,zmax-tank_center_z+enlargeBoxes,xmax-tank_center_x);
      else  hist_mrd_top->AddBin(zmin+shiftSecRow-tank_center_z-enlargeBoxes,xmin-tank_center_x,zmax+shiftSecRow-tank_center_z+enlargeBoxes,xmax-tank_center_x);
    }
  }


  //omit warning messages from ROOT: info messages - 1001, warning messages - 2001, error messages - 3001
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

  return true;
}


bool MonitorMRDEventDisplay::Execute(){

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Executing"<<std::endl;

  //-------------------------------------------------------
  //-----------------Get MRD Live data---------------------
  //-------------------------------------------------------

  std::string State;
  m_data->CStore.Get("State",State);

  if (State == "MRDSingle"){
    //update live vectors

    m_data->Stores["CCData"]->Get("Single",MRDout); 

    OutN = MRDout.OutN;
    Trigger = MRDout.Trigger;
    Value = MRDout.Value;
    Slot = MRDout.Slot;
    Channel = MRDout.Channel;
    Crate = MRDout.Crate;
    TimeStamp = MRDout.TimeStamp;

    MonitorMRDEventDisplay::PlotMRDEvent();

    Value.clear();
    Slot.clear();
    Channel.clear();
    Crate.clear();

    MRDout.Value.clear();
    MRDout.Slot.clear();
    MRDout.Channel.clear();
    MRDout.Crate.clear();
    MRDout.Type.clear();

  } else if (State == "DataFile" || State == "Wait"){
    //do nothing
    if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: State is "<<State<<", do nothing"<<std::endl;
  }else {
    std::cout <<"ERROR (MonitorMRDEventDisplay): State not recognized! Please check data format of MRD file."<<std::endl;
  }

  return true;
}


bool MonitorMRDEventDisplay::Finalise(){

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Finalising"<<std::endl;

  MRDdata->Delete();

  //delete all the pointer to objects that are still active (should all be deleted already, but just in case)
  if (hist_mrd_top) delete hist_mrd_top;
  if (hist_mrd_side) delete hist_mrd_side;

  return true;
}


void MonitorMRDEventDisplay::PlotMRDEvent(){

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: PlotMRDEvent"<<std::endl;

  t = time(0);
  struct tm *now = localtime( & t );

  title_time.str("");
  title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

  std::string top_title = "Top View ";
  std::string side_title = "Side View ";
  std::string top_Title = top_title+title_time.str();
  std::string side_Title = side_title+title_time.str();
  hist_mrd_side->SetTitle(side_Title.c_str());
  hist_mrd_top->SetTitle(top_Title.c_str());

  //fill the Event Display

  for (int i_data = 0; i_data < Values.size(); i_data++){
  
    int crate_id = Crate.at(i_data);
    int slot_id = Slot.at(i_data);
    int channel_id = Channel.at(i_data);
    int tdc_value = Value.at(i_data);

    //fill EventDisplay plots
    std::vector<int> crate_slot_channel{crate_id,slot_id,channel_id};
    int chankey = CrateSpaceToChannelNumMap->at(crate_slot_channel);
    std::cout <<"Converted Rack "<<crate_id<<", Slot "<<slot_id<<"Channel "<<channel_id<<" to channelkey: "<<chankey<<std::endl;

    Detector *det = (Detector*) geom->GetDetector(chankey);
    std::cout <<"det pointer address: "<<det<<std::endl;
    int detkey = det->GetDetectorID();
    std::cout <<"detkey: "<<detkey<<std::endl;
    Paddle *mrdpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);
    std::cout <<"mrdpaddle pointer address: "<<mrdpaddle<<std::endl;
    
    double xmin = mrdpaddle->GetXmin();
    
    double xmax = mrdpaddle->GetXmax();
    double ymin = mrdpaddle->GetYmin();
    double ymax = mrdpaddle->GetYmax();
    double zmin = mrdpaddle->GetZmin();
    double zmax = mrdpaddle->GetZmax();
    int orientation = mrdpaddle->GetOrientation();    //0 is horizontal, 1 is vertical
    int half = mrdpaddle->GetHalf();                  //0 or 1

    //fill histogram

    /*delete det;
    delete mrdpaddle;
    det=0;
    mrdpaddle=0;*/

  }

  //plot the Event Display

  TCanvas *canvas_mrd_evdisplay = new TCanvas("canvas_mrd_evdisplay","Canvas EventDisplay",1400,600);
  canvas_mrd_evdisplay->Divide(2,1);

  Bird_Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);

  canvas_mrd_evdisplay->cd(1);
  hist_mrd_side->Draw("colz");
  canvas_mrd_evdisplay->cd(2);
  hist_mrd_top->Draw("colz");

  std::stringstream ss_evdisplay;
  ss_evdisplay<<outpath<<"MRD_EventDisplay.jpg";
  canvas_mrd_evdisplay->SaveAs(ss_evdisplay.str().c_str());

  hist_mrd_side->Reset();
  hist_mrd_top->Reset();

  delete canvas_mrd_evdisplay;

  //get list of allocated objects (ROOT)
  //std::cout <<"List of Objects (End of UpdateMonitorPlots)"<<std::endl;
  //gObjectTable->Print();

}

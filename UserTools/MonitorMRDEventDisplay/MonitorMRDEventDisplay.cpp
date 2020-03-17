#include "MonitorMRDEventDisplay.h"

MonitorMRDEventDisplay::MonitorMRDEventDisplay():Tool(){}


bool MonitorMRDEventDisplay::Initialise(std::string configfile, DataModel &data){

  //-------------------------------------------------------------------------------
  //----------------------- Useful header -----------------------------------------
  //-------------------------------------------------------------------------------

  if(configfile!="")  m_variables.Initialise(configfile);
  m_data= &data;

  output_file = "mrdeventdisplays.root";

  //-------------------------------------------------------
  //-----------------Get Configuration---------------------
  //-------------------------------------------------------

  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("CustomRange",custom_range);
  m_variables.Get("RangeMin",custom_tdc_min);
  m_variables.Get("RangeMax",custom_tdc_max);
  m_variables.Get("OutputFormat",output_format);
  m_variables.Get("OutputFile",output_file);


  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Initialising"<<std::endl;

  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Output path for plots is "<<outpath<<std::endl;

  if (custom_range !=0 && custom_range != 1) custom_range=0;

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Custom_range: "<<custom_range<<", RangeMin: "<<custom_tdc_min<<", RangeMax: "<<custom_tdc_max<<std::endl;
  if (output_format != "root" && output_format != "image") output_format = "image";

  if (output_format == "root"){
    rootfile = new TFile(output_file.c_str(),"RECREATE");
  }
  //get objects with allocated memory in ROOT
  //std::cout <<"List of Objects (Start of Initialise)"<<std::endl;
  //gObjectTable->Print();

  //-------------------------------------------------------
  //-----------------Load ANNIE geometry-------------------
  //-------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",CrateSpaceToChannelNumMap);
  Position center_position = geom->GetTankCentre();
  tank_center_x = center_position.X();
  tank_center_y = center_position.Y();
  tank_center_z = center_position.Z();

  //-------------------------------------------------------
  //----------Initialize Event display histograms----------
  //-------------------------------------------------------

  hist_mrd_top = new TH2Poly("hist_mrd_top","Top View MRD paddles",1.6,3.,-2.,2);
  hist_mrd_side = new TH2Poly("hist_mrd_side","Side View MRD paddles",1.6,3.,-2.,2);
  hist_facc = new TH2Poly("hist_facc","FACC paddles",-1.66,-1.58,-2.5,2.5);
  hist_mrd_top->GetXaxis()->SetTitle("z [m]");
  hist_mrd_top->GetYaxis()->SetTitle("x [m]");
  hist_mrd_side->GetXaxis()->SetTitle("z [m]");
  hist_mrd_side->GetXaxis()->SetTitle("y [m]");
  hist_facc->GetXaxis()->SetTitle("z [m]");
  hist_facc->GetYaxis()->SetTitle("y [m]");
  hist_mrd_top->GetZaxis()->SetTitle("TDC");
  hist_mrd_side->GetZaxis()->SetTitle("TDC");
  hist_facc->GetZaxis()->SetTitle("TDC");
  hist_mrd_top->SetStats(0);
  hist_mrd_side->SetStats(0);
  hist_facc->SetStats(0);

  canvas_mrd_evdisplay = new TCanvas("canvas_mrd_evdisplay","Canvas EventDisplay",1400,600);
  canvas_facc_evdisplay = new TCanvas("canvas_facc_evdisplay","Canvas Veto EventDisplay",900,600);

  // Set custom bin shapes for the histograms

  std::map<std::string,std::map<unsigned long,Detector*> >* Detectors = geom->GetDetectors();

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
  
 for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Veto").begin();
                                                    it != Detectors->at("Veto").end();
                                                  ++it){
    Detector* afaccpmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = afaccpmt->GetChannels()->begin()->first;
    Paddle *faccpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);

    double xmin = faccpaddle->GetXmin();
    double xmax = faccpaddle->GetXmax();
    double ymin = faccpaddle->GetYmin();
    double ymax = faccpaddle->GetYmax();
    double zmin = faccpaddle->GetZmin();
    double zmax = faccpaddle->GetZmax();
    int orientation = faccpaddle->GetOrientation();    //0 is horizontal, 1 is vertical
    int half = faccpaddle->GetHalf();                  //0 or 1
    int side = faccpaddle->GetSide();

    //std::cout <<"chankey "<<chankey<<", xmin = "<<xmin<<", xmax = "<<xmax<<", ymin = "<<ymin<<", ymax = "<<ymax<<std::endl;

    hist_facc->AddBin(zmin-tank_center_z,ymin-tank_center_y,zmax-tank_center_z,ymax-tank_center_y);

  }

  evnum = 0;

  Bird_Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);

  //omit warning messages from ROOT: info messages - 1001, warning messages - 2001, error messages - 3001
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

  return true;
}


bool MonitorMRDEventDisplay::Execute(){

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Executing"<<std::endl;

  bool has_cc;
  m_data->CStore.Get("HasCCData",has_cc);

  std::string State;
  m_data->CStore.Get("State",State);

  if (has_cc){
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
    evnum++;

  } else if (State == "DataFile" || State == "Wait"){
    //do nothing
    if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: State is "<<State<<", do nothing"<<std::endl;
  }else {
    if (verbosity > 0) std::cout <<"ERROR (MonitorMRDEventDisplay): State not recognized! Please check data format of MRD file."<<std::endl;
  }
  }

  return true;
}


bool MonitorMRDEventDisplay::Finalise(){

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Finalising"<<std::endl;


  //delete all the pointer to objects that are still active (should all be deleted already, but just in case)


  if (output_format == "root") {
    rootfile->Close();
    delete rootfile;
  } else {
    delete hist_mrd_top;
    delete hist_mrd_side;
    delete hist_facc;
    delete canvas_mrd_evdisplay;
    delete canvas_facc_evdisplay;
  }	

  return true;
}


void MonitorMRDEventDisplay::PlotMRDEvent(){

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: PlotMRDEvent"<<std::endl;

  int min_tdc = 999999;
  int max_tdc = 0;

  t = time(0);
  struct tm *now = localtime( & t );

  title_time.str("");
  title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

  std::string top_title = "Top View ";
  std::string side_title = "Side View ";
  std::string facc_title = "FACC ";
  std::string top_Title = top_title+title_time.str();
  std::string side_Title = side_title+title_time.str();
  std::string facc_Title = facc_title+title_time.str();
  hist_mrd_side->SetTitle(side_Title.c_str());
  hist_mrd_top->SetTitle(top_Title.c_str());
  hist_facc->SetTitle(facc_Title.c_str());


  //fill the Event Display

  if (verbosity > 1)  std::cout <<"MonitorMRDEventDisplay: Filling MRD EventDisplay"<<std::endl;
  for (unsigned int i_data = 0; i_data < Value.size(); i_data++){
  
    int crate_id = Crate.at(i_data);
    int slot_id = Slot.at(i_data);
    int channel_id = Channel.at(i_data);
    int tdc_value = Value.at(i_data);

    if (tdc_value > max_tdc) max_tdc = tdc_value;
    if (tdc_value < min_tdc) min_tdc = tdc_value;

    //fill EventDisplay plots
    std::vector<int> crate_slot_channel{crate_id,slot_id,channel_id};
    if (CrateSpaceToChannelNumMap->find(crate_slot_channel)!=CrateSpaceToChannelNumMap->end()){
      int chankey = CrateSpaceToChannelNumMap->at(crate_slot_channel);
      //std::cout <<"Converted Rack "<<crate_id<<", Slot "<<slot_id<<"Channel "<<channel_id<<" to channelkey: "<<chankey<<std::endl;

      Detector *det = (Detector*) geom->GetDetector(chankey);
      int detkey = det->GetDetectorID();
      Paddle *mrdpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);
      
      int orientation = mrdpaddle->GetOrientation();    //0 is horizontal, 1 is vertical
      int half = mrdpaddle->GetHalf();                  //0 or 1
      Position paddle_pos = mrdpaddle->GetOrigin();
      double x_value = paddle_pos.X()-tank_center_x;
      double y_value = paddle_pos.Y()-tank_center_y;
      double z_value = paddle_pos.Z()-tank_center_z;

      std::string detector_element = det->GetDetectorElement();
      //std::cout <<"DetectorElement: "<<detector_element<<std::endl;

      if (detector_element == "MRD"){
        if (half == 1) z_value+=shiftSecRow;

        if (orientation == 0) {
          //int bin_nr = hist_mrd_side->FindBin(z_value,y_value);
          if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Filled "<<tdc_value<<" to hist_mrd_side"<<std::endl;
          hist_mrd_side->Fill(z_value,y_value,tdc_value);
        }
        else{
          //int bin_nr = hist_mrd_top->FindBin(z_value,x_value);
          if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Filled "<<tdc_value<<" to hist_mrd_top"<<std::endl;
          hist_mrd_top->Fill(z_value,x_value,tdc_value);
        } 
      } else if (detector_element == "Veto"){
          if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Filled "<<tdc_value<<" to hist_facc"<<std::endl;
          hist_facc->Fill(z_value,y_value,tdc_value);
      }

    } else {

      //
      //Crate / Slot / Channel configuration is not included in the geometry class --> error
      //
      if (verbosity > 0) std::cout <<"WARNING (MonitorMRDEventDisplay): Read out channel [ Crate "<<crate_id<<" / Slot "<<slot_id<<" / Channel "<<channel_id<<"] does not exist according to Geometry class. Check geometry class / hardware setup for inconsistencies!"<<std::endl;
    }

  }

  //plot the Event Display

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Plotting MRD EventDisplay..."<<std::endl;

  //Setting up Bird_Idx for nicer color palette

  canvas_mrd_evdisplay->Divide(2,1);
  canvas_mrd_evdisplay->cd(1);
  if (custom_range == 0) hist_mrd_side->GetZaxis()->SetRangeUser(0.1,max_tdc);    //make empty bins look empty (white) --> minimum range 0.1
  else hist_mrd_side->GetZaxis()->SetRangeUser(custom_tdc_min,custom_tdc_max);
  hist_mrd_side->GetZaxis()->SetTitleOffset(1.3);
  hist_mrd_side->GetZaxis()->SetTitleSize(0.03);
  hist_mrd_side->Draw("colz L");                           //option L to show contours around bins (indicating where MRD paddles are)
  canvas_mrd_evdisplay->Update();
  TPaletteAxis *palette = 
  (TPaletteAxis*) hist_mrd_side->GetListOfFunctions()->FindObject("palette");
  palette->SetX1NDC(0.9);
  palette->SetX2NDC(0.92);
  palette->SetY1NDC(0.1);
  palette->SetY2NDC(0.9);

  canvas_mrd_evdisplay->cd(2);
  if (custom_range == 0) hist_mrd_top->GetZaxis()->SetRangeUser(0.1,max_tdc);
  else hist_mrd_top->GetZaxis()->SetRangeUser(custom_tdc_min,custom_tdc_max);
  hist_mrd_top->GetZaxis()->SetTitleOffset(1.3);
  hist_mrd_top->GetZaxis()->SetTitleSize(0.03);
  hist_mrd_top->Draw("colz L");
  canvas_mrd_evdisplay->Update();
  TPaletteAxis *palette2 = 
  (TPaletteAxis*) hist_mrd_top->GetListOfFunctions()->FindObject("palette");
  palette2->SetX1NDC(0.9);
  palette2->SetX2NDC(0.92);
  palette2->SetY1NDC(0.1);
  palette2->SetY2NDC(0.9);

  canvas_facc_evdisplay->cd();
  if (custom_range == 0) hist_facc->GetZaxis()->SetRangeUser(0.1,max_tdc);
  else hist_facc->GetZaxis()->SetRangeUser(custom_tdc_min,custom_tdc_max);
  hist_facc->GetZaxis()->SetTitleOffset(1.3);
  hist_facc->GetZaxis()->SetTitleSize(0.03);
  hist_facc->Draw("colz L");
  canvas_facc_evdisplay->Update();
  TPaletteAxis *palette_facc = 
  (TPaletteAxis*) hist_facc->GetListOfFunctions()->FindObject("palette");
  palette_facc->SetX1NDC(0.9);
  palette_facc->SetX2NDC(0.92);
  palette_facc->SetY1NDC(0.1);
  palette_facc->SetY2NDC(0.9);

  if (output_format != "root"){
    std::stringstream ss_evdisplay, ss_evdisplay_facc;
    ss_evdisplay<<outpath<<"MRD_EventDisplay.jpg";
    ss_evdisplay_facc << outpath << "MRD_EventDisplay_FACC.jpg";
    canvas_mrd_evdisplay->SaveAs(ss_evdisplay.str().c_str());
    canvas_facc_evdisplay->SaveAs(ss_evdisplay_facc.str().c_str());
  } else {
    std::stringstream ss_canvas, ss_canvas_facc;
    ss_canvas<<"MRDEventDisplay_"<<evnum;
    ss_canvas_facc<<"MRDEventDisplayFACC_"<<evnum;
    canvas_mrd_evdisplay->SetName(ss_canvas.str().c_str());
    canvas_facc_evdisplay->SetName(ss_canvas_facc.str().c_str());
    rootfile->cd();
    canvas_mrd_evdisplay->Write();
    canvas_facc_evdisplay->Write();
  }
  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Resetting histograms..."<<std::endl;
  hist_mrd_side->Reset("M");
  hist_mrd_top->Reset("M");
  hist_facc->Reset("M");

  canvas_mrd_evdisplay->Clear();
  canvas_facc_evdisplay->Clear();

  //get list of allocated objects (ROOT)
  //std::cout <<"MonitorMRDEventDisplay: List of Objects (End of UpdateMonitorPlots)"<<std::endl;
  //gObjectTable->Print();

}

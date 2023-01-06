#include "PlotDecodedTimestamps.h"

PlotDecodedTimestamps::PlotDecodedTimestamps():Tool(){}


bool PlotDecodedTimestamps::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  //Get configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("OutputFile",output_timestamps);
  m_variables.Get("InputDataSummary",input_datasummary);
  m_variables.Get("InputTimestamps",input_timestamps);
  m_variables.Get("SecondsPerPlot",seconds_per_plot);
  m_variables.Get("TriggerWordConfig",triggerword_config);

  //Initialise ROOT files and trees
  this->SetupFilesAndTrees();

  //Setup color scheme for triggers
  this->SetupTriggerColorScheme();


  return true;
}


bool PlotDecodedTimestamps::Execute(){

  //Get first and last timestamp of DataSummary file
  double t_first=0, t_last=0;
  for (int i_entry = 0; i_entry < entries_datasummary; i_entry++){
    t_datasummary->GetEntry(i_entry);
    if (i_entry == 0) t_first = ctctimestamp/1000000000.;
    else if (i_entry == entries_datasummary-1) t_last = ctctimestamp/1000000000.;
    if (ctctimestamp/1000000000. < t_first) t_first = ctctimestamp/1000000000.;
  }

  //Make sure that t_last>t_first
  double t_max = (t_last >= t_first)? t_last : t_first;
  double t_min = (t_last < t_first)? t_last : t_first;
  int num_snapshots = (t_max-t_min)/seconds_per_plot;

  double t_first_pmt=0, t_last_pmt=0;
  for (int i_entry = 0; i_entry < entries_timestamps_pmt; i_entry++){
    t_timestamps_pmt->GetEntry(i_entry);
    if (i_entry == 0) t_first_pmt = t_pmt/1000000000.;
    else if (i_entry == entries_timestamps_pmt-1) t_last_pmt = t_pmt/1000000000.;
  }

  //Make sure that t_last>t_first
  double t_max_pmt = (t_last_pmt >= t_first_pmt)? t_last_pmt : t_first_pmt;
  double t_min_pmt = (t_last_pmt < t_first_pmt)? t_last_pmt : t_first_pmt;

  double t_first_mrd=0, t_last_mrd=0;
  for (int i_entry = 0; i_entry < entries_timestamps_mrd; i_entry++){
    t_timestamps_mrd->GetEntry(i_entry);
    if (i_entry == 0) t_first_mrd = t_mrd/1000000000.;
    else if (i_entry == entries_timestamps_mrd-1) t_last_mrd = t_mrd/1000000000.;
  }

  //Make sure that t_last>t_first
  double t_max_mrd = (t_last_mrd >= t_first_mrd)? t_last_mrd : t_first_mrd;
  double t_min_mrd = (t_last_mrd < t_first_mrd)? t_last_mrd : t_first_mrd;

  //Create needed canvases and frames for snapshots
  for (int i_snap = 0; i_snap < num_snapshots; i_snap++){
    std::stringstream ss_canvas, ss_frame, ss_title_frame;
    ss_canvas << "canvas_timeframe"<<i_snap;
    TCanvas *c = new TCanvas(ss_canvas.str().c_str(),ss_canvas.str().c_str(),900,600);
    ss_frame << "hist_timeframe"<<i_snap;
    ss_title_frame << "Timestamps Snapshot "<<i_snap;
    TH2F *frame = new TH2F(ss_frame.str().c_str(),ss_title_frame.str().c_str(),10,t_min+(i_snap*seconds_per_plot)-0.5,t_min+(i_snap+1)*seconds_per_plot+0.5,4,0,4);
    frame->GetYaxis()->SetBinLabel(1,"All CTC");
    frame->GetYaxis()->SetBinLabel(2,"CTC");
    frame->GetYaxis()->SetBinLabel(3,"MRD");
    frame->GetYaxis()->SetBinLabel(4,"PMT");
    frame->GetXaxis()->SetTimeDisplay(1);
    frame->GetXaxis()->SetLabelSize(0.03);
    frame->GetXaxis()->SetLabelOffset(0.03);
    frame->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M:%S}");
    frame->GetYaxis()->SetTickLength(0.);
    frame->SetStats(0);
    c->cd();
    frame->Draw();
    canvas_snapshot.push_back(c);
  } 

  //Create histogram for alternative triggerwords
  TH2F *h_alt_trigger = new TH2F("h_alt_trigger","Alternative triggers for tank orphans",64,0,64,500,-1000,1000);
  h_alt_trigger->GetXaxis()->SetTitle("triggerword");
  h_alt_trigger->GetYaxis()->SetTitle("#Delta t");
  TH2F *h_trigger_prompt = new TH2F("h_trigger_prompt","Triggerwords around prompt windows",64,0,64,500,-10000,10000);
  h_trigger_prompt->GetXaxis()->SetTitle("triggerword");
  h_trigger_prompt->GetYaxis()->SetTitle("#Delta t");
  TH2F *h_trigger_delayed = new TH2F("h_trigger_delayed","Triggerwords around extended windows",64,0,64,2000,-10000,80000);
  h_trigger_delayed->GetXaxis()->SetTitle("triggerword");
  h_trigger_delayed->GetYaxis()->SetTitle("#Delta t");
  TH1F *h_trigger_prompt_1D = new TH1F("h_trigger_prompt_1D","Triggerwords around prompt windows",64,0,64);
  h_trigger_prompt_1D->GetXaxis()->SetTitle("triggerword");
  h_trigger_prompt_1D->GetYaxis()->SetTitle("#");
  TH1F *h_trigger_delayed_1D = new TH1F("h_trigger_delayed_1D","Triggerwords around extended windows",64,0,64);
  h_trigger_delayed_1D->GetXaxis()->SetTitle("triggerword");
  h_trigger_delayed_1D->GetYaxis()->SetTitle("#");

  //Loop over DataSummary file information

  double t_first_matched=0, t_last_matched=0;
  std::vector<double> closest_tdiff_prompt;
  std::vector<int> closest_triggerword_prompt;
  std::vector<std::vector<double>> closest_tdiff_prompt_vec;
  std::vector<std::vector<int>> closest_triggerwords_prompt_vec;
  int n_prompt=0;
  std::vector<double> closest_tdiff_extended;
  std::vector<int> closest_triggerword_extended;
  std::vector<std::vector<double>> closest_tdiff_extended_vec;
  std::vector<std::vector<int>> closest_triggerwords_extended_vec;
  int n_delayed=0;
  for (int i_entry = 0; i_entry < entries_datasummary; i_entry++){
    t_datasummary->GetEntry(i_entry);
    if (i_entry==0) t_first_matched = ctctimestamp/1000000000.;
    else if (i_entry==entries_datasummary-1) t_last_matched = ctctimestamp/1000000000.;
    int index_hist = trunc((ctctimestamp/1000000000.-t_min)/seconds_per_plot);
    if (index_hist < 0 || index_hist >= num_snapshots) continue;
    int linecolor=1;
    if (triggerword == 5) linecolor=8;
    else if (triggerword == 36) linecolor=9;
    else if (triggerword == 31) linecolor = 807;
    TLine *l_mrd = new TLine(mrdtimestamp/1000000000.,2.1,mrdtimestamp/1000000000.,2.9);
    l_mrd->SetLineColor(1);
    l_mrd->SetLineStyle(1);
    l_mrd->SetLineWidth(1);
    l_mrd->Draw("same");
    TLine *l_pmt = new TLine(pmttimestamp/1000000000.,3.1,pmttimestamp/1000000000.,3.9);
    l_pmt->SetLineColor(1);
    l_pmt->SetLineStyle(1);
    l_pmt->SetLineWidth(1);
    TLine *l_ctc = new TLine(ctctimestamp/1000000000.,1.1,ctctimestamp/1000000000.,1.9);
    l_ctc->SetLineColor(linecolor);
    l_ctc->SetLineStyle(1);
    l_ctc->SetLineWidth(1);
    canvas_snapshot.at(index_hist)->cd();
    l_mrd->Draw("same");
    l_pmt->Draw("same");
    l_ctc->Draw("same");
    timestamp_snapshot.push_back(l_mrd);
    timestamp_snapshot.push_back(l_pmt);
    timestamp_snapshot.push_back(l_ctc);
    if (data_tank && ((extended_window == true && n_delayed < 500) || (extended_window == false && n_prompt < 500))){
     if (extended_window) {
        std::cout <<"Delayed event "<<n_delayed<<std::endl;
        n_delayed++;
     }
     else {
       std::cout <<"Prompt event "<<n_prompt<<std::endl;
       n_prompt++;
     }
     double min_diff = 1000000000000;
     int temp_closest_triggerword = 99;
     for (int i_trig = 0; i_trig < entries_timestamps; i_trig++){
       t_timestamps->GetEntry(i_trig);
       double tdiff=double(pmttimestamp)-double(t_ctc);
       tdiff = -tdiff;
       if (fabs(tdiff)<100000){
         if (extended_window) {
           h_trigger_delayed->Fill(triggerword_ctc,tdiff);
           h_trigger_delayed_1D->Fill(triggerword_ctc);
         } else {
           h_trigger_prompt->Fill(triggerword_ctc,tdiff);
           h_trigger_prompt_1D->Fill(triggerword_ctc);
         }
       }
     }
   }
  }

  double t_max_matched = (t_last_matched >= t_first_matched)? t_last_matched : t_first_matched;
  double t_min_matched = (t_last_matched < t_first_matched)? t_last_matched : t_first_matched;

  //Loop over complete trigger timestamp information
  for (int i_entry = 0; i_entry < entries_timestamps; i_entry++){
    t_timestamps->GetEntry(i_entry);
    if (t_ctc/1000000000.<t_min || t_ctc/1000000000.>t_max) continue;
    int index_hist = trunc((t_ctc/1000000000.-t_min)/seconds_per_plot);
    if (index_hist < 0 || index_hist >= num_snapshots) continue;
    if (std::find(vector_triggerwords.begin(),vector_triggerwords.end(),triggerword_ctc)==vector_triggerwords.end()) continue;
    int linecolor = map_triggerword_color[triggerword_ctc];
    TLine *l_ctc = new TLine(t_ctc/1000000000.,0.1,t_ctc/1000000000.,0.9);
    l_ctc->SetLineColor(linecolor);
    l_ctc->SetLineStyle(1);
    l_ctc->SetLineWidth(1);
    canvas_snapshot.at(index_hist)->cd();
    l_ctc->Draw("same");
    timestamp_snapshot.push_back(l_ctc);
  }

  //Loop over orphan timestamp information
  double t_first_orphan = 0, t_last_orphan = 0;
  std::vector<double> closest_tdiff;
  std::vector<int> closest_triggerword;
  std::vector<std::vector<double>> closest_tdiff_vec;
  std::vector<std::vector<int>> closest_triggerwords_vec;
  int n_orphan=0;
  for (int i_entry = 0; i_entry < entries_orphan; i_entry++){
    t_datasummary_orphan->GetEntry(i_entry);
    if (i_entry==0) t_first_orphan = orphantimestamp/1000000000.;
    else if (i_entry==entries_orphan-1) t_last_orphan = orphantimestamp/1000000000.;
    if (orphantimestamp/1000000000. < t_first_orphan) t_first_orphan = orphantimestamp/1000000000.;
    if (orphantimestamp/1000000000.<t_min || orphantimestamp/1000000000.>t_max) continue;
    int index_hist = trunc((orphantimestamp/1000000000.-t_min)/seconds_per_plot);
    if (index_hist < 0 || index_hist >=num_snapshots) continue;
    if (*cause_orphan == "tank_no_ctc") std::cout <<"OrphanedEventType: "<<*type_orphan<<", timestamp: "<<orphantimestamp<<", cause: "<<*cause_orphan<<", index_hist: "<<index_hist<<std::endl;
    std::cout <<"OrphanedEventType: "<<*type_orphan<<", timestamp: "<<orphantimestamp<<", cause: "<<*cause_orphan<<", index_hist: "<<index_hist<<std::endl;
    double lmin=0.;
    double lmax=0.;
    int linecolor=1;
    if (*cause_orphan == "tank_no_ctc" || *cause_orphan =="mrd_beam_no_ctc" || *cause_orphan =="mrd_cosmic_no_ctc") linecolor = 2;
    else if (*cause_orphan == "incomplete_tank_event") linecolor = kCyan;
    if (*type_orphan=="Tank"){
      lmin=3.1;
      lmax=3.9;
    } else if (*type_orphan=="MRD"){
      lmin=2.1;
      lmax=2.9;
    } else if (*type_orphan=="CTC"){
      continue; //Don't plot CTC orphans
      lmin=1.1;
      lmax=1.9;
    }
    TLine *l_orphan = new TLine(orphantimestamp/1000000000.,lmin,orphantimestamp/1000000000.,lmax);
    l_orphan->SetLineColor(linecolor);
    l_orphan->SetLineStyle(2);
    l_orphan->SetLineWidth(1);
    canvas_snapshot.at(index_hist)->cd();
    l_orphan->Draw("same");
    timestamp_snapshot.push_back(l_orphan);
    //Find alternative best triggerword for unmatched tank events
    if (*cause_orphan == "tank_no_ctc" && n_orphan < 500){
     n_orphan++;
     double min_diff = 1000000000000;
     int temp_closest_triggerword = 99;
     std::vector<int> temp_closest_triggerwords;
     std::vector<double> temp_closest_tdiff;
     for (int i_trig = 0; i_trig < entries_timestamps; i_trig++){
       t_timestamps->GetEntry(i_trig);
       double tdiff=double(orphantimestamp)-double(t_ctc);
       if (fabs(tdiff)<1000){
         temp_closest_triggerwords.push_back(triggerword_ctc);
         temp_closest_tdiff.push_back(tdiff);
         h_alt_trigger->Fill(triggerword_ctc,tdiff);
         if (fabs(tdiff) <= fabs(min_diff)){
           min_diff = tdiff;
           temp_closest_triggerword = triggerword_ctc;
         }
       }  
     }
    closest_triggerword.push_back(temp_closest_triggerword);
    closest_tdiff.push_back(min_diff);
    closest_triggerwords_vec.push_back(temp_closest_triggerwords);
    closest_tdiff_vec.push_back(temp_closest_tdiff);
   }
  }

  double t_max_orphan = (t_last_orphan >= t_first_orphan)? t_last_orphan : t_first_orphan;
  double t_min_orphan = (t_last_orphan < t_first_orphan)? t_last_orphan : t_first_orphan;

  double t_min_global = (t_min_orphan < t_min_matched)? t_min_orphan : t_min_matched;
  if (t_min < t_min_global) t_min_global = t_min;
  double t_max_global = (t_max_orphan > t_max_matched)? t_max_orphan : t_max_matched;
  if (t_max > t_max_global) t_max_global = t_max;

  canvas_timestreams = new TCanvas("canvas_timestreams","Timestreams",900,600);
  TH2F *frame_timestreams = new TH2F("frame_timestreams","Timestreams",10,t_min_global-100,t_max_global+100,5,0,5);
  frame_timestreams->GetYaxis()->SetBinLabel(1,"Orphaned Event");
  frame_timestreams->GetYaxis()->SetBinLabel(2,"Matched Event");
  frame_timestreams->GetYaxis()->SetBinLabel(3,"CTC Data");
  frame_timestreams->GetYaxis()->SetBinLabel(4,"MRD Data");
  frame_timestreams->GetYaxis()->SetBinLabel(5,"Tank PMT");
  frame_timestreams->GetXaxis()->SetTimeDisplay(1);
  frame_timestreams->GetXaxis()->SetLabelSize(0.03);
  frame_timestreams->GetXaxis()->SetLabelOffset(0.03);
  frame_timestreams->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M:%S}");
  frame_timestreams->GetYaxis()->SetTickLength(0.);
  frame_timestreams->SetStats(0);
  canvas_timestreams->cd();
  frame_timestreams->Draw();

  canvas_timestreams->cd();
  TBox *timestream_orphan = new TBox(t_min_orphan,0.1,t_max_orphan,0.9);
  timestream_orphan->SetFillColor(kBlack);
  timestream_orphan->Draw();
  TBox *timestream_matched = new TBox(t_min_matched,1.1,t_max_matched,1.9);
  timestream_matched->SetFillColor(kGreen+8);
  timestream_matched->Draw();
  TBox *timestream_ctc = new TBox(t_min,2.1,t_max,2.9);
  timestream_ctc->SetFillColor(kOrange);
  timestream_ctc->Draw();
  TBox *timestream_mrd = new TBox(t_min_mrd,3.1,t_max_mrd,3.9);
  timestream_mrd->SetFillColor(kCyan);
  timestream_mrd->Draw();
  TBox *timestream_pmt = new TBox(t_min_pmt,4.1,t_max_pmt,4.9);
  timestream_pmt->SetFillColor(kRed);
  timestream_pmt->Draw();

  timestreams_boxes.push_back(timestream_orphan);
  timestreams_boxes.push_back(timestream_matched);
  timestreams_boxes.push_back(timestream_ctc);
  timestreams_boxes.push_back(timestream_mrd);
  timestreams_boxes.push_back(timestream_pmt);

  f_out->cd();
  canvas_timestreams->Write();
  h_alt_trigger->Write();
  h_trigger_delayed->Write();
  h_trigger_delayed_1D->Write();
  h_trigger_prompt->Write();
  h_trigger_prompt_1D->Write();
  for (int i_snap = 0; i_snap < (int) canvas_snapshot.size(); i_snap++){
    canvas_snapshot.at(i_snap)->Write();
  }
  f_out->Close();
  delete f_out;

  ofstream file_alt_trigger("AlternativeTriggers.txt");
  for (int i_alt=0; i_alt < closest_triggerword.size(); i_alt++){
    file_alt_trigger << closest_triggerword.at(i_alt) << "    " << closest_tdiff.at(i_alt) << std::endl;
    std::vector<int> single_triggerwords = closest_triggerwords_vec.at(i_alt);
    std::vector<double> single_tdiff = closest_tdiff_vec.at(i_alt);
    for (int i_single = 0; i_single < single_tdiff.size(); i_single++){
      file_alt_trigger << single_triggerwords.at(i_single)<< "    "<< single_tdiff.at(i_single) << ", ";
    }
    file_alt_trigger << "----------------------------------------"<<std::endl;
  }
  file_alt_trigger.close();

  return true;
}


bool PlotDecodedTimestamps::Finalise(){

  f_datasummary->Close();
  delete f_datasummary;
  f_timestamps->Close();
  delete f_timestamps;
  
  for (int i_canvas = 0; i_canvas < (int) canvas_snapshot.size(); i_canvas++){
    delete canvas_snapshot.at(i_canvas);
  }
 
  for (int i_line = 0; i_line < (int) timestamp_snapshot.size(); i_line++){
    delete timestamp_snapshot.at(i_line);
  }

  delete canvas_timestreams;

  for (int i_box = 0; i_box < (int) timestreams_boxes.size(); i_box++){
    delete timestreams_boxes.at(i_box);
  }

  return true;
}

void PlotDecodedTimestamps::SetupFilesAndTrees(){

  std::cout <<"SetupFilesAndTrees"<<std::endl;

  f_datasummary = new TFile(input_datasummary.c_str(),"READ");
  t_datasummary = (TTree*) f_datasummary->Get("EventStats");
  t_datasummary_orphan = (TTree*) f_datasummary->Get("OrpahStats");
  f_timestamps = new TFile(input_timestamps.c_str(),"READ");
  t_timestamps = (TTree*) f_timestamps->Get("tree_timestamps_ctc");
  t_timestamps_pmt = (TTree*) f_timestamps->Get("tree_timestamps_pmt");
  t_timestamps_mrd = (TTree*) f_timestamps->Get("tree_timestamps_mrd");

  t_datasummary->SetBranchAddress("TriggerWord",&triggerword);
  t_datasummary->SetBranchAddress("CTCTimestamp",&ctctimestamp);
  t_datasummary->SetBranchAddress("MRDTimestamp",&mrdtimestamp);
  t_datasummary->SetBranchAddress("PMTTimestamp",&pmttimestamp);
  t_datasummary->SetBranchAddress("ExtendedWindow",&extended_window);
  t_datasummary->SetBranchAddress("WindowSize",&adc_samples);
  t_datasummary->SetBranchAddress("DataTank",&data_tank);
  t_datasummary->SetBranchAddress("TrigExtended",&trig_extended);
  t_datasummary->SetBranchAddress("TrigExtendedCC",&trig_extended_cc);
  t_datasummary->SetBranchAddress("TrigExtendedNC",&trig_extended_nc);


  type_orphan = new std::string;
  cause_orphan = new std::string;
  t_datasummary_orphan->SetBranchAddress("OrphanedEventType",&type_orphan);
  t_datasummary_orphan->SetBranchAddress("OrphanTimestamp",&orphantimestamp);
  t_datasummary_orphan->SetBranchAddress("OrphanCause",&cause_orphan);

  t_timestamps->SetBranchAddress("t_ctc",&t_ctc);
  t_timestamps->SetBranchAddress("triggerword_ctc",&triggerword_ctc);
  t_timestamps_pmt->SetBranchAddress("t_pmt",&t_pmt);
  t_timestamps_mrd->SetBranchAddress("t_mrd",&t_mrd);

  entries_datasummary = t_datasummary->GetEntries();
  entries_orphan = t_datasummary_orphan->GetEntries();
  entries_timestamps = t_timestamps->GetEntries();
  entries_timestamps_pmt = t_timestamps_pmt->GetEntries();
  entries_timestamps_mrd = t_timestamps_mrd->GetEntries();

  f_out = new TFile(output_timestamps.c_str(),"RECREATE");

}

void PlotDecodedTimestamps::SetupTriggerColorScheme(){

  if (verbosity > 2) std::cout <<"PlotDecodedTimestamps tool: SetupTriggerColorScheme"<<std::endl;


  //Always include beam and MRD CR trigger
  vector_triggerwords.push_back(5);
  vector_triggerwords.push_back(36);
  map_triggerword_color.emplace(5,8);
  map_triggerword_color.emplace(36,9);

  ifstream filetrigger(triggerword_config.c_str());
  int temp_word;
  int temp_color;
  while(!filetrigger.eof()){
    filetrigger >> temp_word >> temp_color;
    if (filetrigger.eof()) break;
    map_triggerword_color.emplace(temp_word,temp_color);
    vector_triggerwords.push_back(temp_word);
    if (verbosity > 2) std::cout <<"PlotDecodedTimestamps tool: Added triggerword "<<temp_word<<" to vector_triggerwords (color "<<temp_color<<")"<<std::endl;
  }
  filetrigger.close();

}

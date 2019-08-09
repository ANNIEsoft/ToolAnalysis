#include "MonitorTankLive.h"

MonitorTankLive::MonitorTankLive():Tool(){}


bool MonitorTankLive::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();
  m_data= &data; //assigning transient data pointer

  m_variables.Get("OutputPath",outpath);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("verbose",verbosity);

  if (outpath == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity >= 1) std::cout <<"Output path for PMT monitoring plots is "<<outpath<<std::endl;

  num_active_slots=0;
  n_active_slots_cr1=0;
  n_active_slots_cr2=0;
  n_active_slots_cr3=0;

  ifstream file(active_slots.c_str());
  int temp_crate, temp_slot;
  while (!file.eof()){

    file>>temp_crate>>temp_slot;
    if (verbosity >=1) std::cout << "Reading in active Slot: Crate "<<temp_crate<<", Card "<<temp_slot;
    if (file.eof()) {
      if (verbosity >=1) std::cout << std::endl;
      break;
    }
    if (temp_slot<2 || temp_slot>num_slots_tank){
      std::cout <<"ERROR (MonitorPMTLive): Specified slot "<<temp_slot<<" out of range for VME crates [2...21]. Continue with next entry."<<std::endl;
      continue;
    }
    if (!(std::find(crate_numbers.begin(),crate_numbers.end(),temp_crate)!=crate_numbers.end())) crate_numbers.push_back(temp_crate);
    std::vector<int>::iterator it = std::find(crate_numbers.begin(),crate_numbers.end(),temp_crate);
    int index = std::distance(crate_numbers.begin(), it);
    if (verbosity >=1) std::cout <<" (index: "<<index<<")"<<std::endl;
    switch (index) {
      case 0: {
        active_channel_cr1[temp_slot-1]=1;        //slot numbering starts at 1
        n_active_slots_cr1++;
        active_slots_cr1.push_back(temp_slot);
        break;
      }
      case 1: {
        active_channel_cr2[temp_slot-1]=1;
        n_active_slots_cr2++;
        active_slots_cr2.push_back(temp_slot);
        break;
      }
      case 2: {
        active_channel_cr3[temp_slot-1]=1;
        n_active_slots_cr3++;
        active_slots_cr3.push_back(temp_slot);
        break;
      }
    }
  }
  file.close();
  num_active_slots = n_active_slots_cr1+n_active_slots_cr2+n_active_slots_cr3;

  std::cout <<"Number of active Slots (Crate 1/2/3): "<<n_active_slots_cr1<<" / "<<n_active_slots_cr2<<" / "<<n_active_slots_cr3<<std::endl;
  if (verbosity >= 2) std::cout <<"Vector crate_numbers has size: "<<crate_numbers.size()<<std::endl;

  t = time(0);
  struct tm *now = localtime( & t );
  title_time.str("");
  title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

  init = true;                              //variable to initialise time histograms in the first Execute step

  MonitorTankLive::InitializeHists();				//initialize pointers to histograms and canvases

  //omit warning messages from ROOT: info messages - 1001, warning messages - 2001, error messages - 3001
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

  return true;

}


bool MonitorTankLive::Execute(){

  if (verbosity >= 2) std::cout <<"Tool MonitorTankLive: Executing...."<<std::endl;

  std::string State;
  
  //
  // real data readout structure for when the data format and firmware are ready
  //

  /*m_data->CStore.Get("State",State);
  if (State == "PMTSingle"){				//is data valid for live event plotting?->PMTSingle state
  if (verbosity >= 2) std::cout <<"PMTSingle Event: MonitorTankLive is executed..."<<std::endl;
  m_data->Stores["PMTData"]->Get("Single",PMTout); 

  SequenceID = PMTout.SequenceID;
  StartCount = PMTout.StartCount;
  TriggerNumber = PMTout.TriggerNumber;
  CardID = PMTout.CardID;
  Channels = PMTout.Channels;
  BufferSize = PMTout.BufferSize;
  EventSize = PMTout.EventSize;
  FullBufferSize = PMTout.FullBufferSize;
  StartTimeSec = PMTout.StartTimeSec;
  StartTimeNSec = PMTout.StartTimeNSec;
  TriggerCounts = PMTout.TriggerCounts;
  Rates = PMTout.Rates;				//not sure if Rates is going to be a member of the new store raw data format yet
  Data = PMTout.Data;*/

  //
  //implement fake data for now to test the plots. Crate and card configuration taken from a picture of the VME crates taken on July 18th, 2019 (might be outdated)
  //fill ADC values with Gaussian pedestal and Delta peak-like signal structure, distributed equally over the whole buffer
  //

  /*
  TRandom3 random_data;
  TRandom3 random_data2;
  BufferSize = 40000;
  SequenceID = 0;
  StartCount = 0;
  TriggerNumber = 0;
  EventSize = 1;
  FullBufferSize = 40000;
  StartTimeSec = 0;
  StartTimeNSec = 0;

  unsigned long crate_ids[36] = {0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3};
  unsigned long card_ids[36] = {2,3,4,5,7,8,9,10,11,12,13,14,16,2,3,4,5,6,8,9,10,11,13,14,15,16,18,19,4,5,6,7,9,10,11,12};
  for (int i_slot = 0; i_slot < 36; i_slot++){
    CrateID.push_back(crate_ids[i_slot]);
    CardID.push_back(card_ids[i_slot]);
    std::vector<unsigned long> tempdata;
    Channels.push_back(num_channels_tank);
    for (int i_channel =0; i_channel < num_channels_tank; i_channel++){
      for (int i_buffer = 0; i_buffer < BufferSize; i_buffer++){
        double random_decision = random_data2.Rndm();
        double random_value;
        unsigned int random_uint;
        if (random_decision < 0.1) random_value = 360;
        else random_value = random_data.Gaus(300,10);
        random_uint = (unsigned int)random_value;
        Data.push_back(random_uint);
      }
    }
  }
  */

  //
  // read in data from phase I root file for more realistic data
  //

  
  TFile *f = new TFile("/ANNIECode/data/RAWDataPhaseI/RAWDataR829S0p26.root");
  TTree *tree = (TTree*) f->Get("PMTData");
  Long64_t nentries = tree->GetEntries();
  std::cout <<"Number of entries: "<<nentries<<std::endl;
  unsigned short data[1000000];

  tree->SetBranchAddress("SequenceID",&SequenceID);
  tree->SetBranchAddress("StartCount",&StartCount);
  tree->SetBranchAddress("StartTimeSec",&StartTimeSec);
  tree->SetBranchAddress("StartTimeNSec",&StartTimeNSec);
  //t->SetBranchAddress("CardID",&CardID);			//reassign card and crate numbers according to new configuration
  tree->SetBranchAddress("Channels",&Channels);
  tree->SetBranchAddress("BufferSize",&BufferSize);
  tree->SetBranchAddress("FullBufferSize",&FullBufferSize);
  tree->SetBranchAddress("Eventsize",&EventSize);
  tree->SetBranchAddress("Data",data);

  int crate_id, card_id;

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    tree->GetEntry(i_slot);
    if (i_slot < n_active_slots_cr1) {
      crate_id = crate_numbers.at(0);
      card_id = active_slots_cr1.at(i_slot);
    }
    else if (i_slot < n_active_slots_cr1+n_active_slots_cr2) {
      crate_id = crate_numbers.at(1);
      card_id = active_slots_cr2.at(i_slot-n_active_slots_cr1);
    }
    else {
      crate_id = crate_numbers.at(2);
      card_id = active_slots_cr3.at(i_slot-n_active_slots_cr1-n_active_slots_cr2);
    }
    for (int i_buffer = 0; i_buffer < FullBufferSize; i_buffer++){
      Data.push_back(data[i_buffer]);
    }
    CrateID.push_back(crate_id);
    CardID.push_back(card_id);
  }

  std::cout <<"CrateID.size: "<<CrateID.size()<<std::endl;
  std::cout <<"CardID.size: "<<CardID.size()<<std::endl;

  //for high verbosity runs, provide entire available information as output
  if (verbosity >= 2){
    std::cout <<"/////////////////////////////////////////////////////////////////"<<std::endl;
    std::cout <<"MonitorTankLive: Detailed Event Information:"<<std::endl;
    std::cout <<"SequenceID: "<<SequenceID<<std::endl;
    std::cout <<"StartCount: "<<StartCount<<std::endl;
    std::cout <<"TriggerNumber: "<<TriggerNumber<<std::endl;
    std::cout <<"BufferSize: "<<BufferSize<<std::endl;
    std::cout <<"EventSize: "<<EventSize<<std::endl;
    std::cout <<"FullBufferSize: "<<FullBufferSize<<std::endl;
    std::cout <<"StartTimeSec: "<<StartTimeSec<<std::endl;
    std::cout <<"StartTimeNSec: "<<StartTimeNSec<<std::endl;
    std::cout <<"TriggerCounts size: "<<TriggerCounts.size()<<std::endl;
    std::cout <<"CrateID size: "<<CrateID.size()<<std::endl;
    std::cout <<"CardID size: "<<CardID.size()<<std::endl;
    std::cout <<"Channels size: "<<Channels.size()<<std::endl;
    std::cout <<"Rates size: "<<Rates.size()<<std::endl;
    std::cout <<"Data size: "<<Data.size()<<std::endl;
    std::cout <<"//////////////////////////////////////////////////////////////////"<<std::endl;
  }

  //get current time & date in string format
  t = time(0);
  struct tm *now = localtime( & t );
  title_time.str("");
  title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

  //plot all the PMT live monitoring plots
  MonitorTankLive::TankPlots();

  //clean up the vectors in use
  TriggerCounts.clear();
  CrateID.clear();
  CardID.clear();
  Channels.clear();
  //Rates.clear();
  Data.clear();

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after execute step)"<<std::endl;
  //gObjectTable->Print();

  return true;

  //
  //continuation of code for real data readout from Store
  //

  /*} else if (State == "DataFile" || State == "Wait"){                   
  if (verbosity >= 3) std::cout <<"Status File (Data File or Wait): MonitorTankLive is not executed..."<<std::endl;        
  return true;
  } else {
  if (verbosity >= 1) std::cout <<"State not recognized: "<<State<<std::endl;
  return true;
  }*/
  
}


bool MonitorTankLive::Finalise(){

  if (verbosity >= 2) std::cout <<"Tool MonitorTankLive: Finalising..."<<std::endl;

  delete h2D_ped;
  delete h2D_sigma;
  delete h2D_rate;
  delete h2D_pedtime;
  delete h2D_sigmatime;
  delete h2D_pedtime_short;
  delete h2D_sigmatime_short;

  delete canvas_ped;
  delete canvas_sigma;
  delete canvas_rate;
  delete canvas_pedtime;
  delete canvas_sigmatime;
  delete canvas_pedtime_short;
  delete canvas_sigmatime_short;

  for (int i_channel = 0; i_channel < num_channels_tank*num_active_slots; i_channel++){
    delete hChannels_freq.at(i_channel);
    delete hChannels_temp.at(i_channel);
  }

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    delete canvas_Channels_temp.at(i_slot);
    delete canvas_Channels_freq.at(i_slot);
  }

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after finalise step)"<<std::endl;
  //gObjectTable->Print();

  return true;

}

void MonitorTankLive::InitializeHists(){

  if (verbosity >= 3) std::cout <<"MonitorTankLive: Initialize Hists."<<std::endl;

  std::string str_ped = " Pedestal Mean (VME)";
  std::string str_sigma = " Pedestal Sigma (VME)";
  std::string str_rate = " Signal Counts (VME)";
  std::string str_pedtime = " Pedestal Evolution (100 values)";
  std::string str_sigmatime = " Sigma Evolution (100 values)";
  std::string str_pedtime_short = " Pedestal Evolution (5 values)";
  std::string str_sigmatime_short = " Sigma Evolution (5 values)";

  std::stringstream ss_title_ped, ss_title_sigma, ss_title_rate, ss_title_pedtime, ss_title_sigmatime, ss_title_pedtime_short, ss_title_sigmatime_short;
  ss_title_ped << title_time.str() << str_ped;
  ss_title_sigma << title_time.str() << str_sigma;
  ss_title_rate << title_time.str() << str_rate;
  ss_title_pedtime << title_time.str() << str_pedtime;
  ss_title_sigmatime << title_time.str() << str_sigmatime;
  ss_title_pedtime_short << title_time.str() << str_pedtime_short;
  ss_title_sigmatime_short << title_time.str() << str_sigmatime_short;

  h2D_ped = new TH2F("h2D_ped",ss_title_ped.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);								//Fitted gauss ADC distribution mean in 2D representation of channels, slots
  h2D_sigma = new TH2F("h2D_sigma",ss_title_sigma.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);							//Fitted gauss ADC distribution sigma in 2D representation of channels, slots
  h2D_rate = new TH2F("h2D_rate",ss_title_rate.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);								//Rate in 2D representation of channels, slots
  h2D_pedtime = new TH2F("h2D_pedtime",ss_title_pedtime.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);		//Time evolution of fitted pedestal mean values of all PMT channels (in percent)
  h2D_sigmatime = new TH2F("h2D_sigmatime",ss_title_sigmatime.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);		//Time evolution of fitted sigma values of all PMT channels (in percent)
  h2D_pedtime_short = new TH2F("h2D_pedtime_short",ss_title_pedtime_short.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);   //Time evolution of fitted pedestal mean values of all PMT channels (in percent)
  h2D_sigmatime_short = new TH2F("h2D_sigmatime_short",ss_title_sigmatime_short.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);   //Time evolution of fitted sigma values of all PMT channels (in percent)

  canvas_ped = new TCanvas("canvas_ped","Pedestal Mean (VME)",900,600);
  canvas_sigma = new TCanvas("canvas_sigma","Pedestal Sigma (VME)",900,600);
  canvas_rate = new TCanvas("canvas_rate","Signal Counts (VME)",900,600);
  canvas_pedtime = new TCanvas("canvas_pedtime","Pedestal Time Evolution (VME)",900,600);
  canvas_sigmatime = new TCanvas("canvas_sigmatime","Sigma Time Evolution (VME)",900,600);
  canvas_pedtime_short = new TCanvas("canvas_pedtime_short","Pedestal Time Evolution Short (VME)",900,600);
  canvas_sigmatime_short = new TCanvas("canvas_sigmatime_short","Sigma Time Evolution Short (VME)",900,600); 

  for (int i_active = 0; i_active<num_active_slots; i_active++){
    int slot_num, crate_num;
    if (i_active < n_active_slots_cr1){
      crate_num = crate_numbers.at(0);
      slot_num = active_slots_cr1.at(i_active);
    }
    else if (i_active >= n_active_slots_cr1 && i_active < n_active_slots_cr1+n_active_slots_cr2){
      crate_num = crate_numbers.at(1);
      slot_num = active_slots_cr2.at(i_active-n_active_slots_cr1);
    }
    else {
      crate_num = crate_numbers.at(2);
      slot_num = active_slots_cr3.at(i_active-n_active_slots_cr1-n_active_slots_cr2);
    }

    std::pair<int, int> temppair(crate_num,slot_num);
    map_crateslot_vector.insert(std::make_pair(temppair,i_active));			//map crate and slot ID to active slot number

    std::stringstream ss_name_hist;
    std::stringstream ss_title_hist;
    std::stringstream ss_name_canvas;
    std::stringstream ss_name_canvas_temp;
    std::stringstream ss_title_canvas;
    std::string crate_str="cr";
    std::string slot_str = "_slot";
    std::string ch_str = "_ch";
    std::string canvas_str = "canvas_";

    ss_name_canvas <<canvas_str<<crate_str<<crate_num<<slot_str<<slot_num;
    ss_name_canvas_temp << ss_name_canvas.str() << "_temp";
    ss_title_canvas << "Crate" <<crate_str<<"_Slot"<<slot_str;
    TCanvas *canvas_Channel_temp = new TCanvas(ss_name_canvas_temp.str().c_str(),ss_title_canvas.str().c_str(),900,600);
    canvas_Channels_temp.push_back(canvas_Channel_temp);
    TCanvas *canvas_Channel = new TCanvas(ss_name_canvas.str().c_str(),ss_title_canvas.str().c_str(),900,600);
    canvas_Channels_freq.push_back(canvas_Channel);

    //create frequency histograms
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      ss_name_hist.str("");
      ss_title_hist.str("");
      ss_name_hist<<crate_str<<crate_num<<slot_str<<slot_num<<ch_str<<i_channel;
      ss_title_hist << title_time.str() << " Freq (VME Crate " << crate_num << " Slot " << slot_num <<")";
      
      TH1I *hChannel_freq = new TH1I(ss_name_hist.str().c_str(),ss_title_hist.str().c_str(),maximum_adc-minimum_adc,minimum_adc,maximum_adc);		//ADC pulse shapes for current event, 1 canvas per slot
      hChannel_freq->GetXaxis()->SetTitle("ADC");
      hChannel_freq->GetYaxis()->SetTitle("Counts");
      hChannel_freq->SetLineWidth(2);
      hChannel_freq->SetLineColor(i_channel+1);
      hChannel_freq->SetStats(0);
      hChannel_freq->GetYaxis()->SetTitleOffset(1.35);
      hChannels_freq.push_back(hChannel_freq);

      channels_mean.push_back(0);
      channels_sigma.push_back(0);
      std::vector<double> pedtime, sigmatime;
      pedtime.assign(100,0.);
      sigmatime.assign(100,0.);
      timeev_ped.push_back(pedtime);
      timeev_sigma.push_back(sigmatime);
    }
  }
  canvas_Channels_freq.at(num_active_slots-1)->Clear();					//this canvas gets otherwise drawn with an additional frequency histogram (last channel)
}


void MonitorTankLive::TankPlots(){

  if (verbosity >= 1) std::cout <<"Plotting Tank Single Event Monitors...."<<std::endl;  				

  int i_loop = 0;
  int i_crate, i_slot;
  double max_ped = 0;
  double min_ped = 999999;
  double max_sigma = 0;
  double min_sigma = 999999;
  long max_rate = 0;
  long min_rate = 999999;
  double max_pedtime = 0.;
  double min_pedtime = 999999.;
  double max_sigmatime = 0.;
  double min_sigmatime = 9999999.;
  double max_pedtime_short = 0.;
  double min_pedtime_short = 999999.;
  double max_sigmatime_short = 0.;
  double min_sigmatime_short = 9999999.;

  //
  //Update the timestamp for the monitoring histograms
  //

  //for (int i_active = 0; i_active<num_active_slots; i_active++){
  for (int i_data=0; i_data < CrateID.size(); i_data++){

    int slot_num, crate_num, i_active;
    slot_num = CardID.at(i_data);
    crate_num = CrateID.at(i_data);

    if (crate_num == crate_numbers.at(0)){
      if (std::find(active_slots_cr1.begin(),active_slots_cr1.end(),slot_num)==active_slots_cr1.end()) {
        std::cout <<"MonitorTankLive ERROR: Slot read out from data ("<<slot_num<<") should not be active according to config file. Check config file..."<<std::endl;
      }
      i_crate = 0;
      std::vector<int>::iterator it = std::find(active_slots_cr1.begin(),active_slots_cr1.end(),slot_num);
      i_active = std::distance(active_slots_cr1.begin(), it);
    }
    else if (crate_num == crate_numbers.at(1)){
      if (std::find(active_slots_cr2.begin(),active_slots_cr2.end(),slot_num)==active_slots_cr2.end()) {
        std::cout <<"MonitorTankLive ERROR: Slot read out from data ("<<slot_num<<") should not be active according to config file. Check config file..."<<std::endl;
      }
      i_crate = 1;
      std::vector<int>::iterator it = std::find(active_slots_cr2.begin(),active_slots_cr2.end(),slot_num);
      i_active = n_active_slots_cr1 + std::distance(active_slots_cr2.begin(), it);  
    } 
    else if (crate_num == crate_numbers.at(2)) {
      if (std::find(active_slots_cr3.begin(),active_slots_cr3.end(),slot_num)==active_slots_cr3.end()) {
        std::cout <<"MonitorTankLive ERROR: Slot read out from data ("<<slot_num<<") should not be active according to config file. Check config file..."<<std::endl;
      }
      i_crate = 2;
      std::vector<int>::iterator it = std::find(active_slots_cr3.begin(),active_slots_cr3.end(),slot_num);
      i_active = n_active_slots_cr1 + n_active_slots_cr2 + std::distance(active_slots_cr3.begin(), it);
    }
    else {
      std::cout <<"MonitorTankLive ERROR: Crate read out from data ("<<CrateID.at(i_data)<<") should not be active according to config file. Check config file..."<<std::endl;
      continue;
    }
    i_slot = slot_num - 1;

    //std::cout <<"i_active from long method: "<<i_active<<std::endl;
    //std::cout <<"i_active from short method: "<<map_crateslot_vector.at(std::make_pair(crate_num,slot_num))<<std::endl;

    //std::cout <<"i_active: "<<i_active<<std::endl;
    /*if (i_active < n_active_slots_cr1){
    crate_num = crate_numbers.at(0);
    slot_num = active_slots_cr1.at(i_active);
    i_crate = 0;
    i_slot = slot_num-1;
    }
    else if (i_active >= n_active_slots_cr1 && i_active < n_active_slots_cr1+n_active_slots_cr2){
    crate_num = crate_numbers.at(1);
    slot_num = active_slots_cr2.at(i_active-n_active_slots_cr1);
    i_crate = 1;
    i_slot = slot_num-1;
    }
    else {
    crate_num = crate_numbers.at(2);
    slot_num = active_slots_cr3.at(i_active-n_active_slots_cr1-n_active_slots_cr2);
    i_crate = 2;
    i_slot = slot_num-1;
    }*/

    std::stringstream ss_title_hist, ss_title_hist_temp, ss_name_hist_temp;
    std::string crate_str="cr";
    std::string slot_str = "_slot";
    std::string ch_str = "_ch";
    ss_title_hist << title_time.str() << " Freq (VME Crate " << crate_num << " Slot " << slot_num <<")";
    ss_title_hist_temp << title_time.str() << " Temp (VME Crate " << crate_num << " Slot " << slot_num <<")";
    hChannels_freq.at(i_active*num_channels_tank)->SetTitle(ss_title_hist.str().c_str());

    std::string str_ped = " Pedestal Mean (VME)";
    std::string str_sigma = " Pedestal Sigma (VME)";
    std::string str_rate = " Signal Counts (VME)";
    std::string str_pedtime = " Pedestal Evolution (100 values)";
    std::string str_sigmatime = " Sigma Evolution (100 values)";
    std::string str_pedtime_short = " Pedestal Evolution (5 values)";
    std::string str_sigmatime_short = " Sigma Evolution (5 values)";

    std::stringstream ss_title_ped, ss_title_sigma, ss_title_rate, ss_title_pedtime, ss_title_sigmatime, ss_title_pedtime_short, ss_title_sigmatime_short;
    ss_title_ped << title_time.str() << str_ped;
    ss_title_sigma << title_time.str() << str_sigma;
    ss_title_rate << title_time.str() << str_rate;
    ss_title_pedtime << title_time.str() << str_pedtime;
    ss_title_sigmatime << title_time.str() << str_sigmatime;
    ss_title_pedtime_short << title_time.str() << str_pedtime_short;
    ss_title_sigmatime_short << title_time.str() << str_sigmatime_short;

    h2D_ped->SetTitle(ss_title_ped.str().c_str());
    h2D_sigma->SetTitle(ss_title_sigma.str().c_str());
    h2D_rate->SetTitle(ss_title_rate.str().c_str());
    h2D_pedtime->SetTitle(ss_title_pedtime.str().c_str());
    h2D_sigmatime->SetTitle(ss_title_sigmatime.str().c_str());
    h2D_pedtime_short->SetTitle(ss_title_pedtime_short.str().c_str());
    h2D_sigmatime_short->SetTitle(ss_title_sigmatime_short.str().c_str());

    if (init){
      for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
        ss_name_hist_temp.str("");
        ss_title_hist_temp.str("");
        ss_name_hist_temp<<crate_str<<crate_num<<slot_str<<slot_num<<ch_str<<i_channel<<"_temp";
        ss_title_hist_temp << title_time.str() << " Temp (VME Crate " << crate_num << " Slot " << slot_num <<")";
        TH1F* hChannel_temp = new TH1F(ss_name_hist_temp.str().c_str(),ss_title_hist_temp.str().c_str(),BufferSize,0,BufferSize);		//Temporal distribution for current event, 1 canvas per slot
        hChannel_temp->GetXaxis()->SetTitle("Buffer Position");
        hChannel_temp->GetYaxis()->SetTitle("Volts");
        hChannel_temp->SetLineWidth(2);
        hChannel_temp->SetLineColor(i_channel+1);
        hChannel_temp->GetYaxis()->SetTitleOffset(1.35);
        hChannel_temp->SetStats(0);
        hChannels_temp.push_back(hChannel_temp);
      }
    }

    hChannels_temp.at(i_active*num_channels_tank)->SetTitle(ss_title_hist_temp.str().c_str());

    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      i_loop = i_active*num_channels_tank+i_channel;
      
      //
      //compute & fill the frequency plots
      //
    
      long sum=0;
      for (int i_buffer = 0; i_buffer < BufferSize; i_buffer++){
        if (Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer) > channels_mean[i_loop]+5*channels_sigma[i_loop]) sum+= Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer);
        hChannels_freq.at(i_loop)->Fill(Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer));
      }

      TFitResultPtr gaussFitResult = hChannels_freq.at(i_loop)->Fit("gaus","Q");
      Int_t gaussFitResultInt = gaussFitResult;
      if (gaussFitResultInt == 0){						//status variable 0 means the fit was ok
        TF1 *gaus = (TF1*) hChannels_freq.at(i_loop)->GetFunction("gaus");
        std::stringstream ss_gaus;
        ss_gaus<<"gaus_"<<i_loop;
        gaus->SetName(ss_gaus.str().c_str());
        channels_mean.at(i_loop) = gaus->GetParameter(1);
        channels_sigma.at(i_loop) = gaus->GetParameter(2);
        vector_gaus.push_back(gaus);
      }

      //compute & fill the temporal plots
      double conversion = 2.415/pow(2.0, 12.0);               //conversion ADC counts --> Volts
      for (int i_buffer = 0; i_buffer < BufferSize/4; i_buffer++){
        int offset = channels_mean.at(i_loop);
        hChannels_temp.at(i_loop)->SetBinContent(i_buffer*4,(Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer*2)-offset)*conversion);
        hChannels_temp.at(i_loop)->SetBinContent(i_buffer*4+1,(Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer*2+1)-offset)*conversion);
        hChannels_temp.at(i_loop)->SetBinContent(i_buffer*4+2,(Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer*2+BufferSize/2)-offset)*conversion);
        hChannels_temp.at(i_loop)->SetBinContent(i_buffer*4+3,(Data.at(i_active*num_channels_tank*BufferSize+i_channel*BufferSize+i_buffer*2+BufferSize/2+1)-offset)*conversion);
      }

      //
      //fill 2D plots
      //

      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;			//top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_ped->SetBinContent(x,y,channels_mean.at(i_loop));
      h2D_sigma->SetBinContent(x,y,channels_sigma.at(i_loop));
      h2D_rate->SetBinContent(x,y,sum);

      //organize minimum and maximum values for the plots later
      if (channels_mean.at(i_loop) > max_ped) max_ped = channels_mean.at(i_loop);
      else if (channels_mean.at(i_loop) < min_ped) min_ped = channels_mean.at(i_loop);
      if (channels_sigma.at(i_loop) > max_sigma) max_sigma = channels_sigma.at(i_loop);
      else if (channels_sigma.at(i_loop) < min_sigma) min_sigma = channels_sigma.at(i_loop);
      if (sum > max_rate) max_rate = sum;
      else if (sum < min_rate) min_rate = sum;


      //fill the time evolution vectors
      for (int i_time=0; i_time < 99; i_time++){
        timeev_ped.at(i_loop).at(i_time) = timeev_ped.at(i_loop).at(i_time+1);
        timeev_sigma.at(i_loop).at(i_time) = timeev_sigma.at(i_loop).at(i_time+1);
      }
      timeev_ped.at(i_loop).at(99) = channels_mean.at(i_loop);
      timeev_sigma.at(i_loop).at(99) = channels_sigma.at(i_loop);

      h2D_pedtime->SetBinContent(x,y,(channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(0)));					//calculate ped change in absolute units
      h2D_sigmatime->SetBinContent(x,y,(channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(0)));			//calculate sigma change in absolute units
      h2D_pedtime_short->SetBinContent(x,y,(channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(95)));         //calculate ped change in absolute units for the last 5 values
      h2D_sigmatime_short->SetBinContent(x,y,(channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(95)));      //calculate sigma change in absolute units for the last 5 values

      if (((channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(0)))>max_pedtime) max_pedtime = (channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(0));
      if (((channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(0)))<min_pedtime) min_pedtime = (channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(0));
      if (((channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(0)))>max_sigmatime) max_sigmatime = (channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(0));
      if (((channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(0)))<min_sigmatime) min_sigmatime = (channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(0));

      if (((channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(95)))>max_pedtime_short) max_pedtime_short = (channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(95));
      if (((channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(95)))<min_pedtime_short) min_pedtime_short = (channels_mean.at(i_loop)-timeev_ped.at(i_loop).at(95));
      if (((channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(95)))>max_sigmatime_short) max_sigmatime_short = (channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(95));
      if (((channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(95)))<min_sigmatime_short) min_sigmatime_short = (channels_sigma.at(i_loop)-timeev_sigma.at(i_loop).at(95));

      } 
    }

    init = false;

    //
    //draw pedestal 2D histogram
    //

    TPad *p = (TPad*) canvas_ped->cd();
    h2D_ped->SetStats(0);
    h2D_ped->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_ped->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_ped->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_ped->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_ped->LabelsOption("v");
    h2D_ped->Draw("colz");

    std::vector<TBox*> vector_box_inactive;
    for (int i_slot = 0; i_slot < num_slots_tank*num_crates_tank; i_slot++){
      int slot_in_crate;
      int crate_nr;
      if (i_slot < num_slots_tank) {slot_in_crate = i_slot; crate_nr=0;}
      else if (i_slot < 2*num_slots_tank) {slot_in_crate = i_slot -num_slots_tank; crate_nr=1;}
      else {slot_in_crate = i_slot - 2*num_slots_tank; crate_nr=2;}

      TBox *box_inactive;
      if (active_channel_cr1[slot_in_crate] == 0 && crate_nr==0) {
        TBox *box_inactive = new TBox(slot_in_crate,2*num_channels_tank,slot_in_crate+1,3*num_channels_tank);
        box_inactive->SetFillStyle(3004);
        box_inactive->SetFillColor(1);
        vector_box_inactive.push_back(box_inactive);
        box_inactive->Draw("same");
      }
      if (active_channel_cr2[slot_in_crate] == 0 && crate_nr==1) {
        TBox *box_inactive = new TBox(slot_in_crate,num_channels_tank,slot_in_crate+1,2*num_channels_tank);
        box_inactive->SetFillStyle(3004);
        box_inactive->SetFillColor(1);
        vector_box_inactive.push_back(box_inactive);
        box_inactive->Draw("same");
      }
      if (active_channel_cr3[slot_in_crate] == 0 && crate_nr==2) {
        TBox *box_inactive = new TBox(slot_in_crate,0,slot_in_crate+1,num_channels_tank);
        box_inactive->SetFillStyle(3004);
        box_inactive->SetFillColor(1);
        vector_box_inactive.push_back(box_inactive);
        box_inactive->Draw("same");
      }
    }
    p->Update();
    if (h2D_ped->GetMaximum()>0.){
    if (abs(max_ped-min_ped)==0) h2D_ped->GetZaxis()->SetRangeUser(min_ped-1,max_ped+1);
    else h2D_ped->GetZaxis()->SetRangeUser(min_ped-0.5,max_ped+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_ped->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
    }
    p->Update();

    TLine *line1 = new TLine(-1,4,num_slots_tank,4.);
    line1->SetLineWidth(2);
    line1->Draw("same");
    TLine *line2 = new TLine(-1,8.,num_slots_tank,8.);
    line2->SetLineWidth(2);
    line2->Draw("same");

    std::stringstream ss_crate1, ss_crate2, ss_crate3;
    ss_crate1 << "ANNIEVME0"<<crate_numbers.at(0);
    ss_crate2 << "ANNIEVME0"<<crate_numbers.at(1);
    ss_crate3 << "ANNIEVME0"<<crate_numbers.at(2);

    TText *text_crate1 = new TText(0.04,0.68,ss_crate1.str().c_str());
    text_crate1->SetNDC(1);
    text_crate1->SetTextSize(0.030);
    text_crate1->SetTextAngle(90.);
    TText *text_crate2 = new TText(0.04,0.41,ss_crate2.str().c_str());
    text_crate2->SetNDC(1);
    text_crate2->SetTextSize(0.030);
    text_crate2->SetTextAngle(90.);
    TText *text_crate3 = new TText(0.04,0.14,ss_crate3.str().c_str());
    text_crate3->SetNDC(1);
    text_crate3->SetTextSize(0.030);
    text_crate3->SetTextAngle(90.);

    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();

    TLatex *label_mean = new TLatex(0.905,0.92,"#mu_{Ped}");
    label_mean->SetNDC(1);
    label_mean->SetTextSize(0.030);
    label_mean->Draw();

    std::stringstream ss_ped;
    ss_ped<<outpath<<"PMT_2D_Ped.jpg";
    if (verbosity >=2) std::cout <<"Output path Pedestal 2D plot: "<<ss_ped.str()<<std::endl;
    canvas_ped->SaveAs(ss_ped.str().c_str());

    //
    //draw pedestal sigma 2D histogram
    //

    TPad *p_sigma = (TPad*) canvas_sigma->cd();
    h2D_sigma->SetStats(0);
    h2D_sigma->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_sigma->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p_sigma->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_sigma->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_sigma->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_sigma->LabelsOption("v");
    h2D_sigma->Draw("colz");

    for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
      vector_box_inactive.at(i_box)->Draw("same");
    }

    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();
    line1->Draw("same");
    line2->Draw("same");

    TLatex *label_sigma = new TLatex(0.905,0.92,"#sigma_{Ped}");
    label_sigma->SetNDC(1);
    label_sigma->SetTextSize(0.030);
    label_sigma->Draw();

    p_sigma ->Update();

    if (h2D_sigma->GetMaximum()>0.){
    if (abs(max_sigma-min_sigma)==0) h2D_sigma->GetZaxis()->SetRangeUser(min_sigma-1,max_sigma+1);
    else h2D_sigma->GetZaxis()->SetRangeUser(min_sigma-0.5,max_sigma+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_sigma->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
    }
    p_sigma->Update();
    std::stringstream ss_sigma;
    ss_sigma<<outpath<<"PMT_2D_Sigma.jpg";
    canvas_sigma->SaveAs(ss_sigma.str().c_str());
    if (verbosity >=2) std::cout <<"Output path Pedestal Sigma 2D plot: "<<ss_sigma.str()<<std::endl;

    //
    //draw rate 2D histogram
    //

    TPad *p_rate = (TPad*) canvas_rate->cd();
    h2D_rate->SetStats(0);
    h2D_rate->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_rate->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p_rate->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_rate->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_rate->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_rate->LabelsOption("v");
    h2D_rate->Draw("colz");

    for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
      vector_box_inactive.at(i_box)->Draw("same");
    }

    line1->Draw("same");
    line2->Draw("same");
    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();
    TText *label_rate = new TLatex(0.905,0.92,"Counts");
    label_rate->SetNDC(1);
    label_rate->SetTextSize(0.030);
    label_rate->Draw();
    p_rate ->Update();

    if (h2D_rate->GetMaximum()>0.){
    if (abs(max_rate-min_rate)==0) h2D_sigma->GetZaxis()->SetRangeUser(min_rate-1,max_rate+1);
    else h2D_rate->GetZaxis()->SetRangeUser(min_rate-0.5,max_rate+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_rate->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
    }
    p_rate->Update();

    std::stringstream ss_rate;
    ss_rate<<outpath<<"PMT_2D_Rate.jpg";
    canvas_rate->SaveAs(ss_rate.str().c_str());
    if (verbosity >=2) std::cout <<"Output path Pedestal Rate plot: "<<ss_rate.str()<<std::endl;

    //
    //draw pedestal time evolution plot
    //

    TPad *p_pedtime = (TPad*) canvas_pedtime->cd();
    h2D_pedtime->SetStats(0);
    h2D_pedtime->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_pedtime->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p_pedtime->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_pedtime->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_pedtime->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_pedtime->LabelsOption("v");
    h2D_pedtime->Draw("colz");

    for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
      vector_box_inactive.at(i_box)->Draw("same");
    }

    line1->Draw("same");
    line2->Draw("same");
    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();
    TLatex *label_pedtime = new TLatex(0.905,0.92,"#Delta #mu_{Ped}");
    label_pedtime->SetNDC(1);
    label_pedtime->SetTextSize(0.030);
    label_pedtime->Draw();
    p_pedtime->Update();

    if (h2D_pedtime->GetMaximum()>0.){
    if (abs(max_pedtime-min_pedtime)==0) h2D_pedtime->GetZaxis()->SetRangeUser(min_pedtime-1,max_pedtime+1);
    else h2D_pedtime->GetZaxis()->SetRangeUser(min_pedtime-0.5,max_pedtime+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_pedtime->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
    }
    p_pedtime->Update();

    std::stringstream ss_pedtime;
    ss_pedtime<<outpath<<"PMT_2D_PedTime.jpg";
    canvas_pedtime->SaveAs(ss_pedtime.str().c_str());
    if (verbosity >=2) std::cout <<"Output path Pedestal Time Evolution plot: "<<ss_pedtime.str()<<std::endl;

    //
    //draw sigma time evolution plot
    //

    TPad *p_sigmatime = (TPad*) canvas_sigmatime->cd();
    h2D_sigmatime->SetStats(0);
    h2D_sigmatime->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_sigmatime->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p_sigmatime->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_sigmatime->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_sigmatime->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_sigmatime->LabelsOption("v");
    h2D_sigmatime->Draw("colz");

    for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
      vector_box_inactive.at(i_box)->Draw("same");
    }

    line1->Draw("same");
    line2->Draw("same");
    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();
    TLatex *label_sigmatime = new TLatex(0.905,0.92,"#Delta #sigma_{Ped}");
    label_sigmatime->SetNDC(1);
    label_sigmatime->SetTextSize(0.030);
    label_sigmatime->Draw();

    p_sigmatime->Update();

    if (h2D_sigmatime->GetMaximum()>0.){
      if (abs(max_sigmatime-min_sigmatime)==0) h2D_sigmatime->GetZaxis()->SetRangeUser(min_sigmatime-1,max_sigmatime+1);
      else h2D_sigmatime->GetZaxis()->SetRangeUser(min_sigmatime-0.5,max_sigmatime+0.5);
      TPaletteAxis *palette = 
      (TPaletteAxis*)h2D_sigmatime->GetListOfFunctions()->FindObject("palette");
      palette->SetX1NDC(0.9);
      palette->SetX2NDC(0.92);
      palette->SetY1NDC(0.1);
      palette->SetY2NDC(0.9);
    }

    std::stringstream ss_sigmatime;
    ss_sigmatime<<outpath<<"PMT_2D_SigmaTime.jpg";
    canvas_sigmatime->SaveAs(ss_sigmatime.str().c_str());
    if (verbosity >=2) std::cout <<"Output path Pedestal Sigma Time Evolution plot: "<<ss_sigmatime.str()<<std::endl;

    //
    //Draw short time evolution plots
    //

    //
    //draw pedestal time evolution plot (short)
    //

    TPad *p_pedtime_short = (TPad*) canvas_pedtime_short->cd();
    h2D_pedtime_short->SetStats(0);
    h2D_pedtime_short->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_pedtime_short->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p_pedtime_short->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_pedtime_short->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_pedtime_short->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_pedtime_short->LabelsOption("v");
    h2D_pedtime_short->Draw("colz");

    for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
      vector_box_inactive.at(i_box)->Draw("same");
    }

    line1->Draw("same");
    line2->Draw("same");
    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();
    TLatex *label_pedtime_short = new TLatex(0.905,0.92,"#Delta #mu_{Ped}");
    label_pedtime_short->SetNDC(1);
    label_pedtime_short->SetTextSize(0.030);
    label_pedtime_short->Draw();
    p_pedtime_short->Update();

    if (h2D_pedtime_short->GetMaximum()>0.){
      if (abs(max_pedtime_short-min_pedtime_short)==0) h2D_pedtime_short->GetZaxis()->SetRangeUser(min_pedtime_short-1,max_pedtime_short+1);
      else h2D_pedtime_short->GetZaxis()->SetRangeUser(min_pedtime_short-0.5,max_pedtime_short+0.5);
      TPaletteAxis *palette = 
      (TPaletteAxis*)h2D_pedtime->GetListOfFunctions()->FindObject("palette");
      palette->SetX1NDC(0.9);
      palette->SetX2NDC(0.92);
      palette->SetY1NDC(0.1);
      palette->SetY2NDC(0.9);
    }
    p_pedtime_short->Update();

    std::stringstream ss_pedtime_short;
    ss_pedtime_short<<outpath<<"PMT_2D_PedTime_Short.jpg";
    canvas_pedtime_short->SaveAs(ss_pedtime_short.str().c_str());
    if (verbosity >=2) std::cout <<"Output path Pedestal Time Evolution plot (short=: "<<ss_pedtime_short.str()<<std::endl;

    //
    //draw sigma time evolution plot (short)
    //

    TPad *p_sigmatime_short = (TPad*) canvas_sigmatime_short->cd();
    h2D_sigmatime_short->SetStats(0);
    h2D_sigmatime_short->GetXaxis()->SetNdivisions(num_slots_tank);
    h2D_sigmatime_short->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
    p_sigmatime_short->SetGrid();
    for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
      std::stringstream ss_slot;
      ss_slot<<(i_label+1);
      std::string str_slot = "slot "+ss_slot.str();
      h2D_sigmatime_short->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
    for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
      std::stringstream ss_ch;
      if (i_label < 4) ss_ch<<((3-i_label)%4);
      else if (i_label < 8) ss_ch<<((7-i_label)%4);
      else ss_ch<<((11-i_label)%4);
      std::string str_ch = "ch "+ss_ch.str();
      h2D_sigmatime_short->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    }
    h2D_sigmatime_short->LabelsOption("v");
    h2D_sigmatime_short->Draw("colz");

    for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
      vector_box_inactive.at(i_box)->Draw("same");
    }

    line1->Draw("same");
    line2->Draw("same");
    text_crate1->Draw();
    text_crate2->Draw();
    text_crate3->Draw();
    TLatex *label_sigmatime_short = new TLatex(0.905,0.92,"#Delta #sigma_{Ped}");
    label_sigmatime_short->SetNDC(1);
    label_sigmatime_short->SetTextSize(0.030);
    label_sigmatime_short->Draw();
    p_sigmatime_short->Update();

    if (h2D_sigmatime_short->GetMaximum()>0.){
      if (abs(max_sigmatime_short-min_sigmatime_short)==0) h2D_sigmatime_short->GetZaxis()->SetRangeUser(min_sigmatime_short-1,max_sigmatime_short+1);
      else h2D_sigmatime_short->GetZaxis()->SetRangeUser(min_sigmatime_short-0.5,max_sigmatime_short+0.5);
      TPaletteAxis *palette = 
      (TPaletteAxis*)h2D_sigmatime_short->GetListOfFunctions()->FindObject("palette");
      palette->SetX1NDC(0.9);
      palette->SetX2NDC(0.92);
      palette->SetY1NDC(0.1);
      palette->SetY2NDC(0.9);
    }

    std::stringstream ss_sigmatime_short;
    ss_sigmatime_short<<outpath<<"PMT_2D_SigmaTime_Short.jpg";
    canvas_sigmatime_short->SaveAs(ss_sigmatime_short.str().c_str());
    if (verbosity >=2) std::cout <<"Output path Pedestal Sigma Time Evolution plot (short): "<<ss_sigmatime_short.str()<<std::endl;

    //
    //End of short time evolution plots
    //

    //
    //Draw single slot diagrams
    //

    for (int i_slot = 0; i_slot<num_active_slots; i_slot++){

      int max_freq = 0;
      double max_temp = -999999.;
      double min_temp = 999999.;

      int slot_num, crate_num;
      if (i_slot < n_active_slots_cr1){
        crate_num = crate_numbers.at(0);
        slot_num = active_slots_cr1.at(i_slot);
      }
      else if (i_slot >= n_active_slots_cr1 && i_slot < n_active_slots_cr1+n_active_slots_cr2){
        crate_num = crate_numbers.at(1);
        slot_num = active_slots_cr2.at(i_slot-n_active_slots_cr1);
      }
      else {
        crate_num = crate_numbers.at(2);
        slot_num = active_slots_cr3.at(i_slot-n_active_slots_cr1-n_active_slots_cr2);
      }

      canvas_Channels_temp.at(i_slot)->cd();
      TLegend *leg = new TLegend(0.75,0.7,0.9,0.9);
      for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
        std::stringstream ss_channel;
        ss_channel << "Cr "<<crate_num<<"/Sl"<<slot_num<<"/Ch"<<i_channel;
        hChannels_temp.at(i_slot*num_channels_tank+i_channel)->Draw("same");
        if (hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMaximum() > max_temp) max_temp = hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMaximum();
        if (hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMinimum() < min_temp) min_temp = hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMinimum();
        leg->AddEntry(hChannels_temp.at(i_slot*num_channels_tank+i_channel),ss_channel.str().c_str(),"l");
      }
      hChannels_temp.at(i_slot*num_channels_tank)->GetYaxis()->SetRangeUser(min_temp,max_temp);
      leg->Draw();
      std::stringstream ss_canvas_temp;
      ss_canvas_temp << outpath << "PMT_Temp_Cr"<<crate_num<<"_Sl"<<slot_num<<".jpg";
      canvas_Channels_temp.at(i_slot)->SaveAs(ss_canvas_temp.str().c_str());
      delete leg;

      canvas_Channels_freq.at(i_slot)->cd();
      TLegend *leg_freq = new TLegend(0.75,0.7,0.9,0.9);
      for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
        std::stringstream ss_channel;
        ss_channel << "Cr "<<crate_num<<"/Sl"<<slot_num<<"/Ch"<<i_channel;
        hChannels_freq.at(i_slot*num_channels_tank+i_channel)->Draw("same");
        if (hChannels_freq.at(i_slot*num_channels_tank+i_channel)->GetMaximum() > max_freq) max_freq = hChannels_freq.at(i_slot*num_channels_tank+i_channel)->GetMaximum();
        leg_freq->AddEntry(hChannels_freq.at(i_slot*num_channels_tank+i_channel),ss_channel.str().c_str());
      }
      hChannels_freq.at(i_slot*num_channels_tank)->GetYaxis()->SetRangeUser(0.,max_freq);
      leg_freq->Draw();
      std::stringstream ss_canvas_freq;
      ss_canvas_freq << outpath << "PMT_Freq_Cr"<<crate_num<<"_Sl"<<slot_num<<".jpg";
      canvas_Channels_freq.at(i_slot)->SaveAs(ss_canvas_freq.str().c_str());
      delete leg_freq;
    }

  //std::cout <<"Deleting gaussian fit functions"<<std::endl;
  for (int i_gaus=0; i_gaus < vector_gaus.size(); i_gaus++){
    delete vector_gaus.at(i_gaus);
  }
  vector_gaus.clear();

  //std::cout <<"Resetting & Clearing Channel histograms, canvasses"<<std::endl;
  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      hChannels_freq.at(i_slot*num_channels_tank+i_channel)->Reset();
      hChannels_temp.at(i_slot*num_channels_tank+i_channel)->Reset();
    }
  }

  //std::cout <<"Resetting hists"<<std::endl;
  h2D_rate->Reset();
  h2D_ped->Reset();
  h2D_sigma->Reset();
  h2D_pedtime->Reset();
  h2D_sigmatime->Reset();
  h2D_pedtime_short->Reset();
  h2D_sigmatime_short->Reset();

  //std::cout <<"Deleting Boxes"<<std::endl;
  for (int i_box = 0; i_box < vector_box_inactive.size(); i_box++){
    delete vector_box_inactive.at(i_box);
  }

  //std::cout <<"Clearing Canvasses"<<std::endl;
  canvas_rate->Clear();
  canvas_ped->Clear();
  canvas_sigma->Clear();
  canvas_pedtime->Clear();
  canvas_sigmatime->Clear();
  canvas_pedtime_short->Clear();
  canvas_sigmatime_short->Clear();

  //std::cout <<"Deleting labels, lines, texts"<<std::endl;
  delete label_mean;
  delete label_sigma;
  delete label_rate;
  delete label_sigmatime;
  delete label_pedtime;
  delete label_sigmatime_short;
  delete label_pedtime_short;
  delete line1;
  delete line2;
  delete text_crate1;
  delete text_crate2;
  delete text_crate3;

}

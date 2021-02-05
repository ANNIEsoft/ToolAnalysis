#include "MaxPEPlots.h"

MaxPEPlots::MaxPEPlots():Tool(){}


bool MaxPEPlots::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("MaxPEFilename",maxpe_filename);
  m_variables.Get("verbosity",verbosity);


  f_maxpe = new TFile(maxpe_filename.c_str(),"RECREATE");
  t_maxpe = new TTree("t_maxpe","Max PE");
  t_maxpe->Branch("max_pe_global",&max_pe_global);
  t_maxpe->Branch("max_pe",&max_pe);
  t_maxpe->Branch("extended",&extended); 
  t_maxpe->Branch("triggerword",&triggerword); 

  h_maxpe = new TH1F("h_maxpe","Max P.E. distribution",5000,0,1000);
  h_maxpe_prompt = new TH1F("h_maxpe_prompt","Max P.E. distribution (prompt)",5000,0,1000);
  h_maxpe_delayed = new TH1F("h_maxpe_delayed","Max P.E. distribution (extended)",5000,0,1000);
  h_maxpe_chankey = new TH2F("h_maxpe_chankey","Max P.E. distribution vs. chankey",132,332,464,500,0,500);  
  h_maxpe_prompt_chankey = new TH2F("h_maxpe_prompt_chankey","Max P.E. distribution (prompt) vs. chankey",132,332,464,500,0,500);  
  h_maxpe_delayed_chankey = new TH2F("h_maxpe_delayed_chankey","Max P.E. distribution (delayed) vs. chankey",132,332,464,500,0,500);  
  h_maxpe_trigword5 = new TH1F("h_maxpe_trigword5","Max P.E. distribution (Trgword 5)",5000,0,1000);
  h_maxpe_prompt_trigword5 = new TH1F("h_maxpe_prompt_trigword5","Max P.E. distribution (prompt, Trgword 5)",5000,0,1000);
  h_maxpe_delayed_trigword5 = new TH1F("h_maxpe_delayed_trigword5","Max P.E. distribution (extended, Trgword 5)",5000,0,1000);
  h_maxpe_chankey_trigword5 = new TH2F("h_maxpe_chankey_trigword5","Max P.E. distribution vs. chankey (Trgword 5)",132,332,464,500,0,500);  
  h_maxpe_prompt_chankey_trigword5 = new TH2F("h_maxpe_prompt_chankey_trigword5","Max P.E. distribution (prompt) vs. chankey (Trgword 5)",132,332,464,500,0,500);  
  h_maxpe_delayed_chankey_trigword5 = new TH2F("h_maxpe_delayed_chankey_trigword5","Max P.E. distribution (delayed) vs. chankey (Trgword 5)",132,332,464,500,0,500);  

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  std::map<std::string, std::map<unsigned long,Detector*>>* Detectors = geom->GetDetectors();

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin(); it != Detectors->at("Tank").end(); ++it){

    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    pmt_detkeys.push_back(detkey);
    PMT_maxpe.insert(std::pair<unsigned long, double>(detkey,0.));
  }

  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelNumToTankPMTSPEChargeMap);

 /*
  std::vector<TH1F*> h_maxpe_prompt;
  std::vector<TH1F*> h_maxpe_delayed;
*/
  return true;
}


bool MaxPEPlots::Execute(){

  max_pe_global = 0;
  max_pe.clear();

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl;}

  //Get Data Streams for event
  bool got_datastreams = m_data->Stores["ANNIEEvent"]->Get("DataStreams",DataStreams);
  if (!got_datastreams){
    std::cout <<"No DataStreams in ANNIEEvent store!"<<std::endl;
    return false;
  }
  if (DataStreams["Tank"]==0) return true;	//Don't do anything if no tank hits

  std::map<unsigned long, std::vector<Waveform<unsigned short>>> raw_waveform_map;
  bool has_raw = m_data->Stores["ANNIEEvent"]->Get("RawADCData",raw_waveform_map);
  std::cout <<"has_raw: "<<has_raw<<std::endl;
  if (!has_raw) {
    Log("RunValidation tool: Did not find RawADCData in ANNIEEvent! Abort",v_error,verbosity);
    /*return false;*/
  }
  extended = false;
  int size_of_window = 2000;
  if (has_raw){
    for (auto& temp_pair : raw_waveform_map) {
      const auto& achannel_key = temp_pair.first;
      auto& araw_waveforms = temp_pair.second;
      for (unsigned int i=0; i< araw_waveforms.size(); i++){
        auto samples = araw_waveforms.at(i).GetSamples();
        int size_sample = 2*samples->size();
        if (size_sample > size_of_window) {
          size_of_window = size_sample;
          extended = true;
        }
      }
    }
  }

  bool got_trigword = m_data->Stores["ANNIEEvent"]->Get("TriggerWord",triggerword);
  if (!got_trigword){
    std::cout <<"No trgword in ANNIEEvent!" << std::endl;
  }

  bool got_evnum = m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  if (!got_evnum){
    std::cout <<"No EventNumber in ANNIEEvent!" << std::endl;
  }

  bool got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
  if (!got_hits){
    std::cout << "No Hits store in ANNIEEvent! " << std::endl;
    return false;
  }


  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    PMT_maxpe[detkey] = 0;
  }

  std::cout <<"extended: "<<extended<<std::endl;

  int vectsize = Hits->size();
  for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
    unsigned long chankey = apair.first;
    Detector* thistube = geom->ChannelToDetector(chankey);
    int detectorkey = thistube->GetDetectorID();
    if (thistube->GetDetectorElement()=="Tank"){
      std::vector<Hit>& ThisPMTHits = apair.second;
      for (Hit &ahit : ThisPMTHits){
        double thistime = ahit.GetTime();
        if (thistime > 2000) continue;
        double thischarge = ahit.GetCharge();
        double thischarge_pe = thischarge/ChannelNumToTankPMTSPEChargeMap->at(detectorkey);
        if (thischarge_pe > max_pe_global) max_pe_global = thischarge_pe;
        if (thischarge_pe > PMT_maxpe[detectorkey]) PMT_maxpe[detectorkey] = thischarge_pe;
        if (verbosity > 2) std::cout << "Key: " << detectorkey << ", charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
      }
    }
  }

  h_maxpe->Fill(max_pe_global);
  if (!extended) h_maxpe_prompt->Fill(max_pe_global);
  else h_maxpe_delayed->Fill(max_pe_global);


  if (triggerword == 5){
    h_maxpe_trigword5->Fill(max_pe_global);
    if (!extended) h_maxpe_prompt_trigword5->Fill(max_pe_global);
    else h_maxpe_delayed_trigword5->Fill(max_pe_global);
  }

  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    h_maxpe_chankey->Fill(detkey,PMT_maxpe[detkey]);
    if (!extended) h_maxpe_prompt_chankey->Fill(detkey,PMT_maxpe[detkey]);
    else h_maxpe_delayed_chankey->Fill(detkey,PMT_maxpe[detkey]);
    max_pe.push_back(PMT_maxpe[detkey]);
    if (triggerword == 5){
      h_maxpe_chankey_trigword5->Fill(detkey,PMT_maxpe[detkey]);
      if (!extended) h_maxpe_prompt_chankey_trigword5->Fill(detkey,PMT_maxpe[detkey]);
      else h_maxpe_delayed_chankey_trigword5->Fill(detkey,PMT_maxpe[detkey]);
    }
  }

  t_maxpe->Fill();

  return true;
}


bool MaxPEPlots::Finalise(){

  f_maxpe->Write();
  f_maxpe->Close();
  delete f_maxpe;

  return true;
}

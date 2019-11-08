#include "PrintADCData.h"

PrintADCData::PrintADCData():Tool(){}


bool PrintADCData::Initialise(std::string configfile, DataModel &data){

  std::cout << "PrintADCData: tool initializing" << std::endl;
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  visualize = false;

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("VisualizeADCData",visualize);
  if(annieeventexists==0) {
    std::cout << "PrintADCData: No ANNIE Event in store to print!" << std::endl;
    return false;
  }

  EntryNum = 0;
  std::cout << "PrintADCData: tool initialized" << std::endl;
  return true;
}


bool PrintADCData::Execute(){
  //Just print the whole dang thing
  if(verbosity>2) std::cout << "PrintADCData: getting total entries from header" << std::endl;
  m_data->Stores["ANNIEEvent"]->Header->Get("TotalEntries",totalentries);
  if(verbosity>2) std::cout << "PrintADCData: Number of ANNIEEvent entries: " << totalentries << std::endl;
  if(verbosity>2) std::cout << "PrintADCData: looping through entries" << std::endl;
  m_data->Stores["ANNIEEvent"]->Get("RawADCData",RawADCData);
  //m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNum);
  //m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNum);
  //
  //std::cout << "Run number for entry is " << RunNum << std::endl;
  //std::cout << "Subrun number for entry is " << SubrunNum << std::endl;
  std::cout << "Num. of PMT signals for entry: " << RawADCData.size() << std::endl;

  if ( RawADCData.empty() ) {
    Log("PrintADCData Error: Found an empty RawADCData entry in event", 0,
      verbosity);
  }
  else { 
    for (const auto& temp_pair : RawADCData) {
      const auto& channel_key = temp_pair.first;
      std::cout << "Waveform info for channel " << channel_key << std::endl;
      //Default running: raw_waveforms only has one entry.  If we go to a
      //hefty-mode style of running though, this could have multiple minibuffers
      //const std::vector< Waveform<unsigned short> > raw_waveforms;
      //raw_waveforms = temp_pair.second;
      const auto& raw_waveforms = temp_pair.second;
      //Length of waveform vector should be 1 in normal mode, but greater than
      //1 in a hefty-like format (multiple waveform buffers)
        std::cout << "Waveform size: " << raw_waveforms.size() << std::endl;
      for (int j=0; j < raw_waveforms.size(); j++){
        std::cout << "Printing waveform info for buffer at index " << j << std::endl;
        Waveform<unsigned short> awaveform = raw_waveforms.at(j);
        std::cout << "Waveform start time: " << std::setprecision(16) << awaveform.GetStartTime() << std::endl;
        std::vector<unsigned short>* thewave=awaveform.GetSamples();
        if(verbosity>4){
          std::cout << "BEGIN SAMPLES" << std::endl;
          for (int i=0; i < thewave->size(); i++){
            std::cout << thewave->at(i) << std::endl;
          }
        }
      }
    }
    // Method directly taken from Marcus' SimulatedWaveformDemo 
	// make a numberline to act as the x-axis of the plotting TGraph
    if(visualize){
      Log("SimulatedWaveformDemo Tool: Looping over "+to_string(RawADCData.size())
      	 +" Tank PMT channels",v_debug,verbosity);
      for(std::pair<const unsigned long,std::vector<Waveform<uint16_t>>>& achannel : RawADCData){
        const unsigned long channelkey = achannel.first;
        
        // Each Waveform represents one minibuffer on this channel
        Log("SimulatedWaveformDemo Tool: Looping over "+to_string(achannel.second.size())
             +" minibuffers",v_debug,verbosity);
        for(Waveform<uint16_t>& wfrm : achannel.second){
          std::vector<uint16_t>* samples = wfrm.GetSamples();
          int SampleLength = samples->size();
          numberline.resize(SampleLength);
          std::iota(numberline.begin(),numberline.end(),0);
          upcastdata.resize(SampleLength);
          // Note that because these are built directly from the simulated minibuffers,
          // samples will have the Phase 1 interleaving unless it is disabled in the
          // PulseSimulation tool by passing config variable DoPhaseOneRiffle=0
          
          // As a demonstration, let's just plot the waveform on a TGraph
          const uint16_t thismax = *std::max_element(samples->begin(), samples->end());
          Log("SimulatedWaveformDemo Tool: checking max "+to_string(thismax)+" against "
            +to_string(maxwfrmamp),v_debug,verbosity);
          if(thismax<maxwfrmamp){ continue; }
          maxwfrmamp = thismax;
          
          // for plotting on a TGraph we need to up-cast the data from uint16_t to int32_t
          Log("SimulatedWaveformDemo Tool: Making TGraph",v_debug,verbosity);
          for(int samplei=0; samplei<SampleLength; samplei++){
              upcastdata.at(samplei) = samples->at(samplei);
          }
          if(mb_graph){ delete mb_graph; }
          mb_graph = new TGraph(SampleLength, numberline.data(), upcastdata.data());
          mb_graph->SetName("mb_graph");
          if(gROOT->FindObject("mb_canv")==nullptr) mb_canv = new TCanvas("mb_canv");
          mb_canv->cd();
          mb_canv->Clear();
          mb_graph->Draw("alp");
          mb_canv->Modified();
          mb_canv->Update();
          gSystem->ProcessEvents();
          do{
            gSystem->ProcessEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          } while (gROOT->FindObject("mb_canv")!=nullptr); // wait until user closes canvas
          Log("SimulatedWaveformDemo Tool: graph closed, looping",v_debug,verbosity);
        } // end loop over minibuffers
      } // end loop over channelkeys
    }
  }
  
  return true;
}


bool PrintADCData::Finalise(){
  std::cout << "PrintADCData exitting" << std::endl;
  return true;
}

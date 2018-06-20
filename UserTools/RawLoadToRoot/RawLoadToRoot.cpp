#include "RawLoadToRoot.h"
#include "ChannelKey.h"

RawLoadToRoot::RawLoadToRoot():Tool(){}


bool RawLoadToRoot::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("treeoutfile", treeoutfile);
  m_variables.Get("lowerbound", lbound);
  m_variables.Get("upperbound", ubound);
  m_variables.Get("onoffswitch", onoffswitch);

  treeoutput = new TFile(treeoutfile, "RECREATE");
  treeoutput->mkdir("waveforms");

  rawtotree = new TTree("RawLoadTree", "RawLoadTree");

  rawtotree->Branch("EventNumber", &EventNumber);
  rawtotree->Branch("RunNumber", &RunNumber);
  rawtotree->Branch("SubRunNumber", &SubRunNumber);

  RelEventNumber = 1;

  return true;
}


bool RawLoadToRoot::Execute(){

  auto* annieevent = m_data->Stores["ANNIEEvent"];

  annieevent->Get("EventNumber", EventNumber);
  annieevent->Get("RunNumber", RunNumber);
  annieevent->Get("SubRunNumber", SubRunNumber);
  annieevent->Get("RawADCData", RawADCData);
  //annieevent->Get("MinibufferTimestamps", MinibufferTimestamps);
  //annieevent->Get("HeftyInfo", HeftyInfo);

  //if statement restricts tool to range of trigger events specified in config file
  if((lbound<=RelEventNumber && RelEventNumber<=ubound && onoffswitch==1)||onoffswitch==0){

    map<ChannelKey,vector<Waveform<unsigned short>>> :: iterator itr;
    int mycounter=0;
    int chancount=0;
    for(itr=RawADCData.begin(); itr!=RawADCData.end(); ++itr){

      ChannelKey ck = itr->first;
      //std::cout<<ck.GetDetectorElementIndex()<<std::endl;
      vector<Waveform<unsigned short>> TheWaveforms = itr->second;

      std::cout<<"Waveform vector size: "<<TheWaveforms.size()<<std::endl;
      for(int mmm=0; mmm<TheWaveforms.size(); mmm++){

        TString wname;
        wname+="Waveform_CH";
        wname+=chancount;
        wname+="_wf";
        wname+=mmm;
        wname+="_eventnum";
        wname+=40*EventNumber+mmm;
        theWaveformHist = new TH1D(wname,wname,1000,0,2000);
        Waveform<unsigned short> aWaveform = TheWaveforms.at(mmm);
        for(int nnn=0; nnn<aWaveform.Samples().size(); nnn++){
          theWaveformHist->SetBinContent(nnn+1,(double)aWaveform.GetSample(nnn));
        }

        treeoutput->cd("waveforms");
        theWaveformHist->Write();
        mycounter++;
        delete theWaveformHist;
      }
      chancount++;
//      delete theWaveformHist;

    }

    //std::cout<<"Waveform Created for"<<std::endl;

    rawtotree->Fill();

  }else{

  }

  //std::cout<<"Relative Event Number: "<<RelEventNumber<<std::endl;

  RelEventNumber++;

  return true;
}

bool RawLoadToRoot::Finalise(){

  treeoutput->cd();
  rawtotree->Write();
  treeoutput->Close();

  return true;
}

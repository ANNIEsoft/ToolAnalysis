#include "HistogramsRootLAPPDData.h"

HistogramsRootLAPPDData::HistogramsRootLAPPDData():Tool(){}


bool HistogramsRootLAPPDData::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // setup the output files
  TString OutFile = "lappdout.root";	 //default input file
  m_variables.Get("outfile", OutFile);
  tff = new TFile(OutFile,"RECREATE");
  miter=0;
  // initialize the histograms
   HitMultiplicity = new TH1D("HitMultiplicity","HitMultiplicity",1000,0,200);

   DataTree = new TTree("DataTree","DataTree");
   DataTree->Branch("Xcoord", &xpos);
   DataTree->Branch("Ycoord", &ypos);
   DataTree->Branch("Zcoord", &zpos);
   DataTree->Branch("Time", &thehittime);
   DataTree->Branch("LAPPDID", &LAPPDnumber);
   AmpTree = new TTree("AmpTree", "AmpTree");
   AmpTree-> Branch("Amp",&amp);
 
  get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",anniegeom);
  if(not get_ok){
    cerr<<"HistogramsRootLAPPDData Tool: Could not get AnnieGeometry from ANNIEEvent!"<<endl;
    return false;
  }
  get_ok = m_data->CStore.Get("detectorkey_to_lappdid",detectorkey_to_lappdid);
  if(not get_ok){
    cerr<<"HistogramsRootLAPPDData Tool: Could not get detectorkey_to_lappdid from ANNIEEvent!"<<endl;
    return false;
  }

  return true;
}


  bool HistogramsRootLAPPDData::Execute()
  {
    
    
    
    LAPPDresponse response;
    std::map<unsigned long,vector<LAPPDHit>> mclappdhits;
    m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",mclappdhits);
    std::map<int,vector<Waveform<double>>> rawlappddata;
    m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData" ,rawlappddata);
    std::map<int, vector<LAPPDPulse>> SimpleRecoLAPPDPulses;
    m_data->Stores["ANNIEEvent"]->Get("SimpleRecoLAPPDPulses", SimpleRecoLAPPDPulses);
    
    
    
    std::map<unsigned long, vector<LAPPDHit>> :: iterator itrr;
    for (itrr =mclappdhits.begin(); itrr != mclappdhits.end(); ++itrr)
      {
	
	unsigned long stripkey = itrr->first;
	Detector* det = anniegeom->ChannelToDetector(stripkey);
	if(det==nullptr){
		cerr<<"HistogramsRootLAPPDData Tool: LAPPD Detector not found! "<<endl;
		continue;
	}
	int detkey = det->GetDetectorID();
	int lappdnumber = detectorkey_to_lappdid.at(detkey);  // FIXME should transition to using detkey
	//Watch yo back
	vector<LAPPDHit> hitlist = itrr->second;
	double hitcount =0;
	TString eventnum;  
	eventnum +="Event";
	eventnum +=miter;
	eventnum +="LAPPD";
	eventnum +=lappdnumber;
	TH2D*  TimeToStrip = new TH2D("HSHM"+eventnum,"HSHM"+eventnum,50,0.5,1000.5,50,0.5,30.5);
	if((lappdnumber==26)||(lappdnumber==51)||(lappdnumber==56)||(lappdnumber==86)||(lappdnumber==91))
	  {
	    for(int i=0; i<hitlist.size(); i++)
	      {
		hitcount++;  
		xpos= hitlist.at(i).GetPosition().at(0);
		ypos= hitlist.at(i).GetPosition().at(1);
		zpos= hitlist.at(i).GetPosition().at(2);
		LAPPDnumber = lappdnumber;
		int stripint = response.FindStripNumber(hitlist.at(i).GetLocalPosition().at(1));
		double strip = stripint;
		double tpsec =hitlist.at(i).GetTpsec();
		thehittime= tpsec;
		
		TimeToStrip->Fill(tpsec,strip);
		DataTree->Fill();
		
	      }
	    HitMultiplicity->Fill(hitcount);
	    
	    if(miter<20)
	      {
		tff->cd();
		TimeToStrip->Write();
	      }
	  }
      }
    TH2D *VoltageMapL = new TH2D("VoltageMapL","VoltageMapL",256,0.5,25600.5,30,0.5,30.5);
    TH2D *VoltageMapR = new TH2D("VoltageMapR","VoltageMapR",256,0.5,25600.5,30,0.5,30.5);
    std::map<int, vector<Waveform<double>>> :: iterator itr;
    for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr)
      {
	//std::cout<<rawlappddata.size()<<endl;
	int channelno = itr->first;
	vector<Waveform<double>> wavs = itr->second;
	TString eventnumb;  
	eventnumb+= "Event";
	eventnumb+= miter;
	eventnumb+= "LAPPD";
	eventnumb+=channelno/60;
	if((channelno/60 ==26)||(channelno/60 ==51)||(channelno/60 ==56)||(channelno/60 ==86)||(channelno/60 ==91))
	  {  
	    // std::cout<<wavs.size()<<endl;
	    for(int j=0; j<wavs.size(); j++)
	      {
		double strip =double(channelno%60);	
		for(int k=0; k<wavs.at(j).GetSamples()->size(); k++)
		  {
		    strip = double (channelno%60);
		    double wavetime = double (k*100.);
		    double sample = wavs.at(j).GetSamples()->at(k);
		    if(strip<=29.)
		      {

			double stripl= double (30- channelno%60);
			VoltageMapL->Fill(wavetime,stripl,-sample);
		      }
		    if(strip>29.)
		      {
			double stripr =double (channelno%30)+1;
			VoltageMapR->Fill(wavetime,stripr,-sample);
		      }
		    
		  }
	      }
	    if(channelno%60==59)
	      {
		if(miter<20)
		  {
		    VoltageMapL->SetNameTitle(eventnumb+"L",eventnumb+"L");
		    VoltageMapR->SetNameTitle(eventnumb+"R",eventnumb+"R");
		    VoltageMapL->Write();
		    VoltageMapR->Write();
		  }
		VoltageMapL->Reset();
		VoltageMapR->Reset(); 
	      }
	  }
	map<int, vector<LAPPDPulse>>::iterator p;
	p= SimpleRecoLAPPDPulses.find(channelno);
	vector<LAPPDPulse> Pulses = p->second;
	for(int l=0; l<Pulses.size(); l++)
	  {
	    LAPPDPulse thepulse = Pulses.at(l);
	    amp = thepulse.GetPeak();
	    AmpTree->Fill();
	  }
      }
    miter++;
    delete VoltageMapL;
    delete VoltageMapR;
    return true;
  }


bool HistogramsRootLAPPDData::Finalise(){

  // go to the top level of the output file
  tff->cd();
  HitMultiplicity->Write();
  DataTree-> Write();
  AmpTree-> Write();
  tff->Close();

  
  
  return true;
}

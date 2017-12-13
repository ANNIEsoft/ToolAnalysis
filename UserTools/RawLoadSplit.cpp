#include "RawLoadSplit.h"

RawLoadSplit::RawLoadSplit():Tool(){}


bool RawLoadSplit::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////

  if(configfile!="")  m_variables.Initialise(configfile); ///loading config file
  //m_variables.Print();  //optional print out

  m_data= &data; //// assigning transient data pointer

  /////////////////////////////////////////////////////////////////

  m_variables.Get("Debug",m_debug); //// assigning debug level


  std::string filelist;
  m_variables.Get("FileList",filelist); // assigning file list

  PMTDataChain=new TChain("PMTData");
  RunInformationChain=new TChain("RunInformation");
  MRDChain= new TChain("CCData");
  
  std::string line;
  ifstream myfile (filelist.c_str());
  
  if (myfile.is_open()){
    
    // m_data->outfile="";
    
    while ( getline (myfile,line) ){
      
/*
      if(m_data->outfile==""){
	char * pch;
	char *elements=new char[line.length()];
	strcpy(elements, line.c_str());
	pch = strtok (elements,"/");
	while (pch != NULL)
	  {
	    m_data->outfile=pch;
	    pch = strtok(NULL,"/");
	  }
      }
*/     
      std::cout<<"Loading file "<<line<<std::endl;	
      PMTDataChain->Add(line.c_str());
      RunInformationChain->Add(line.c_str());
      MRDChain->Add(line.c_str());
      
    }
    myfile.close();
  }
  else return false;
  
  BoostStore PersistantStore;
  std::map<std::string,BoostStore> EntryStores;
  
  WaterPMTData=new PMTData(PMTDataChain);
  RunInformationData=new RunInformation(RunInformationChain);
  MRDData=new MRDTree(MRDChain);  
  MRDData->fChain->SetName("MRDData");


  /////Finding trigger info from configuration files
  long entries=RunInformationData->fChain->GetEntries();
  Store triggerinfo;
  for(long entry=0; entry<entries;entry++){

    RunInformationData->GetEntry(entry);

    if(*(RunInformationData->InfoTitle)=="TriggerVariables"){

      triggerinfo.JsonParser(*(RunInformationData->InfoMessage));

    }
  }

  ////////


  ///// determining downsample level and number of samples per buffer

  int nsamples=0;
  int bufferlength=0;
  int samplelength=0;

  triggerinfo.Get("TriggerSamples",samplelength);
  triggerinfo.Get("TotalSamples",bufferlength);
  samplelength*=4;
  bufferlength*=4;

  nsamples=bufferlength/samplelength;
  ////

  currententry=0;
  currentsequenceid=-1;
   
  m_data->EntryStores["WaterPMTData"]=new BoostStore(false,2);
  m_data->EntryStores["WaterPMTData"]->Set("RawLoadSplitConfig",m_variables); //header info

  return true;
}


bool RawLoadSplit::Execute(){

  if(currententry<WaterPMTData->fChain->GetEntries()){
  WaterPMTData->GetEntry(currententry);
  while(currentsequenceid!=WaterPMTData->SequenceID) currentsequenceid++;
  
  annie::RawReadout tmp;
  tmp.set_sequence_id(WaterPMTData->SequenceID);
  
  while(WaterPMTData->SequenceID == currentsequenceid){
    
    tmp.add_card(WaterPMTData->CardID, WaterPMTData->LastSync, WaterPMTData->StartTimeSec,
		 WaterPMTData->StartTimeNSec, WaterPMTData->StartCount, WaterPMTData->Channels,
		 WaterPMTData->BufferSize, WaterPMTData->Eventsize * 4,
		 WaterPMTData->Data, WaterPMTData->TriggerCounts, WaterPMTData->Rates);
    
    currententry++;
    WaterPMTData->GetEntry(currententry);
    
  }  

  std::stringstream entryname;
  entryname<<currentsequenceid;
  m_data->EntryStores["WaterPMTData"]->Set(entryname.str().c_str(), tmp);

  }
  else m_data->vars.Set("StopLoop",1);


  return true;
}


bool RawLoadSplit::Finalise(){

  return true;
}

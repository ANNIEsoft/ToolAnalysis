#include "MRDDataDecoder.h"

MRDDataDecoder::MRDDataDecoder():Tool(){}


bool MRDDataDecoder::Initialise(std::string configfile, DataModel &data){
  std::cout << "Initializing MRDDataDecoder tool" << std::endl;
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;
  EntriesPerExecute = 0;
  DummyRunNumber = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("Mode",Mode);
  m_variables.Get("InputFile",InputFile);
  m_variables.Get("EntriesPerExecute",EntriesPerExecute);
  m_variables.Get("UseDummyRunNumber",DummyRunNumber);

  //Default mode of operation is the continuous flow of data for the live monitoring
  //The other possibility is reading in data from a specified list of files
  if (Mode != "Continuous" && Mode != "SingleFile" && Mode != "FileList") {
    if (verbosity > 0) std::cout <<"ERROR (MRDDataDecoder): Specified mode of operation ("<<Mode<<") is not an option [Continuous/SingleFile/FileList]. Setting default Continuous mode"<<std::endl;
    Mode = "Continuous";
  }
  //When operating in Continous mode, always scan the whole file (all entries)
  if (Mode == "Continuous"){
    EntriesPerExecute = -1;
  }
 
  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",MRDCrateSpaceToChannelNumMap);
 
  FileNum = 0;
  EntryNum = 0;
  RawData = new BoostStore(false,0);
  MRDData = new BoostStore(false,2);
  m_data->CStore.Set("NewMRDDataAvailable",false);

  if(Mode=="FileList"){
    if(verbosity>v_warning){
      std::cout << "MRDDataDecoder tool: Running in file list mode. " <<
          "Files must be from same run and have sequential parts." << std::endl;
      std::cout << "MRDDataDecoder tool: Organizing file list by part." << std::endl;
    }
    OrganizedFileList = this->OrganizeRunParts(InputFile);
    Log("MRDDataDecoder tool: files to load have been organized.",v_message,verbosity);
  }
  
  m_data->CStore.Set("PauseMRDDecoding",false);
  Log("MRDDataDecoder Tool: Initialized successfully",v_message,verbosity);
  return true;
}


bool MRDDataDecoder::Execute(){

  m_data->CStore.Set("NewMRDDataAvailable",false);
  bool PauseMRDDecoding = false;
  m_data->CStore.Get("PauseMRDDecoding",PauseMRDDecoding);
  if (PauseMRDDecoding){
    std::cout << "MRDDataDecoder tool: Pausing MRD decoding to let Tank data catch up..." << std::endl;
    return true;
  }

  //Check if we are starting a new file
  if (FileCompleted) m_data->CStore.Set("MRDFileComplete",false);

 // Load RawData and MRDData booststores
  if(Mode=="SingleFile"){
    if(FileNum > 0){
      std::cout << "MRDDataDecoder tool: Single file has already been loaded" << std::endl;
      return true;
    } else if (EntryNum == 0){
      Log("MRDDataDecoder Tool: Raw Data file as BoostStore",v_message,verbosity); 
      // Initialize RawData
      RawData->Initialise(InputFile.c_str());
      RawData->Print(false);
      RawData->Get("CCData",*MRDData);
      MRDData->Print(false);
    } else {
      Log("MRDDataDecoder Tool: Continuing Raw Data file processing",v_message,verbosity);
    }
  }

  else if (Mode == "FileList"){

    if(OrganizedFileList.size()==0){
      std::cout << "MRDDataDecoder tool ERROR: no files in file list to parse!" << std::endl;
      return false;
    }
    if(FileCompleted || CurrentFile=="NONE"){
      Log("MRDDataDecoder tool: File in list completed.  Moving to next part.",v_message,verbosity);
      if(verbosity>v_warning) std::cout << "MRDDataDecoder tool: Next file to load: "+OrganizedFileList.at(FileNum) << std::endl;
      CurrentFile = OrganizedFileList.at(FileNum);
      Log("MRDDataDecoder Tool: LoadingRaw Data file as BoostStore",v_debug,verbosity);
      RawData->Initialise(CurrentFile.c_str());
      RawData->Print(false);
      Log("MRDDataDecoder Tool: Accessing MRD Data in raw data",v_message,verbosity);
      RawData->Get("CCData",*MRDData);
      MRDData->Print(false);
    } else {
     if(verbosity>v_message) std::cout << "MRDDataDecoderTool: continuing file " << OrganizedFileList.at(FileNum) << std::endl;
    }
    FileCompleted = false;
  }

  else if (Mode == "Continuous"){
    std::string State;
    m_data->CStore.Get("State",State);
    std::cout << "MRDDataDecoder tool: checking CStore for status of data stream" << std::endl;
    if (State == "MRDSingle" || State == "Wait"){
      //Single event file available for monitoring; not relevant for this tool
      if (verbosity > 2) std::cout <<"MRDDataDecoder: State is "<<State<< ". No data file available" << std::endl;
      return true; 
    } else if (State == "DataFile"){
      Log("MRDDataDecoder: New data file available.",v_message,verbosity);
      m_data->Stores["CCData"]->Get("FileData",MRDData);
      MRDData->Print(false);
    } else {
      Log("MRDDataDecoder: State not recognized >>> "+State+" <<<. Please ensure the MonitorReceive tool is executed before this tool when running in continuous mode",v_error,verbosity);
      return true;
    }
  }

  Log("MRDDataDecoder Tool: Accessing run information data",v_message,verbosity);
  int RunNumber;
  int SubRunNumber;
  uint64_t StarTime;
  int RunType;
  BoostStore RunInfo(false,0);
  Store Postgress;
  bool get_runinfo;
  if (DummyRunNumber){
    Postgress.Set("RunNumber",0);
    Postgress.Set("SubRunNumber",0);
    Postgress.Set("RunType",0);
    Postgress.Set("StarTiime",0);
  } else{
    get_runinfo = RawData->Get("RunInformation",RunInfo);
    if (get_runinfo){
      RunInfo.Print(false);
      RunInfo.Get("Postgress",Postgress);
      Postgress.Print();
      Postgress.Get("RunNumber",RunNumber);
      Postgress.Get("SubRunNumber",SubRunNumber);
      Postgress.Get("RunType",RunType);
      Postgress.Get("StarTime",StarTime);
      if(verbosity>v_message) std::cout<<"Processing RunNumber: "<<RunNumber<<std::endl;
      if(verbosity>v_message) std::cout<<"Processing SubRunNumber: "<<SubRunNumber<<std::endl;
      if(verbosity>v_message) std::cout<<"Run is of run type: "<<RunType<<std::endl;
      if(verbosity>v_message) std::cout<<"StartTime of Run: "<<StarTime<<std::endl;
    } else {
      //set filler parameters in case there is no information in the RunInfo store
      RunNumber = 0;
      SubRunNumber = 0;
      StarTime = 0;
      RunType = 0;
      Postgress.Set("RunNumber",RunNumber);
      Postgress.Set("SubRunNumber",SubRunNumber);
      Postgress.Set("RunType",RunType);
      Postgress.Set("StarTime",StarTime);
    }
  }

  /////////////////// getting MRD Data ////////////////////
  Log("MRDDataDecoder Tool: Accessing MRD Data in raw data",v_message,verbosity); 
  // Show the total entries in this file  
  MRDData->Header->Get("TotalEntries",totalentries);
  if(verbosity>v_message) std::cout<<"Total entries in MRDData store: "<<totalentries<<std::endl;

  NumMRDDataProcessed = 0;
  int ExecuteEntryNum = 0;
  int EntriesToDo = 0;
  if (EntriesPerExecute <= 0) {
    EntriesToDo = totalentries;
    Log("MRDDataDecoder Tool: Parsing entire MRDData booststore this loop",v_message,verbosity);
  }
  else {
    EntriesToDo = EntriesPerExecute;
    Log("MRDDataDecoder Tool: Parsing entries "+std::to_string(EntryNum)+"-"+std::to_string(EntryNum+EntriesToDo-1)+" out of"+std::to_string(totalentries)+" of MRDData booststore this loop.",v_message,verbosity);
  }
  while((ExecuteEntryNum < EntriesToDo) && (EntryNum < totalentries)){
	Log("MRDDataDecoder Tool: Procesing MRDData Entry "+to_string(EntryNum),v_debug, verbosity);
    MRDOut mrddata;
    MRDData->GetEntry(EntryNum);
    MRDData->Get("Data",mrddata);
   
    std::string mrdTriggertype = "No Loopback";
    std::vector<unsigned long> chankeys;
    unsigned long timestamp = mrddata.TimeStamp;    //in ms since 1970/1/1
    std::vector<std::pair<unsigned long, int>> ChankeyTimePairs;
    MRDEvents.emplace(timestamp,ChankeyTimePairs);
    
    //For each entry, loop over all crates and get data
    for (unsigned int i_data = 0; i_data < mrddata.Crate.size(); i_data++){
      int crate = mrddata.Crate.at(i_data);
      int slot = mrddata.Slot.at(i_data);
      int channel = mrddata.Channel.at(i_data);
      int hittimevalue = mrddata.Value.at(i_data);
      std::vector<int> CrateSlotChannel{crate,slot,channel};
      unsigned long chankey = MRDCrateSpaceToChannelNumMap[CrateSlotChannel];
      if (chankey !=0){
        std::pair <unsigned long,int> keytimepair(chankey,hittimevalue);  //chankey will be 0 when looking at loopback channels that don't have an entry in the mapping-->skip
        MRDEvents[timestamp].push_back(keytimepair);
      }
      if (crate == 7 && slot == 11 && channel == 14) mrdTriggertype = "Cosmic";   //FIXME: don't hard-code the trigger channels?
      if (crate == 7 && slot == 11 && channel == 15) mrdTriggertype = "Beam";     //FIXME: don't hard-code the trigger channels?
    }
    
    //Entry processing done.  Label the trigger type and increment index
    TriggerTypeMap.emplace(timestamp,mrdTriggertype);
    NumMRDDataProcessed += 1;
    ExecuteEntryNum += 1;
    EntryNum+=1;
  }

  //MRD Data file fully processed.   
  //Push the map of TriggerTypeMap and FinishedMRDHits 
  //to the CStore for ANNIEEvent to start Building ANNIEEvents. 
  Log("MRDDataDecoder Tool: Saving Finished MRD Data into CStore.",v_debug, verbosity);
  //FIXME: add a check for if there is or is not an entry in the CStore
  m_data->CStore.Get("MRDEvents",CStoreMRDEvents);
  CStoreMRDEvents.insert(MRDEvents.begin(),MRDEvents.end());
  m_data->CStore.Set("MRDEvents",CStoreMRDEvents);

  m_data->CStore.Get("MRDEventTriggerTypes",CStoreTriggerTypeMap);
  CStoreTriggerTypeMap.insert(TriggerTypeMap.begin(),TriggerTypeMap.end());
  m_data->CStore.Set("MRDEventTriggerTypes",TriggerTypeMap);
  //FIXME: Should we now clear CStoreMRDEvents and CStoreTriggerTypesto free up memory?
  
  m_data->CStore.Set("NewMRDDataAvailable",true);
  m_data->CStore.Set("MRDRunInfoPostgress",Postgress);

  //Check the size of the WaveBank to see if things are bloating
  Log("MRDDataDecoder Tool: Size of MRDEvents (# MRD Triggers processed):" + 
          to_string(MRDEvents.size()),v_debug, verbosity);
  Log("MRDDataDecoder Tool: Size of MRDEvents in CStore:" + 
          to_string(TriggerTypeMap.size()),v_debug, verbosity);

  std::cout << "MRD EVENT CSTORE ENTRIES SET SUCCESSFULLY.  Clearing MRDEvents map from this file." << std::endl;
  MRDEvents.clear();
  TriggerTypeMap.clear();

   if(EntryNum == totalentries){
    Log("MRDDataDecoder Tool: RUN PART COMPLETED.  INDICATING FILE IS DONE.",v_debug, verbosity);
    FileCompleted = true;
    EntryNum = 0;
    FileNum += 1;
    RawData->Close(); RawData->Delete(); delete RawData; RawData = new BoostStore(false,0);
    MRDData->Close(); MRDData->Delete(); delete MRDData; MRDData = new BoostStore(false,2);
    if(Mode == "SingleFile"){
      Log("MRDDataDecoder Tool: Single file parsed.  Ending toolchain after this loop.",v_message, verbosity);
      m_data->vars.Set("StopLoop",1);
    }
    if(Mode == "FileList" && FileNum == int(OrganizedFileList.size())){
      Log("MRDDataDecoder Tool: Full file list parsed.  Ending toolchain after this loop.",v_message, verbosity);
      m_data->vars.Set("StopLoop",1);
    }
    //No need to stop the loop in continous mode
    if (Mode == "Continous"){
      Log("MRDDataDecoder Tool: Full raw file parsed. Waiting until next raw file is available.",v_message,verbosity);
    }
  } 

  ////////////// END EXECUTE LOOP ///////////////
  return true;
}


bool MRDDataDecoder::Finalise(){
  RawData->Close();
  RawData->Delete();
  delete RawData;
  MRDData->Close();
  MRDData->Delete();
  delete MRDData;
  std::cout << "MRDDataDecoder tool exitting" << std::endl;
  return true;
}

std::vector<std::string> MRDDataDecoder::OrganizeRunParts(std::string FileList)
{
  std::vector<std::string> OrganizedFiles;
  std::vector<std::string> UnorganizedFileList;
  std::vector<int> RunCodes;
  int ThisRunCode;
  //First, parse the lines and get all files.
  std::string line;
  ifstream myfile(FileList.c_str());
  if (myfile.is_open()){
    if(verbosity>v_debug) std::cout << "Lines in FileList being printed"  << std::endl; //has our stuff;
    while(getline(myfile,line)){
      if(verbosity>vv_debug) std::cout << "Parsing next line " << std::endl;
      if(line.find("#")!=std::string::npos) continue;
      if(verbosity>v_debug) std::cout << line << std::endl; //has our stuff;
      //std::vector<std::string> splitline;
      //boost::split(splitline,line, boost::is_any_of(","), boost::token_compress_on);
      //for(int i = 0;i<splitline.size(); i ++){
        //std::string filename = splitline.at(i);
        std::string filename = line;
	    int rawfilerun, rawfilesubrun, rawfilepart;
        int numargs = 0;
	    try{
	    	std::reverse(line.begin(),line.end());
            //Part number is the first character now.
            size_t pplace = line.find("p");
            std::string part = line.substr(0,pplace);
            std::reverse(part.begin(),part.end());
            rawfilepart = std::stoi(part);
	    	// Get the address
	    	char * p = std::strtok(const_cast<char*>(line.c_str()),"RSp");
	    	while(numargs <2){
	    		p = std::strtok(NULL,"RSp");
	    		std::string capturedstring(p);
                if(verbosity>vv_debug) std::cout << "Caotured string is " << capturedstring << std::endl;
	    		std::reverse(capturedstring.begin(),capturedstring.end());
	    		if (numargs == 0) rawfilesubrun = std::stoi(capturedstring.c_str());
	    		else if (numargs == 1) rawfilerun = std::stoi(capturedstring.c_str());
	    		numargs++;
	    	}
	    } catch (int e){
            std::cout <<"unrecognised input file pattern!: "<<filename
	    		<<"File will be ignored,"<<endl;
	    	//return;
	    	rawfilerun=-1;
	    	rawfilesubrun=-1;
	    	rawfilepart = -1;
	    }
        ThisRunCode = rawfilerun*1000000 + ((rawfilesubrun+1)*1000) + rawfilepart;
        if (rawfilerun!=-1){
          if(verbosity>v_warning) std::cout << "MRDDataDecoder Tool: will parse file : " << filename << std::endl;
          UnorganizedFileList.push_back(filename);
          RunCodes.push_back(ThisRunCode);
        } else {
          if(verbosity>v_error) std::cout << "MRDDataDecoder Tool: Problem parsing this file name: " << filename << std::endl;
        }
      //} // End parse filename in data line 
    } // End parsing each line in file
    if(verbosity>v_debug) std::cout << "MRDDataDecoder Tool: Organizing filenames: " << std::endl;
    //Now, organize files based on the part number array
    std::vector < std::pair<int,std::string>> SortingVector;
    for (int i = 0; i < UnorganizedFileList.size(); i++){
      SortingVector.push_back( std::make_pair(RunCodes.at(i),UnorganizedFileList.at(i)));
    }
    std::sort(SortingVector.begin(),SortingVector.end());
    for (int j = 0; j<SortingVector.size();j ++) {
      OrganizedFiles.push_back(SortingVector.at(j).second);
    }
  } else {
    Log("MRDDataDecoder Tool: FIle not found at nput file list path. "
        "no Organized Run List will be returned ",
        v_error, verbosity);
  }
  return OrganizedFiles;
}

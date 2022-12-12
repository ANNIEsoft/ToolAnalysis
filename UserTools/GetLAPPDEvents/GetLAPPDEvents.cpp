#include "GetLAPPDEvents.h"

GetLAPPDEvents::GetLAPPDEvents():Tool(){}


bool GetLAPPDEvents::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("InputFile",InputFile);
  m_variables.Get("OutputFile",OutputFile);
  m_variables.Get("OutputFileLAPPD",OutputFileLAPPD);
    
  OrganizedFileList = this->OrganizeRunParts(InputFile);

  RawData = new BoostStore(false,0);
  LAPPDData = new BoostStore(false,2);
  PMTData = new BoostStore(false,2);
  MRDData = new BoostStore(false,2);
  TrigData = new BoostStore(false,2);

  out_file.open(OutputFile.c_str());
  out_file_lappd.open(OutputFileLAPPD.c_str());

  out_file << "Filename, VME events, MRD events, Trig events, LAPPD events"<<std::endl;

  FileNum = 0;
  FileCompleted = false;

  return true;
}


bool GetLAPPDEvents::Execute(){

  bool ProcessingComplete = false;
   if(FileCompleted) ProcessingComplete = this->InitializeNewFile();
  if(ProcessingComplete) {
    Log("LoadRawData Tool: All files have been processed.",v_message,verbosity);
    m_data->CStore.Set("FileProcessingComplete",true);
    return true;
  }
 
  if(OrganizedFileList.size()==0){
    std::cout << "GetLAPPDEvents tool ERROR: no files in file list to parse!" << std::endl;
    m_data->vars.Set("StopLoop",1);
    return false;
  }

  std::string CurrentFile = OrganizedFileList.at(FileNum);
  std::cout <<"CurrentFile: "<<CurrentFile<<std::endl;
  RawData->Initialise(CurrentFile.c_str());
  std::cout <<"Initialized"<<std::endl;
  RawData->Print(false);
  long lappdtotalentries;
  long pmttotalentries, mrdtotalentries, trigtotalentries;
  if (RawData->Has("PMTData")){
    RawData->Get("PMTData",*PMTData);
    PMTData->Header->Get("TotalEntries",pmttotalentries);
  } else pmttotalentries = 0;
  out_file << CurrentFile <<", "<<pmttotalentries<<", ";

    if (RawData->Has("CCData")){
    RawData->Get("CCData",*MRDData);
    MRDData->Header->Get("TotalEntries",mrdtotalentries);
  } else mrdtotalentries = 0;
  out_file <<mrdtotalentries <<", ";

  if (RawData->Has("TrigData")){
    RawData->Get("TrigData",*TrigData);
    TrigData->Header->Get("TotalEntries",trigtotalentries);
  } else trigtotalentries = 0;
  out_file <<trigtotalentries <<", ";

  if (RawData->Has("LAPPDData")){
  try{
      RawData->Get("LAPPDData",*LAPPDData);
      LAPPDData->Header->Get("TotalEntries",lappdtotalentries);
      std::cout << CurrentFile << ", " <<lappdtotalentries<<std::endl;
      out_file << lappdtotalentries<<std::endl;
      if (lappdtotalentries > 0) out_file_lappd << CurrentFile << std::endl;
      if(verbosity>3) LAPPDData->Print(false);
      FileCompleted = true;
     } catch (...) {
       Log("LoadRawData: Did not find LAPPDData in raw data file! (Maybe corrupted!!!) Don't process LAPPDData",0,0);
      lappdtotalentries=0;
       std::cout <<"1"<<std::endl;
      std::cout << CurrentFile << ", " <<lappdtotalentries<<std::endl;
      out_file << lappdtotalentries<<std::endl;
      FileCompleted = true;

    }
    } else {
      out_file << ", 0"<<std::endl;
      FileCompleted = true;

    }

    Log("LoadRawData Tool: LAPPDData has "+std::to_string(lappdtotalentries)+" entries",v_debug,verbosity);

  return true;
}


bool GetLAPPDEvents::Finalise(){

  out_file.close();
  out_file_lappd.close();

  return true;
}

std::vector<std::string> GetLAPPDEvents::OrganizeRunParts(std::string FileList)
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
                // Get the
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
      }
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

bool GetLAPPDEvents::InitializeNewFile(){
  bool EndOfProcessing = false;
  FileNum += 1;
  std::cout <<"GetLAPPDEvents Tool: File # "<<FileNum<<" / " << OrganizedFileList.size()<<std::endl;
  RawData->Close(); RawData->Delete(); delete RawData; RawData = new BoostStore(false,0);
  LAPPDData->Close(); LAPPDData->Delete(); delete LAPPDData; LAPPDData = new BoostStore(false,2);
  PMTData->Close(); PMTData->Delete(); delete PMTData; PMTData = new BoostStore(false,2);
  MRDData->Close(); MRDData->Delete(); delete MRDData; MRDData = new BoostStore(false,2);
  TrigData->Close(); TrigData->Delete(); delete TrigData; TrigData = new BoostStore(false,2);

  if (FileNum == int(OrganizedFileList.size())){
    Log("LoadRawData Tool: Full file list parsed.  Ending toolchain after this loop.",v_message, verbosity);
    m_data->vars.Set("StopLoop",1);
    EndOfProcessing = true;
  }
  return EndOfProcessing;
}

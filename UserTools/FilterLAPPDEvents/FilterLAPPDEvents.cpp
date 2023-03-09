#include "FilterLAPPDEvents.h"

FilterLAPPDEvents::FilterLAPPDEvents():Tool(){}


bool FilterLAPPDEvents::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

    m_variables.Get("DesiredCuts", DesiredCuts);
    m_variables.Get("verbosity", verbosity);
    m_variables.Get("FilteredFilesBasename", FilteredFilesBasename);
    m_variables.Get("SavePath", SavePath);
    m_variables.Get("csvInputFile", csvInputFile);
    m_variables.Get("RangeOfRuns", RangeOfRuns);


    // Open csv input file
    DetailedRunInfo = LoadDetailedRunCSV(csvInputFile);
    // print first 10 lines to check
    for(unsigned int i=0; i<10; i++){
        for(unsigned int j=0; j<DetailedRunInfo.at(i).size(); j++){
            std::cout<<DetailedRunInfo.at(i).at(j)<<",";
        }
        std::cout<<std::endl;
    }

    matched = 0;
    

    FilteredEvents = new BoostStore(false,2);   
    
    // Check if user has set a recognised cuts selection
    if(DesiredCuts != "LAPPDEvents" && DesiredCuts != "LAPPDEventsBeamgateMRDTrack" && DesiredCuts != "LAPPDEventsBeamgateMRDTrackNoVeto"){
        std::cout << "FilterLAPPDEvents Error: Invalid Cuts!" << '\n';
        std::cout << "Must be LAPPDEvents, LAPPDEventsBeamgateMRDTrack, or LAPPDEventsBeamgateMRDTrackNoVeto" << '\n';
        return false;
    }


  return true;
}


bool FilterLAPPDEvents::Execute(){

    std::map<std::string,bool> dataStreams;
    int runNumber, eventNumber, localEventNumber, partNumber;
    m_data->Stores["ANNIEEvent"]->Get("DataStreams",dataStreams);
    m_data->Stores["ANNIEEvent"]->Get("RunNumber",runNumber);
    m_data->Stores["ANNIEEvent"]->Get("EventNumber",eventNumber);
    m_data->Stores["ANNIEEvent"]->Get("LocalEventNumber",localEventNumber);
    m_data->Stores["ANNIEEvent"]->Get("PartNumber",partNumber);
    //std::cout<<"EventNumber is: "<<eventNumber<<std::endl;
    //std::cout <<"partNumber: "<<partNumber<<std::endl;


    // Check if event has LAPPD data
    if (dataStreams["LAPPD"] == 1){
        // if event has LAPPD data, we can always just save it and move on to the next
        if(DesiredCuts == "LAPPDEvents"){
            if(verbosity>0)std::cout<<"FilterLAPPDEvents: Saving event with LAPPDEvents cut"<<std::endl;
            this->SetAndSaveEvent();
	    matched++;
        }
        // For other cuts, need to find the event in the csv input file and save it if it fits
        else {
            //std::cout <<"runNumber: "<<runNumber<<", partNumber: "<<partNumber<<", evnumber: "<<localEventNumber<<std::endl;
            }
            // Loop through each row of the csv
            for(unsigned int i=0; i<DetailedRunInfo.size(); i++){
                // Pick out the run number, part number, and (local) event number of the event from the csv
                if(runNumber == stoi(DetailedRunInfo.at(i).at(0)) && partNumber == stoi(DetailedRunInfo.at(i).at(2)) && localEventNumber == stoi(DetailedRunInfo.at(i).at(3))){
                    if(DesiredCuts == "LAPPDEventsBeamgateMRDTrack"){
                        // Select events which have 1 in "InBeamWindow" and "MRDTrack" columns from spreadsheet
                        if (stoi(DetailedRunInfo.at(i).at(5)) == 1 && stoi(DetailedRunInfo.at(i).at(6)) == 1){
                            if(verbosity>0)std::cout<<"FilterLAPPDEvents: Saving event with LAPPDEventsBeamgateMRDTrack cuts"<<std::endl;
                            this->SetAndSaveEvent();
                            matched++;
                        }
                    }

                    else if(DesiredCuts == "LAPPDEventsBeamgateMRDTrackNoVeto"){
                        // Select events which have 1 in "InBeamWindow", "MRDTrack", and "NoVeto" columns from spreadsheet
                        if (stoi(DetailedRunInfo.at(i).at(5)) == 1 && stoi(DetailedRunInfo.at(i).at(6)) == 1 && stoi(DetailedRunInfo.at(i).at(7)) == 1){
                            if(verbosity>0)std::cout<<"FilterLAPPDEvents: Saving event with LAPPDEventsBeamgateMRDTrackNoVeto cuts"<<std::endl;
                            this->SetAndSaveEvent();
                            matched++;
                        }
                    }
                        
                }
                
        }
    }

  return true;
}


bool FilterLAPPDEvents::Finalise(){
    // Save and close the file
    FilteredEvents->Close();
    FilteredEvents->Delete();
    delete FilteredEvents;
    std::cout<<"Number of matched run numbers, part numbers, and (local) event numbers: "<<matched<<std::endl;
  return true;
}

void FilterLAPPDEvents::SetAndSaveEvent(){
    // Declare all event variables
    uint32_t RunNumber, SubrunNumber, EventNumber, TriggerWord, LocalEventNumber;
    uint64_t RunStartTime, EventTimeTank, EventTimeMRD, EventTimeLAPPD, CTCTimestamp, LAPPDOffset;
    int PartNumber, RunType, TriggerExtended;
    std::map<std::string, bool> DataStreams;
    std::string MRDTriggerType;
    std::map<std::string, int> MRDLoopbackTDC;
    TriggerClass TriggerData;
    BeamStatus BeamStatus;
    std::map<unsigned long, std::vector<Hit>>* TDCData;
    std::map<unsigned long, std::vector<Hit>> *Hits, *AuxHits;
    std::map<unsigned long, std::vector<int>> RawAcqSize;
    std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCData, RecoAuxADCData;
    PsecData LAPPDData;

    std::map<unsigned long, std::vector<Hit>> *NewTDCData = new std::map<unsigned long, std::vector<Hit>>;
    std::map<unsigned long, std::vector<Hit>> *NewHits = new  std::map<unsigned long, std::vector<Hit>>;
    std::map<unsigned long, std::vector<Hit>> *NewAuxHits = new std::map<unsigned long, std::vector<Hit>>;

    // Get and Set all variables in the event to the new booststore
    m_data->Stores["ANNIEEvent"]->Get("AuxHits",AuxHits);
    for (auto&& entry : (*AuxHits)){
        NewAuxHits->emplace(entry.first,entry.second);
        }
    FilteredEvents->Set("AuxHits",NewAuxHits,true);

    m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);  FilteredEvents->Set("BeamStatus",BeamStatus);
    m_data->Stores["ANNIEEvent"]->Get("CTCTimestamp",CTCTimestamp);  FilteredEvents->Set("CTCTimestamp",CTCTimestamp);
    m_data->Stores["ANNIEEvent"]->Get("DataStreams",DataStreams);  FilteredEvents->Set("DataStreams",DataStreams);
    m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);  FilteredEvents->Set("EventNumber",EventNumber);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeLAPPD",EventTimeLAPPD);  FilteredEvents->Set("EventTimeLAPPD",EventTimeLAPPD);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeMRD",EventTimeMRD);  FilteredEvents->Set("EventTimeMRD",EventTimeMRD);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeTank",EventTimeTank);  FilteredEvents->Set("EventTimeTank",EventTimeTank);
    m_data->Stores["ANNIEEvent"]->Get("Hits",Hits);
    for (auto&& entry : (*Hits)){
        NewHits->emplace(entry.first,entry.second);
        }
    FilteredEvents->Set("Hits",NewHits,true);
    
    m_data->Stores["ANNIEEvent"]->Get("LAPPDData",LAPPDData);  FilteredEvents->Set("LAPPDData",LAPPDData);
    m_data->Stores["ANNIEEvent"]->Get("LAPPDOffset",LAPPDOffset);  FilteredEvents->Set("LAPPDOffset",LAPPDOffset);
    m_data->Stores["ANNIEEvent"]->Get("LocalEventNumber",LocalEventNumber);  FilteredEvents->Set("LocalEventNumber",LocalEventNumber);
    m_data->Stores["ANNIEEvent"]->Get("MRDLoopbackTDC",MRDLoopbackTDC);  FilteredEvents->Set("MRDLoopbackTDC",MRDLoopbackTDC);
    m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType",MRDTriggerType);  FilteredEvents->Set("MRDTriggerType",MRDTriggerType);
    m_data->Stores["ANNIEEvent"]->Get("PartNumber",PartNumber);  FilteredEvents->Set("PartNumber",PartNumber);
    m_data->Stores["ANNIEEvent"]->Get("RawAcqSize",RawAcqSize);  FilteredEvents->Set("RawAcqSize",RawAcqSize);
    m_data->Stores["ANNIEEvent"]->Get("RecoADCData",RecoADCData);  FilteredEvents->Set("RecoADCData",RecoADCData);
    m_data->Stores["ANNIEEvent"]->Get("RecoAuxADCData",RecoAuxADCData);  FilteredEvents->Set("RecoAuxADCData",RecoAuxADCData);
    m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);  FilteredEvents->Set("RunNumber",RunNumber);
    m_data->Stores["ANNIEEvent"]->Get("RunStartTime",RunStartTime);  FilteredEvents->Set("RunStartTime",RunStartTime);
    m_data->Stores["ANNIEEvent"]->Get("RunType",RunType);    FilteredEvents->Set("RunType",RunType);
    m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);  FilteredEvents->Set("SubrunNumber",SubrunNumber);
    m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
    for (auto&& entry : (*TDCData)){
        NewTDCData->emplace(entry.first,entry.second);
        }
    
    FilteredEvents->Set("TDCData",NewTDCData,true);
    m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);  FilteredEvents->Set("TriggerData",TriggerData);
    m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",TriggerExtended);  FilteredEvents->Set("TriggerExtended",TriggerExtended);
    m_data->Stores["ANNIEEvent"]->Get("TriggerWord",TriggerWord);  FilteredEvents->Set("TriggerWord",TriggerWord);

    std::string Filename = SavePath + FilteredFilesBasename + "_" + DesiredCuts + "_" + RangeOfRuns;

    if (verbosity>0) std::cout<<"Filename is "<<Filename<<std::endl;
    FilteredEvents->Save(Filename);
    FilteredEvents->Delete();
    return;
}


std::vector<std::vector<std::string>> FilterLAPPDEvents::LoadDetailedRunCSV(std::string filename){
    std::vector<std::vector<std::string>> DetailedRunInfo;
    std::vector<std::string> row;
    std::string line, word;

    std::fstream file(filename, ios::in);
    if(file.is_open()){
        while(std::getline(file, line)){
            //std::cout<<"line equals: "<<line<<std::endl;
            row.clear();

            std::stringstream str(line);
            //std::cout<<"str equals: "<<str.str()<<std::endl;

            while(std::getline(str, word, ',')){
                //std::cout<<"word equals: "<<word<<std::endl;
                row.push_back(word);
                }
            DetailedRunInfo.push_back(row);
        }
    }
    else
        std::cout<<"Could not open the file: "<<filename<<'\n';

    return DetailedRunInfo;
    
}


#include "LAPPDASCIIReadIn.h"

LAPPDASCIIReadIn::LAPPDASCIIReadIn():Tool(){}


bool LAPPDASCIIReadIn::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  string NewFileName;
  int Nboards;
  //get the data file name from the config file
  //needs to have a path to the data files (inside the "")
  m_variables.Get("PSECinputfile", NewFileName);
  m_variables.Get("DoPedSubtraction", DoPedSubtract);
  m_variables.Get("Nboards", Nboards);
  m_variables.Get("Pedinputfile1", PedFileName1);
  m_variables.Get("Pedinputfile2", PedFileName2);
  LAPPDPSECReadInVerbosity=0;
  m_variables.Get("LAPPDPSECReadInVerbosity",LAPPDPSECReadInVerbosity);


  std::string OWL;
  m_variables.Get("RawDataOutpuWavLabel",OWL);
  OutputWavLabel = OWL;


  //Opening data file
  DataFile.open(NewFileName);
  if(!DataFile.is_open())
  {
        cout<<"Failed to open "<<NewFileName<<"!"<<endl;
        return false;
  }
  cout<<"Opened file: "<<NewFileName<<endl;


  //parameters (potentially) used by the whole ToolChain
  m_variables.Get("Nsamples", Nsamples);
  m_variables.Get("NChannels", NChannels);
  m_variables.Get("TrigChannel", TrigChannel);
  m_variables.Get("LAPPDchannelOffset", LAPPDchannelOffset);
  m_variables.Get("SampleSize", SampleSize);
  m_variables.Get("oldLaser",oldLaser);


  m_data->Stores["ANNIEEvent"]->Set("Nsamples", Nsamples);
	m_data->Stores["ANNIEEvent"]->Set("NChannels", NChannels);
	m_data->Stores["ANNIEEvent"]->Set("TrigChannel", TrigChannel);
	m_data->Stores["ANNIEEvent"]->Set("LAPPDchannelOffset", LAPPDchannelOffset);
	m_data->Stores["ANNIEEvent"]->Set("SampleSize", SampleSize);
  m_data->Stores["ANNIEEvent"]->Set("oldLaser", oldLaser);

  PedestalValues = new std::map<unsigned long, vector<int>>;

  if(DoPedSubtract==1){
      ReadPedestals(0);
      if(Nboards==2) ReadPedestals(1);
  }

  eventNo=0; //event number

  if(DoPedSubtract==1) cout<<"PEDSIZES: "<<PedestalValues->size()<<" "<<PedestalValues->at(0).size()<<" "<<PedestalValues->at(4).at(5)<<endl;

  return true;
}


bool LAPPDASCIIReadIn::Execute(){
  //create raw lappd object


  //since unsure if LAPPDFilter, LAPPDBaselineSubtract, or LAPPDcfd are being used, isFiltered, isBLsub, and isCFD are set to false
  bool isFiltered = false;
  m_data->Stores["ANNIEEvent"]->Set("isFiltered",isFiltered);
  bool isBLsub = false;
  m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted",isBLsub);
  bool isCFD=false;
  m_data->Stores["ANNIEEvent"]->Set("isCFD",isCFD);

  //Boards: -- Hardcoded for now
  vector<int> NReadBoards = {0,1};
  m_data->Stores["ANNIEEvent"]->Set("ACDCboards",NReadBoards);

  if(eventNo%10==0) cout<<"Event: "<<eventNo<<endl;

  if(LAPPDPSECReadInVerbosity>0) cout<<"BEGIN LAPPDPSECReadIn "<< endl;

  LAPPDWaveforms = new std::map<unsigned long, vector<Waveform<double>>>;

  //create a waveform for temp storage


  string nextLine; //temp line to parse

  map <unsigned long, vector<Waveform<double>>> :: iterator itr;

  vector<string> acdcmetadata;
  while(getline(DataFile, nextLine))
  {
      int sampleNo=0; //sample number
      unsigned long channelNo=0; //channel number
      istringstream iss(nextLine); //copies the current line in the file
      int location=-1; //counts the current perameter in the line
      string stempValue; //current string in the line
      int tempValue; //current int in the line

      //starts the loop at the beginning of the line
      while(iss >> stempValue)
      {
          location++;
          //  cout<<location<<endl;
          //when location==0, it is at the sample number

          //cout<<"beginning "<<stempValue<<endl;

          if(location==0)
          {
              sampleNo = stoi(stempValue, 0, 10);

              //(int)strtol(tempValue, NULL, 16);
              //cout<<"SAMPLE: "<<sampleNo<<endl;
              //sampleNo=tempValue;
              continue;
          }

          if(LAPPDPSECReadInVerbosity>1) {cout<<"PARSING 30 Channels for sample: "<<sampleNo<<endl;}

          //this is the meta data per format
          if((location)%31==0)
          {
              acdcmetadata.push_back(stempValue);
              if(LAPPDPSECReadInVerbosity>1) { cout<<"parsing metadata at column: "<< location<<" "<<stempValue<<endl;}
              continue;
          }

          int tempValue = stoi(stempValue, 0, 10);

          int theped;
          map <unsigned long, vector<int>> :: iterator pitr;
          if((PedestalValues->count(channelNo))>0){
              pitr = PedestalValues->find(channelNo);
              theped = (pitr->second).at(sampleNo);
          } else theped = 0;

          if(LAPPDPSECReadInVerbosity>1 && theped==0) cout<<"THE PED = 0!!!!!!!! "<<theped<<" "<<channelNo<<" "<<sampleNo<<" "<<PedestalValues->count(channelNo)<<endl;
          int pedsubValue = tempValue - theped;

          if(sampleNo==0)
          {
              Waveform<double> tempwav;
              //if none of the above if statements happen
              //it is going through each channel 0 through the NChannels-1
              //the tempValue will be placed into the tempwav
              tempwav.PushSample(0.3*((double)pedsubValue));
              //cout<<"does it get here? "<<location<<" "<<tempValue<<endl;
              vector<Waveform<double>> Vtempwav;
              Vtempwav.push_back(tempwav);
              //each tempwav will be inserted into the correct channelno
              LAPPDWaveforms->insert(pair<unsigned long, vector<Waveform<double>>>(channelNo,Vtempwav));
              //clear the tempwav for a new value
              if(LAPPDPSECReadInVerbosity>2) cout<<"FIRST SAMPLE: "<<eventNo<<" "<<sampleNo<<" "<<channelNo<<" "<<tempValue<<endl;
          }
          else
          {
              //fitr = LAPPDWaveforms->find(channelNo);
              (((LAPPDWaveforms->find(channelNo))->second).at(0)).PushSample(0.3*((double)pedsubValue));
              if(LAPPDPSECReadInVerbosity>2) {cout<<"SUBSEQUENT SAMPLES: "<<eventNo<<" "<<sampleNo<<" "<<channelNo<<" "<<tempValue<<endl;}
          }

        channelNo++;
      }

      //when the sampleNo gets to ff
      //it is at sample 256 and needs to go to the next event
      //if(sampleNo=='ff')
      if(sampleNo==255)
      {
          if(LAPPDPSECReadInVerbosity>1) {cout<<"END OF THE EVENT "<<sampleNo<<" "<<eventNo<<endl;}
          //eventNo++;
          break;
      }

      //cout<<"at the end"<<endl;
  }

  if(LAPPDPSECReadInVerbosity>0) {
        cout<<"END LAPPDPSECReadIn...acdcmetadata size: "<<acdcmetadata.size()<<"   LAPPDWaveforms size: "<<(((LAPPDWaveforms->find(0))->second).at(0)).GetSamples()->size()<<endl;
        cout<<"*************************************************************"<<endl;
  }


  m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel,LAPPDWaveforms);
  m_data->Stores["ANNIEEvent"]->Set("ACDCmetadata",acdcmetadata);
  LAPPDWaveforms->clear();
  eventNo++;

  return true;
}


bool LAPPDASCIIReadIn::Finalise(){
  DataFile.close();
  return true;
}


bool LAPPDASCIIReadIn::ReadPedestals(int boardNo){

    cout<<"Getting Pedestals "<<boardNo<<endl;

    string nextLine; //temp line to parse
    double finalsampleNo;

    if(boardNo==0){
        PedFile.open(PedFileName1);
        if(!PedFile.is_open())
        {
          cout<<"Failed to open "<<PedFileName1<<"!"<<endl;
          return false;
        }
        cout<<"Opened file: "<<PedFileName1<<endl;
    }else{
        PedFile.open(PedFileName2);
        if(!PedFile.is_open())
        {
          cout<<"Failed to open "<<PedFileName2<<"!"<<endl;
          return false;
        }
        cout<<"Opened file: "<<PedFileName2<<endl;
    }

    int sampleNo=0; //sample number
    while(getline(PedFile, nextLine))
    {
        istringstream iss(nextLine); //copies the current line in the file
        int location=-1; //counts the current perameter in the line
        string stempValue; //current string in the line
        int tempValue; //current int in the line

        unsigned long channelNo=0; //channel number
        if(boardNo==1) { channelNo = 30; } //cout<<"NEW BOARD "<<channelNo<<" "<<sampleNo<<endl;}

        //starts the loop at the beginning of the line
        while(iss >> stempValue)
        {
            location++;

            int tempValue = stoi(stempValue, 0, 10);

            if(sampleNo==0){
                vector<int> tempPed;
                tempPed.push_back(tempValue);
                //cout<<"First time: "<<channelNo<<" "<<tempValue<<endl;
                PedestalValues->insert(pair <unsigned long,vector<int>> (channelNo,tempPed));
                //newboard=false;
            }
            else{
                //cout<<"Following time: "<<channelNo<<" "<<tempValue<<"    F    "<<PedestalValues->count(channelNo)<<endl;
                (((PedestalValues->find(channelNo))->second)).push_back(tempValue);
            }

            channelNo++;
        }

        sampleNo++;
        //when the sampleNo gets to ff
        //it is at sample 256 and needs to go to the next event
        //if(sampleNo=='ff')
        //if(sampleNo==255)
        //{
            //cout<<"DID THIS HAPPEN "<<sampleNo<<" "<<eventNo<<endl;
        //    eventNo++;
        //    break;
        //}

        //cout<<"at the end"<<endl;
    }

    cout<<"FINAL SAMPLE NUMBER: "<<PedestalValues->size()<<" "<<(((PedestalValues->find(0))->second)).size()<<endl;
    PedFile.close();
  return true;
}


bool LAPPDASCIIReadIn::MakePedestals(){

  //Empty for now...

  return true;
}

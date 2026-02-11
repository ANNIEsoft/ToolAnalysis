#include "LAPPDLoadTXT.h"

LAPPDLoadTXT::LAPPDLoadTXT() : Tool() {}

bool LAPPDLoadTXT::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  LAPPDLoadTXTVerbosity = 0;
  m_variables.Get("LAPPDLoadTXTVerbosity", LAPPDLoadTXTVerbosity);

  PedFileNameTXT = "../Pedestals/TXTPdedestals/P";
  m_variables.Get("PedFileNameTXT", PedFileNameTXT);

  NBoards = 2;
  m_variables.Get("NBoards", NBoards);

  OutputWavLabel = "RawLAPPDData";
  m_variables.Get("OutputWavLabel", OutputWavLabel);

  DoPedSubtract = true;
  m_variables.Get("DoPedSubtract", DoPedSubtract);

  if (DoPedSubtract)
  {
    PedestalValues = new std::map<unsigned long, vector<int>>;
    for (int i = 0; i < NBoards; i++)
    {
      Log("LAPPDLoadTXT: Loading " + std::to_string(NBoards) + "Pedestal file from " + PedFileNameTXT, 0, LAPPDLoadTXTVerbosity);
      ReadPedestal(i);
    }
    Log("LAPPDLoadTXT: Pedestal file loaded", 0, LAPPDLoadTXTVerbosity);
  }

  Nsamples = 256;
  m_variables.Get("Nsamples", Nsamples);
  NChannels = 30;
  m_variables.Get("NChannels", NChannels);
  TrigChannel = 5;
  m_variables.Get("TrigChannel", TrigChannel);
  LAPPDchannelOffset = 1000;
  m_variables.Get("LAPPDchannelOffset", LAPPDchannelOffset);
  SampleSize = 80;
  m_variables.Get("SampleSize", SampleSize);

  m_data->Stores["ANNIEEvent"]->Set("Nsamples", Nsamples);
  m_data->Stores["ANNIEEvent"]->Set("NChannels", NChannels);
  m_data->Stores["ANNIEEvent"]->Set("TrigChannel", TrigChannel);
  m_data->Stores["ANNIEEvent"]->Set("LAPPDchannelOffset", LAPPDchannelOffset);
  m_data->Stores["ANNIEEvent"]->Set("SampleSize", SampleSize);

  m_variables.Get("DataFileName", DataFileName);
  DataFile.open(DataFileName);
  if (!DataFile.is_open())
  {
    cout << "LAPPDLoadTXT: Failed to open " << DataFileName << "!" << endl;
    return false;
  }

  eventNo = 0;

  oldLaser = 0;
  m_variables.Get("oldLaser", oldLaser);

  return true;
}

bool LAPPDLoadTXT::Execute()
{
  Log("LAPPDLoadTXT: Reading data file", v_debug, LAPPDLoadTXTVerbosity);
  bool isFiltered = false;
  m_data->Stores["ANNIEEvent"]->Set("isFiltered", isFiltered);
  bool isBLsub = false;
  m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted", isBLsub);
  bool isCFD = false;
  m_data->Stores["ANNIEEvent"]->Set("isCFD", isCFD);

  LAPPDWaveforms = new std::map<unsigned long, vector<Waveform<double>>>;

  if (eventNo % 50 == 0)
    Log("LAPPDLoadTXT: Event " + std::to_string(eventNo), v_message, LAPPDLoadTXTVerbosity);

  // alwasys set LAPPDana to true because we are loading ASCII data, every loop mush has data
  bool LAPPDana = true;
  m_data->CStore.Set("LAPPDana", LAPPDana);

  ReadData();

  vector<int> NReadBoards = {0, 1};
  m_data->Stores["ANNIEEvent"]->Set("ACDCboards", NReadBoards);
  m_data->Stores["ANNIEEvent"]->Set("SortedBoards", NReadBoards);

  vector<int> ACDCReadedLAPPDID = {0, 0};
  m_data->Stores["ANNIEEvent"]->Set("ACDCReadedLAPPDID", ACDCReadedLAPPDID);

  int LAPPD_ID = 0;
  m_data->Stores["ANNIEEvent"]->Set("oldLaser", oldLaser);
  m_data->Stores["ANNIEEvent"]->Set("LAPPD_ID", LAPPD_ID);
  m_data->Stores["ANNIEEvent"]->Set("TrigChannel", TrigChannel);
  m_data->Stores["ANNIEEvent"]->Set("LAPPDchannelOffset", LAPPDchannelOffset);
  m_data->Stores["ANNIEEvent"]->Set("SampleSize", SampleSize);

  m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel, LAPPDWaveforms);
  m_data->Stores["ANNIEEvent"]->Set("ACDCmetadata", metaData);
  LAPPDWaveforms->clear();
  metaData.clear();
  eventNo++;

  return true;
}

bool LAPPDLoadTXT::Finalise()
{
  DataFile.close();
  Log("LAPPDLoadTXT: Data file closed", v_message, LAPPDLoadTXTVerbosity);

  return true;
}

bool LAPPDLoadTXT::ReadPedestal(int boardNo)
{
  // copied fomr LAPPDLoadStore

  if (LAPPDLoadTXTVerbosity > 0)
    cout << "Getting Pedestals " << boardNo << endl;

  std::string LoadName = PedFileNameTXT;
  string nextLine; // temp line to parse
  double finalsampleNo;
  std::string ext = std::to_string(boardNo);
  ext += ".txt";
  LoadName += ext;
  ifstream PedFile;
  PedFile.open(LoadName); // final name: PedFileNameTXT + boardNo + .txt
  if (!PedFile.is_open())
  {
    cout << "Failed to open " << LoadName << "!" << endl;
    return false;
  }
  if (LAPPDLoadTXTVerbosity > 0)
    cout << "Opened pedestal file: " << LoadName << endl;

  int sampleNo = 0; // sample number
  while (getline(PedFile, nextLine))
  {
    istringstream iss(nextLine);            // copies the current line in the file
    int location = -1;                      // counts the current perameter in the line
    string stempValue;                      // current string in the line
    int tempValue;                          // current int in the line
    unsigned long channelNo = boardNo * 30; // channel number
    // cout<<"NEW BOARD "<<channelNo<<" "<<sampleNo<<endl;
    // starts the loop at the beginning of the line
    while (iss >> stempValue)
    {
      location++;
      tempValue = stoi(stempValue, 0, 10);
      if (sampleNo == 0)
      {
        vector<int> tempPed;
        tempPed.push_back(tempValue);
        PedestalValues->insert(pair<unsigned long, vector<int>>(channelNo, tempPed));
        if (LAPPDLoadTXTVerbosity > 0)
          cout << "Inserting pedestal at channelNo " << channelNo << endl;
      }
      else
      {
        (((PedestalValues->find(channelNo))->second)).push_back(tempValue);
      }

      channelNo++;
    }
    sampleNo++;
  }
  if (LAPPDLoadTXTVerbosity > 0)
    cout << "FINAL SAMPLE NUMBER: " << PedestalValues->size() << " " << (((PedestalValues->find(0))->second)).size() << endl;
  PedFile.close();
  return true;
}

void LAPPDLoadTXT::ReadData()
{
  string nextLine; // temp line to parse

  map<unsigned long, vector<Waveform<double>>>::iterator itr;

  int lineNumber = 0;
  while (getline(DataFile, nextLine))
  {
    int sampleNo = 0;            // sample number
    unsigned long channelNo = 0; // channel number
    istringstream iss(nextLine); // copies the current line in the file
    int location = -1;           // counts the current perameter in the line
    string stempValue;           // current string in the line
    int tempValue;               // current int in the line

    while (iss >> stempValue)
    {
      location++;
      if (location == 0)
      {
        sampleNo = stoi(stempValue, 0, 10);
        Log("LAPPDLoadTXT: at sampleNo " + std::to_string(sampleNo) + " at line " + std::to_string(lineNumber), 5, LAPPDLoadTXTVerbosity);
        continue;
      }
      Log("LAPPDLoadTXT: Parsing 30 channels for sample " + std::to_string(sampleNo) + " at location " + std::to_string(location), 10, LAPPDLoadTXTVerbosity);

      if (location % 31 == 0)
      {
        metaDataString.push_back(stempValue);
        // convert string stempValue to unsigned short
        unsigned short tempMeta = (unsigned short)stoi(stempValue, 0, 16);
        metaData.push_back(tempMeta);
        continue;
      }

      int tempValue = stoi(stempValue, 0, 10);

      int theped;
      map<unsigned long, vector<int>>::iterator pitr;
      if ((PedestalValues->count(channelNo)) > 0)
      {
        pitr = PedestalValues->find(channelNo);
        theped = (pitr->second).at(sampleNo);
      }
      else
      {
        theped = 0;
      }

      int pedsubValue = tempValue - theped;
      if (LAPPDLoadTXTVerbosity > 11)
        cout << "LAPPDLoadTXT: data value: " << tempValue << " pedestal value: " << theped << " pedsub value: " << pedsubValue << ", inserted " << 0.3 * ((double)pedsubValue) << endl;

      if (sampleNo == 0)
      {
        Waveform<double> tempwav;
        tempwav.PushSample(0.3 * ((double)pedsubValue)); // what is 0.3?
        vector<Waveform<double>> Vtempwav;
        Vtempwav.push_back(tempwav);
        LAPPDWaveforms->insert(pair<unsigned long, vector<Waveform<double>>>(channelNo, Vtempwav));
        Log("LAPPDLoadTXT: Inserting waveform at channel " + std::to_string(channelNo), 5, LAPPDLoadTXTVerbosity);
      }
      else
      {
        (((LAPPDWaveforms->find(channelNo))->second).at(0)).PushSample(0.3 * ((double)pedsubValue));
      }

      channelNo++;
    }
    lineNumber++;
    if (sampleNo == 255)
    {
      Log("LAPPDLoadTXT: Event " + std::to_string(eventNo) + " loaded", v_debug, LAPPDLoadTXTVerbosity);
      if (DataFile.peek() == EOF) // 检查下一个字符是否为EOF
      {
        m_data->vars.Set("StopLoop", 1);
        Log("LAPPDLoadTXT: End of data file reached, setting StopLoop to 1", 0, LAPPDLoadTXTVerbosity);
      }
      break;
    }
  }
}

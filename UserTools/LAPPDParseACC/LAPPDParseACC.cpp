#include "LAPPDParseACC.h"

LAPPDParseACC::LAPPDParseACC():Tool(){}

using namespace std;



bool LAPPDParseACC::Initialise(std::string configfile, DataModel &data){


  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  bool isSim = false;
  bool isLoaded = false;
  m_data->Stores["ANNIEEvent"]->Header->Set("isSim",isSim);
  m_data->Stores["ANNIEEvent"]->Set("isLoaded",isLoaded); //are all events loaded into the store? 






  /* Get Pedestal Data */
  string path;
  m_variables.Get("filepath", path);
  string name;
  m_variables.Get("filename", name);
  string filebase = path + "/" + name;

  string pedpath = filebase + ".ped";
  ifstream pfs;
  cout << "Trying to load ped file from " << filebase << endl;
  try {
    pfs.open(pedpath.c_str());
  } catch (...) {
    cout << "Could not load pedestal data file." << endl;
    return false;
  }

  /* Determine Number of boards */
  int board;
  string line;
  getline(pfs, line); //skip header
  while(!pfs.eof()){
    pfs >> board;
    getline(pfs, line);
    // If the vector does not contain the board entry
    if(find(boards.begin(), boards.end(), board) == boards.end()) {
      boards.push_back(board);
      cout << "Found board " << board << endl;
    }
  }
  pfs.close();
  pfs.open(pedpath.c_str());

  map<int, map<int, Waveform<int>>> rawPedData;

  // Skip header
  getline(pfs, line);

  int b, c;
  int key;
  int sample;
  vector<int> samples;
  while(!pfs.eof()){
    pfs >> b;
    pfs >> c;
    for (int i = 0; i < n_cells; i++) {
      pfs >> sample;
      samples.push_back(sample);

    }
    Waveform<int> data;
    data.SetSamples(samples);
    rawPedData[b][c] = data;
  }
  pfs.close();

  m_data->Stores["ANNIEEvent"]->Set("rawPedData", rawPedData);
  /* Open other file streams */

  string datapath = filebase + ".acdc";
  try {
      dfs.open(datapath.c_str());
  } catch (...) {
    cout << "Could not load acdc data file." << endl;
    return false;
  }
  //process the header already
  getline(dfs, line);

  string metapath = filebase + ".meta";
  try {
      mfs.open(metapath.c_str());
  } catch (...) {
    cout << "Could not load acdc data file." << endl;
    return false;
  }
  //process the header already
  getline(mfs, meta_header);


  //Open a calibration file that contains
  //info about PSEC4 electronics; for example:
  //actual sample times based on aperture jitter calibration
  //and time-lengths of striplines given an MCP pulse; for each 
  //board 
  

  string calfilename;
  m_variables.Get("calibfile", calfilename);
  filebase = path + "/" + calfilename;
  ifstream calfs;

  cout << "Trying to load psec4 calibraiton file from " << filebase << endl;
  try {
    calfs.open(filebase.c_str());
  } catch (...) {
    cout << "Could not load psec4 calibration file." << endl;
    return false;
  }

  
  //structure: caldata[board]["cal info"] = vector of floats
  map<int, map<string, vector<float>>> caldata;
  vector<float> tempdata;
  string string_key;
  float temp_bit;
  while(getline(calfs, line))
  {
    istringstream iss(line);
    iss>>string_key; //first column always the key 
    iss>>board; //second column always the board
    while(iss >> temp_bit)
    {
      tempdata.push_back(temp_bit);
    }
    caldata[board][string_key] = tempdata;
    tempdata.clear();
  }

  m_data->Stores["ANNIEEvent"]->Set("psec4caldata", caldata);

  //define the number of samples and number
  //of channels based on this calibration data
  n_cells = caldata[boards.at(0)]["sampletimes"].size();
  n_chs = caldata[boards.at(0)]["striptimes"].size();
  m_data->Stores["ANNIEEvent"]->Set("num_chs", n_chs);
  m_data->Stores["ANNIEEvent"]->Set("num_cells", n_cells);

  return true;
}


//By default loads in all of the data in the entire data file. 
//If Toolchain is doing a loop, it checks to see if the data is 
//already in the datastore. 
bool LAPPDParseACC::Execute(){
  //check if the data has already been loaded. 
  //if not, continue and load the entire data file into the store
  bool isLoaded;

  m_data->Stores["ANNIEEvent"]->Get("isLoaded", isLoaded);
  if(isLoaded==true) return true; // dont load again, just go to the next usertool


  //structure: 
  //rawData[event][board][channel] = Waveform<double> 
  map<int, map<int, map<int, Waveform<double>>>> rawData;



  int event, board, channel;
  string line;
  int line_counter = 0;
  double adccounts_to_mv = 1.2*1000.0/4096.0;

  //start of this loop is just after
  //the header of the datafile
  Waveform<double> tempwav;
  while(getline(dfs, line))
  {
    istringstream iss(line); //the current line in the file
    int temp_bit; //the current integer/bit in the line
    int char_count = -1; //counts the present integer/bit number in the line

    //top of this loop is the start of the line
    //in the data file. 
    while(iss >> temp_bit)
    {
      char_count++;

      //if we are on the event number
      if(char_count == 0)
      {
        event = temp_bit;
        continue;
      }
      if(char_count == 1)
      {
        //this is the board number
        //not implemented yet
        board = temp_bit;
        continue;
      }
      if(char_count == 2)
      {
        //this is the channel number
        channel = temp_bit;
        continue;
      }
      //if none of the above if statements fire
      //then we are in a sample (0 - 255) of ADC counts
      tempwav.PushSample(temp_bit*adccounts_to_mv);
    }

    //check to make sure the size of the tempwav is correct
    int tempsize = tempwav.GetSamples()->size();
    if(tempsize != n_cells)
    {
      cout << "Warning, event " << event << " board " \
      << board << " channel " << channel << \
      " has the incorrect number of samples: " << tempsize << endl;
    }
    //push to data, reinitialize
    rawData[event][board][channel] = tempwav;
    tempwav.ClearSamples();
    line_counter++;
  }

  m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData_ftbf", rawData);
  


  /* Metadata */
  //structure metadata[event][board]["key string"] = unsigned int 

  map<int, map<int, map<string, unsigned int>>> metadata;
  stringstream headers(meta_header);
  string header; //temp variable for the particular header column parsing the stringstream headers
  vector<string> header_vec; //a vector containing header column info
  bool first_loop = true; //flag to fill the header_vec only the first time around
  while(getline(mfs, line))
  {
    istringstream iss(line); //the current line in the file
    unsigned int temp_bit;
    int char_count = -1;
    while(iss >> temp_bit)
    {
      headers >> header;
      if(first_loop) header_vec.push_back(header);
      char_count++;

      //if we are on the event number
      if(char_count == 0)
      {
        event = temp_bit;
        continue;
      }
      if(char_count == 1)
      {
        //this is the board number
        //not implemented yet
        board = temp_bit;
        continue;
      }
      //if none of the above if statements fire
      //then we are in a metadata key
      metadata[event][board][header_vec.at(char_count)] = temp_bit;
    }
    first_loop = false;
  }

  m_data->Stores["ANNIEEvent"]->Header->Set("metaData", metadata);


  


  isLoaded = true;
  m_data->Stores["ANNIEEvent"]->Set("isLoaded", isLoaded); //have loaded the entire data file

  return true;
}


bool LAPPDParseACC::Finalise(){
  dfs.close();
  mfs.close();

  return true;
}

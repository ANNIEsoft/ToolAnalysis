#include "LAPPDParseACC.h"

LAPPDParseACC::LAPPDParseACC():Tool(){}

using namespace std;



bool LAPPDParseACC::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  m_data->Stores["ANNIEEvent"] = new BoostStore(false, 2);

  /* Get Pedestal Data */
  string path;
  m_variables.Get("filepath", path);
  string name;
  m_variables.Get("filename", name);
  string filebase = path + "/" + name;

  string pedpath = filebase + ".ped";
  ifstream pfs;
  try {
    pfs.open(pedpath.c_str());
  } catch (...) {
    cout << "Could not load pedestal data file." << endl;
    return false;
  }

  /* Determine Number of boards */
  string line;
  int board;
  // Skip header
  getline(pfs, line);
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

  map<int, vector<Waveform<double>>> rawPedData;

  // Skip header
  getline(pfs, line);

  int b, c;
  int key;
  int samples[NUM_CELLS];
  while(!pfs.eof()){
    pfs >> b;
    pfs >> c;
    key = b << 4 | c;
    vector<Waveform<double>> in;
    for (int i = 0; i < NUM_CELLS; i++) {
      pfs >> samples[i];
    }
    Waveform<double> data;
    std::vector<double> v(begin(samples), end(samples));
    data.SetSamples(v);
    in.push_back(data);
    rawPedData[key] = in;
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
  getline(dfs, line);

  string metapath = filebase + ".meta";
  try {
      mfs.open(metapath.c_str());
  } catch (...) {
    cout << "Could not load acdc data file." << endl;
    return false;
  }
  getline(mfs, meta_header);

  event = 0;
  return true;
}


bool LAPPDParseACC::Execute(){
  if (dfs.eof() || mfs.eof()) m_data->vars.Set("StopLoop", 1);

  map<int, vector<Waveform<double>>> rawData;
  /* Data */
  int e, b, c, key;
  int samples[NUM_CELLS];
  for(int k = 0; k < NUM_CHS * boards.size(); k++){
    dfs >> e; // Event, we can skip this
    dfs >> b;
    dfs >> c;
    key = b << 4 | c;
    vector<Waveform<double>> in;
    for (int i = 0; i < NUM_CELLS; i++) {
      dfs >> samples[i];
    }
    Waveform<double> data;
    std::vector<double> v(begin(samples), end(samples));
    data.SetSamples(v);
    in.push_back(data);
    rawData[key] = in;
  }
  m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData", rawData);

  /* Metadata */
  map<int, map<string, double>> meta;
  stringstream headers(meta_header);
  string header;
  for (int k = 0; k < boards.size(); k++) {
    map<string, double> values;
    mfs >> e; // Event, we can skip this
    mfs >> key;
    for (int m = 0; headers >> header; m++) {
      if (m < 2) continue;
      mfs >> values[header];
    }
    meta[key] = values;
  }
  m_data->Stores["ANNIEEvent"]->Header->Set("metaData", meta);

  bool print;
  m_variables.Get("print", print);
  if (print){
      m_data->Stores["ANNIEEvent"]->Print();
  }
  event++;
  return true;
}


bool LAPPDParseACC::Finalise(){
  dfs.close();
  mfs.close();

  return true;
}

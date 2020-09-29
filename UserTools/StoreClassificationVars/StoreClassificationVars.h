#ifndef StoreClassificationVars_H
#define StoreClassificationVars_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>

#include "Tool.h"
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"

#include "Hit.h"
#include "LAPPDHit.h"
#include "Position.h"
#include "Direction.h"
#include "RecoVertex.h"
#include "RecoDigit.h"


class StoreClassificationVars: public Tool {

 public:

  StoreClassificationVars();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  bool LoadVariableConfig(std::string config_name);
  bool LoadHistConfig(std::string histogram_config);
  void InitClassHistograms();
  void InitClassTree();
  void InitCSV();
  void FillClassificationVars(std::vector<std::string> variable_vector, bool isMC);
  void WriteClassHistograms();
  void EvaluatePionEnergies();

 private:

  // Configuration variables 
  int verbosity=0;
  std::string filename;
  bool save_root;
  bool save_csv;
  std::string variable_config;
  std::string variable_config_path;
  std::string histogram_config;
  bool EventCutStatus;
  bool selection_passed;
  bool mldata_present;
  bool isData;
  int i_loop = 0;

  // Variable names
  std::vector<std::string> variable_names;
  std::vector<std::string> mc_names;
  std::vector<std::string> vec_names;

  // Classification maps
  std::map<std::string,int> classification_map_int;
  std::map<std::string,double> classification_map_double;
  std::map<std::string,bool> classification_map_bool;
  std::map<std::string,std::vector<double>> classification_map_vector;
  std::map<std::string,int> classification_map_int_copy;
  std::map<std::string,double> classification_map_double_copy;
  std::map<std::string,bool> classification_map_bool_copy;
  std::map<std::string,std::vector<double>*> classification_map_vector_pointer;
  std::map<std::string,int> classification_map_map;

  // Pion energy map
  std::map<int,std::vector<double>> map_pion_energies;
  int n_neutrons; 
 
  // Histogram configuration maps
  std::map<std::string,int> n_bins;
  std::map<std::string,double> min_bins;
  std::map<std::string,double> max_bins;

  // Files to save data in
  
  TFile *file = nullptr;
  ofstream csv_file, csv_statusfile, csv_pion_energies;

  // Classification variables - TTree
  TTree *tree = nullptr;

  // Classification variables - histograms (1D)
  std::map<std::string,TH1F*> vector_hist; 


  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  int get_ok;

};


#endif

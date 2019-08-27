 
#ifndef CNNImage_H
#define CNNImage_H

#include <string>
#include <fstream>

#include "TH2F.h"
#include "TMath.h"
#include "TFile.h"

#include "Tool.h"


/**
 * \class CNNImage
 *
 * The tool CNNImage is supposed to create custom csv input files for CNN classification processes. The framework relies on the EventDisplay tool and basically feeds that geometric information of the hit pattern into a matrix format
*
* $Author: M.Nieslony $
* $Date: 2019/08/09 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/

class CNNImage: public Tool {


 public:

  CNNImage(); ///< constructor for CNNImage class
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void draw_cnn_image(); ///< Fill the hit pattern information in a TH2

 private:

  std::string cnn_outpath; //path where to save the CNN image information
  std::string detector_config;
  int verbosity;
  std::string mode;     //Charge, Time
  int dimension;        //dimension of the CNN image (e.g. 32, 64)
  int runnumber;
  int subrunnumber;
  int evnum;
  TimeClass* EventTime=nullptr;

  std::vector<MCParticle>* mcparticles = nullptr;
  std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
  std::map<unsigned long, std::vector<MCLAPPDHit>>* MCLAPPDHits = nullptr;
  Geometry *geom = nullptr;

  double tank_radius;
  double tank_height;
  double tank_center_x;
  double tank_center_y;
  double tank_center_z;
  double min_y;
  double max_y;
  int n_tank_pmts;
  double size_top_drawing = 0.1;

  ofstream outfile;

  Position truevtx;
  double truevtx_x, truevtx_y, truevtx_z;

  std::map<int, double> x_pmt, y_pmt, z_pmt;
  std::map<unsigned long,double> charge;
  std::map<unsigned long, double> time;
  TFile *file = nullptr;

  double maximum_pmts;
  double total_charge_pmts;
  int total_hits_pmts;
  double min_time_pmts;
  double max_time_pmts;

  std::map<unsigned long, int> channelkey_to_pmtid;
  std::map<int,unsigned long> pmt_tubeid_to_channelkey;
  std::vector<unsigned long> pmt_detkeys;
  std::vector<unsigned long> hitpmt_detkeys;

};


#endif

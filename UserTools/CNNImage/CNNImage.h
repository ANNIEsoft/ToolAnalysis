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
  void ConvertPositionTo2D(Position xyz_pos, double &x, double &y); ///<Convert 3D position into 2D rolled up coordinates
  void ConvertPositionTo2D_Top(Position xyz_pos, double &x, double &y); ///<Convert 3D position into 2D rolled up coordinates (top PMTs only)
  void ConvertPositionTo2D_Bottom(Position xyz_pos, double &x, double &y); ///<Convert 3D position into 2D rolled up coordinates (bottom PMTs only)


 private:

  //configuration variables
  std::string cnn_outpath; //path where to save the CNN image information
  std::string detector_config;
  std::string data_mode;  //Charge, Time
  std::string save_mode;  //How is the PMT information supposed to be written out? Geometric/PMT-wise
  int dimensionX;        //dimension of the CNN image in x-direction
  int dimensionY;        //dimension of the CNN image in y-direction
  int dimensionLAPPD;    //dimension of LAPPD CNN images (both directions, square)
  bool includeTopBottom;
  int verbosity;
  int dimensionLAPPD;
  //ANNIEEvent variables
  int runnumber;
  int subrunnumber;
  int evnum;
  TimeClass* EventTime=nullptr;
  std::vector<MCParticle>* mcparticles = nullptr;
  std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
  std::map<unsigned long, std::vector<MCLAPPDHit>>* MCLAPPDHits = nullptr;
  Geometry *geom = nullptr;
  int mrdeventcounter;
  std::map<unsigned long, std::vector<MCHit>>* TDCData = nullptr;

  // RecoEvent
  int nrings;

  //geometry variables
  double tank_radius;
  double tank_height;
  double tank_center_x;
  double tank_center_y;
  double tank_center_z;
  double min_y, max_y, min_y_lappd, max_y_lappd;
  int n_tank_pmts;
  double size_top_drawing = 0.1;
  std::vector<double> vec_pmt2D_x, vec_pmt2D_x_Top, vec_pmt2D_y_Top, vec_pmt2D_x_Bottom, vec_pmt2D_y_Bottom, vec_pmt2D_y, vec_lappd2D_x, vec_lappd2D_y;
  int npmtsX, npmtsY, nlappdX, nlappdY;

  //I/O variables
  ofstream outfile, outfile_time, outfile_lappd, outfile_lappd_time, outfile_Rings, outfile_MRD;
  TFile *file = nullptr;

  //mctruth information
  Position truevtx;
  double truevtx_x, truevtx_y, truevtx_z;

  //PMT information
  std::map<int, double> x_pmt, y_pmt, z_pmt, x_lappd, y_lappd, z_lappd;
  std::map<unsigned long,double> charge, time, total_charge_lappd;
  std::map<unsigned long, std::vector<std::vector<double>>> charge_lappd, time_lappd;
  std::map<unsigned long, std::vector<std::vector<int>>> hits_lappd;
  double maximum_pmts, maximum_lappds;
  double total_charge_pmts, total_charge_lappds;
  int total_hits_pmts, total_hits_lappds;
  double min_time_pmts, max_time_pmts, min_time_lappds, max_time_lappds;

  //detectorkey layout organization
  std::map<unsigned long, int> channelkey_to_pmtid;
  std::map<int,unsigned long> pmt_tubeid_to_channelkey;
  std::vector<unsigned long> pmt_detkeys, lappd_detkeys, pmt_chankeys;
  std::vector<unsigned long> hitpmt_detkeys;

};


#endif

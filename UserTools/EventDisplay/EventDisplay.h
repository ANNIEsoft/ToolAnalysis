#ifndef EventDisplay_H
#define EventDisplay_H

#include <string>
#include <iostream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "Tool.h"
#include "TEllipse.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TBox.h"
#include "TColor.h"
#include "TLegend.h"
#include "TPolyMarker.h"
#include "TPolyLine.h"
#include "TPaveText.h"
#include "TPaveLabel.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMath.h"
#include "TMarker.h"
#include "BeamStatus.h"
#include "TriggerClass.h"
#include "Detector.h"
#include "Geometry.h"
#include "Hit.h"
#include "Position.h"
#include "Direction.h"
#include "LAPPDHit.h"
#include "TList.h"
#include "TFile.h"
#include "TLatex.h"


class EventDisplay: public Tool {

 public:

  EventDisplay();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  void make_gui();
  void draw_event();
  void draw_event_box();
  void draw_particle_legend();
  void draw_pmt_legend();
  void draw_lappd_legend();
  void draw_mrd_legend();
  void draw_event_PMTs();
  void draw_event_LAPPDs();
  void draw_event_MRD();
  void draw_true_vertex(Position trueVtx, Direction trueDir, int ring_color);
  void draw_true_ring(Position trueVtx, Direction trueDir, int ring_color);
  void draw_barycenter(Position posBary);
  void delete_canvas_contents();
  void draw_schematic_detector();
  void set_color_palette();
  void translate_xy(double vtxX, double vtxY, double vtxZ, double &xWall, double &yWall, int &status_hit, double &phi_calc);
  void find_projected_xyz(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double &projected_x, double &projected_y, double &projected_z);
  void ParseUserInput(std::string user_string);

 private:


    //define config variables
    int verbose;
    int single_event; //-999: loop over all events, any other number: only show specific event
    std::string mode;   //Charge, Time
    std::string EventType; //ANNIEEvent, RecoEvent
    std::string str_event_list;
    bool selected_event; //only show events that passed EventSelector cuts
    bool text_box; //true: show statistics box, false: don't show statistics box
    double threshold; //only display events with time/charge > threshold
    double threshold_lappd; //only displays LAPPD events with time/charge > threshold
    double threshold_time;
    double threshold_time_lappd;
    double threshold_time_low;
    double threshold_time_high;
    double threshold_time_low_mrd;
    double threshold_time_high_mrd;
    int lappds_selected;  //0-all LAPPDs, 1-only specified LAPPDs
    std::string lappds_file;  //file specifying single PMTs
    bool draw_vertex;
    bool draw_ring;
    bool save_plots;  //variable to decide whether plots will be saved or not
    bool draw_histograms; //variable to decide whether charge/time histograms are drawn in addition
    bool draw_mrd;
    std::string out_file; //name for output file 
    bool user_input;
    bool use_tapplication;
    double marker_size;
    std::string output_format;
    bool isData;
    std::string user_trigger_label;
    std::string string_date_label;
    std::string histogram_config;
    int npmtcut;
    bool draw_cluster;
    bool draw_cluster_mrd;
    std::string charge_format;
    std::string singlePEgains;
    int user_run_number = -1;
    int user_run_type;
    bool draw_ring_temp;
    bool draw_vertex_temp;
    bool use_filtered_digits;

    //define event variables
    uint32_t evnum;
    int mcevnum;
    int runnumber;
    int subrunnumber;
    int terminate_execution;  //for continuous execution of multiple events, prompt user input
    uint16_t MCTriggernum;
    ULong64_t EventTankTime;
    int RunType;
    std::map<unsigned long,vector<MCHit>>* TDCData=nullptr;
    std::map<unsigned long,vector<Hit>>* TDCData_Data=nullptr;
    TimeClass* EventTime=nullptr;
    BeamStatusClass* BeamStatus=nullptr;
    std::vector<TriggerClass>* TriggerData;
    std::vector<MCParticle>* mcparticles=nullptr;
    std::map<unsigned long, std::vector<MCHit>>* MCHits=nullptr;
    std::map<unsigned long, std::vector<Hit>>* Hits=nullptr;
    std::map<unsigned long, std::vector<MCLAPPDHit>>* MCLAPPDHits=nullptr;
    std::map<unsigned long, std::vector<LAPPDHit>>* LAPPDHits=nullptr;
    std::map<double,std::vector<Hit>>* m_all_clusters;  //from ClusterFinder tool
    std::map<double,std::vector<unsigned long>>* m_all_clusters_detkey;  //from ClusterFinder tool
    std::vector<std::vector<int>> MrdTimeClusters;  //from TimeClustering tool
    std::vector<double> MrdDigitTimes;  //from TimeClustering tool
    std::vector<unsigned long> mrddigitchankeysthisevent;  //from TimeClustering tool
    RecoVertex *TrueVertex = nullptr;
    RecoVertex *TrueStopVertex = nullptr;
    bool EventCutStatus;
    std::vector<RecoDigit>* RecoDigits = nullptr; 
    double TrueMuonEnergy;
    int NumMrdTimeClusters;
    int nrings;
    bool passed_selection_cuts;
    std::vector<unsigned int> particles_ring;
    std::map<int,std::string> map_runtypes;
    int i_loop;
    boost::posix_time::ptime *Epoch;
    std::vector<int> ev_list;
    std::map<unsigned long,double> pmt_gains;
    double thetaC = 0.7505;    //43 degrees in rad (Cherenkov angle in water)
    std::string trigger_label;
    std::map<int,int> active_lappds_user;  // WCSim LAPPD IDs read from config file
    std::map<int,int> active_lappds;       // converted to detectorkey
    long cluster_time;


    //geometry variables
    Geometry *geom = nullptr;
    Detector det;
    double tank_height;
    double tank_radius;
    int max_num_lappds = 200;              //one is allowed to dream, right?
    int n_tank_pmts;
    int n_mrd_pmts;
    int n_veto_pmts;
    int n_lappds;
    double max_y;
    double min_y;
    double min_mrd_y, max_mrd_y, min_mrd_x, max_mrd_x, min_mrd_z, max_mrd_z;
    double mrd_diffz, mrd_diffy, mrd_diffx;
    std::vector<int> n_particles_ring;
    double detector_version;
    std::string detector_config;
    double tank_center_x;
    double tank_center_y;
    double tank_center_z;
    std::map<int, double> x_pmt, y_pmt, z_pmt;          //143 currently max configuration with additional 2 inch PMTs
    std::map<unsigned long, std::vector<double>> mrd_x, mrd_y, mrd_z;
    std::map<unsigned long, int> mrd_orientation, mrd_half, mrd_side;

    //bool variables for schematic hit plot
    bool facc_hit;
    bool tank_hit;
    bool mrd_hit;

    //variables for graphical visualization
    TPad *p1 = nullptr;
    TEllipse *top_circle = nullptr;
    TEllipse *bottom_circle = nullptr;
    TBox *box = nullptr;
    TBox *schematic_facc = nullptr;
    TEllipse *schematic_tank = nullptr;
    TBox *schematic_mrd = nullptr;
    TBox *border_schematic_facc = nullptr;
    TBox *border_schematic_mrd = nullptr;
    TPaveLabel *pmt_title = nullptr;
    TPaveLabel *max_text = nullptr;
    TPaveLabel *min_text = nullptr;
    TPaveLabel *lappd_title = nullptr;
    TPaveLabel *max_lappd = nullptr;
    TPaveLabel *min_lappd = nullptr;
    TPaveLabel *mrd_title = nullptr;
    TPaveLabel *max_mrd = nullptr;
    TPaveLabel *min_mrd = nullptr;
    TText *schematic_facc_label = nullptr;
    TText *schematic_tank_label = nullptr;
    TText *schematic_mrd_label = nullptr;
    TLatex *legend_mu = nullptr;
    TLatex *legend_e = nullptr;
    TLatex *legend_pi = nullptr;
    TLatex *legend_pi0 = nullptr;
    TLatex *legend_k = nullptr;
    TText *title_mrd_side = nullptr;
    TText *title_mrd_top = nullptr;
    std::map<unsigned long, TBox*> mrd_paddles, box_mrd_paddles;


    //chankey WCSim ID mappings
    std::map<int,unsigned long> lappd_tubeid_to_detectorkey;
    std::map<unsigned long,int> channelkey_to_pmtid;
    std::map<int,unsigned long> pmt_tubeid_to_channelkey;
    std::map<unsigned long,int> channelkey_to_mrdpmtid;
    std::map<unsigned long,int> channelkey_to_faccpmtid;
    static const unsigned long n_channels_per_lappd = 60;


    //Container holding charge/timing information
    std::map<unsigned long,double> charge;
    std::map<unsigned long,double> time;
    std::map<unsigned long,int> hits;
    std::map<unsigned long,double> charge_lappd;
    std::map<unsigned long,std::vector<Position>> hits_LAPPDs;
    std::map<unsigned long,std::vector<double>> time_lappd;
    std::map<unsigned long,double> mrddigittimesthisevent;


    //max and min values for color scale
    double maximum_pmts;
    double maximum_lappds;
    int total_hits_pmts;
    int total_hits_lappds;
    int num_lappds_hit;
    double total_charge_pmts;
    double maximum_time_pmts;
    double maximum_time_lappds;
    double maximum_time_mrd;
    double maximum_time_overall;
    double min_time_pmts;
    double min_time_lappds;
    double min_time_mrd;
    double min_time_overall;
    double max_cluster_time;
    double min_cluster_time;

    //sizes for drawings
    double size_top_drawing=0.1;
    double size_schematic_drawing=0.05;
    int color_marker;

    //canvas 
    TFile *root_file = nullptr;
    TCanvas *canvas_ev_display;
    TCanvas *canvas_pmt;
    TCanvas *canvas_pmt_supplementary;
    TCanvas *canvas_lappd;
    TPaveText* text_event_info = nullptr;

    //markers
    std::vector<TMarker*> vector_colordot;
    std::vector<TMarker*> vector_colordot_lappd;
    std::vector<TMarker*> vector_colordot_mrd;
    std::vector<TPolyMarker*> marker_pmts_top;
    std::vector<TPolyMarker*> marker_pmts_bottom;
    std::vector<TPolyMarker*> marker_pmts_wall;
    std::vector<TPolyMarker*> marker_lappds;
    std::vector<TPolyMarker*> marker_mrd;
    TPolyMarker *marker_vtx = nullptr;
    TPolyMarker *marker_bary = nullptr;
    TPolyLine *ring_visual[10] = {};     //individual ring segments for parts of the ring that change between top/bottom/wall
    int current_n_polylines = 0;

    //histograms
    std::vector<unsigned long> lappd_detkeys;
    std::vector<unsigned long> pmt_detkeys;
    std::vector<unsigned long> hitpmt_detkeys;
    std::vector<unsigned long> mrd_detkeys;
    std::vector<unsigned long> hitmrd_detkeys;
    TH1F *time_PMTs = nullptr;
    std::map<int,TH1F*> time_LAPPDs;	//the keys for the LAPPD histogram maps are the indices of the lappd_detkeys vector
    TH1F *charge_PMTs = nullptr;
    TH2F *charge_time_PMTs = nullptr;
    std::map<int,TH1F*> charge_LAPPDs;
    double pmt_Qmax, pmt_Qmin, pmt_Tmax, pmt_Tmin, lappd_Qmax, lappd_Qmin, lappd_Tmax, lappd_Tmin;
    int pmt_Qbins, pmt_Tbins, lappd_Qbins, lappd_Tbins;

    //legends
    TLegend *leg_charge = nullptr;
    TLegend *leg_time = nullptr;

    //variables regarding true interaction vertex and expected ring
    Position truevtx;
    Direction truedir;
    double truevtx_x, truevtx_y, truevtx_z;
    double truedir_x, truedir_y, truedir_z;
    double vtxproj_x, vtxproj_y, vtxproj_z;
    static const int num_ring_points = 500;
    double xring[10][num_ring_points], yring[10][num_ring_points];
    std::vector<Position> particles_truevtx;        //Vtx Positions for all particles contributing to a ring
    std::vector<Direction> particles_truedir;       //Direction for all particles contributing to a ring
    std::vector<int> particles_color;               //Ring colors for all particles contributing to a ring
    std::vector<int> particles_pdg;		    //PDG codes for all particles contributing to a ring
    std::map<int,int> pdg_to_color;                 //color scheme for primary particles


    //color palettes (kBird color palette)
    const int n_colors=255;
    double alpha=1.;	//make colors opaque, not transparent
    Int_t Bird_palette[255];
    Int_t Bird_Idx;
    Double_t stops[9] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000};
    Double_t red[9]   = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764};
    Double_t green[9] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832};
    Double_t blue[9]  = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539};

    //TApplication for visual implementation
    TApplication *app_event_display = nullptr;

    //verbosity options
    int v_error = 0;
    int v_warning = 1;
    int v_message = 2;
    int v_debug = 3;

};


#endif

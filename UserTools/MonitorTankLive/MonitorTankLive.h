#ifndef MonitorTankLive_H
#define MonitorTankLive_H

#include <string>
#include <map>
#include <iostream>

#include "Tool.h"

#include "TCanvas.h"
#include "TH1I.h"
#include "TH1F.h"
#include "TH2I.h"
#include "TH2F.h"
#include "TGraph2D.h"
#include "TLegend.h"
#include "TF1.h"
#include "TThread.h"
//#include "PMTOut.h"						//include when data format is ready
#include "TPaletteAxis.h"
#include "TPaveText.h"
#include "TRandom3.h"
#include "TLatex.h"
#include "TList.h"
#include "TObject.h"
#include "TTree.h"
#include "TFile.h"

#include "TObjectTable.h"

#include "TROOT.h"

/**
 * \class MonitorTankLive
*
* $Author: M. Nieslony $
* $Date: 2019/08/07 12:55:00 $
* Contact: mnieslon@uni-mainz.de
*/

class MonitorTankLive: public Tool {


  public:

    MonitorTankLive(); ///< Simple constructor
    bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute(); ///< Execute function used to perform Tool purpose. 
    bool Finalise(); ///< Finalise funciton used to clean up resources.

    void InitializeHists(); ///< Function to initialize all histograms and canvases
    void TankPlots(); ///< Function to fill histograms and canvases for PMT Live monitoring

  private:

    //config input variables
    BoostStore *PMTData;					//store containing the PMT raw data information
    std::string outpath;					//output path to write produced monitoring plots
    //PMTOut PMTout;      				//the class that has all the information about the pmt raw data format					//include when data format is ready
    std::string active_slots;				//file containing the position of active cards within the VME crates
    int verbosity;						//levels: 1-very general information, 2-detail with store contents, 3-Status variable (Wait/DataFile/PMTSingle) 

    const int num_crates_tank = 3;      //3 VME crates distributed over racks 5 and 6
    const int num_slots_tank = 21;      //VME crate has 21 slots
    const int num_channels_tank = 4;    //each VME card has up to 4 usable channels

    std::vector<int> crate_numbers;		//vector container to store the numbers of the employed VME crates
    //several containers storing the position of active cards within the VME crates follow
    int active_channel_cr1[21]={0};		
    int active_channel_cr2[21]={0};
    int active_channel_cr3[21]={0};
    int n_active_slots_cr1, n_active_slots_cr2, n_active_slots_cr3;
    int num_active_slots;
    std::vector<int> active_slots_cr1;
    std::vector<int> active_slots_cr2;
    std::vector<int> active_slots_cr3;
    std::map<std::pair<int,int>,int> map_crateslot_vector;	//map crate & slot id to the position in the data storing vectors

    time_t t;
    std::stringstream title_time;         //for plotting the date & time on each monitoring plot
    int maximum_adc = 400;				//define x-borders of ADC histogram plots
    int minimum_adc = 200;
    int buffersize;
    bool init;
    TH2F *h2D_ped, *h2D_sigma, *h2D_rate; //define 2D histograms to show the current rates, pedestal values, sigma values
    TH2F *h2D_pedtime, *h2D_sigmatime, *h2D_pedtime_short, *h2D_sigmatime_short;
    std::vector<TH1F*> hChannels_temp;	//temp plots for each channel
    std::vector<TH1I*> hChannels_freq;		//frequency plots for each channel
    TCanvas *canvas_ped, *canvas_sigma, *canvas_rate, *canvas_pedtime, *canvas_sigmatime, *canvas_pedtime_short, *canvas_sigmatime_short;
    std::vector<TCanvas*> canvas_Channels_temp;
    std::vector<TCanvas*> canvas_Channels_freq;

    std::vector<int> channels_rates;	//map of PMT VME channel # to the rate of the respective PMT
    std::vector<unsigned long> channels_timestamps;
    std::vector<double> channels_mean;
    std::vector<double> channels_sigma;

    std::vector<std::vector<double>> timeev_ped;
    std::vector<std::vector<double>> timeev_sigma; 
    std::vector<TF1*> vector_gaus;

    //PMT store PMTOut (presumably) includes the following variables (inferred from exemplary phase I raw data root tree)
    int SequenceID, StartTimeSec, StartTimeNSec, BufferSize, FullBufferSize, EventSize, TriggerNumber;
    long StartCount;
    std::vector<int> CrateID, CardID, Channels, TriggerCounts, Rates;
    std::vector<unsigned short> Data;

};

#endif

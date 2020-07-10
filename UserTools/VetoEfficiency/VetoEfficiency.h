/* vim:set noexpandtab tabstop=4 wrap */
#ifndef VetoEfficiency_H
#define VetoEfficiency_H

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <set>

#include "Tool.h"
#include "TH2F.h"

class TFile;
class TTree;
class TH1F;
class TCanvas;
class TApplication;

class ADCPulse;

// a struct to contain coincidence events
struct CoincidenceInfo {
	CoincidenceInfo() {};
	std::map<unsigned long,std::vector<double>> vetol1hits;
	std::map<unsigned long,std::vector<double>> vetol2hits;
	std::map<unsigned long,std::vector<double>> mrdl1hits;
	std::map<unsigned long,std::vector<double>> mrdl2hits;
	double tank_charge=0;
	int num_unique_water_pmts=0;
	double event_time_ns=std::numeric_limits<double>::max();
};

/**
 * \class VetoEfficiency
 *
 * A tool to measure the efficiency of second veto layer PMTs, using phase I data.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class VetoEfficiency: public Tool {
	
	public:
	
	VetoEfficiency(); ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute(); ///< Execute function used to perform Tool purpose.
	bool Finalise(); ///< Finalise function used to clean up resources.
	
	private:
	std::string outputfilename;
	std::string debugfilename;
	TFile* rootfileout=nullptr;
	TTree* roottreeout=nullptr;
	TTree* summarytreeout=nullptr;
	TFile* f_veto=nullptr;		//Additional debug TFile
	bool useTApplication;

	double compute_tank_charge(size_t minibuffer_number,
	const std::map< unsigned long, std::vector< std::vector<ADCPulse> > >& adc_hits,
	uint64_t start_time, uint64_t end_time, int& num_unique_water_pmts, int& num_pmts_above);
	bool found_coincidence;	

	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
	// some wrapper functions used by PhaseITreeMaker tool code,
	// just to roll in error checking for retrieving objects from BoostStores
	template <typename T, typename AStore> bool get_object_from_store(
		const std::string& object_label, T& obj, AStore& s){
		Log("Retrieving \"" + object_label + "\" from a Store", 4, verbosity);
		bool got_object = s.Get(object_label, obj);
	
		// Check for problems
		if ( !got_object ) {
			Log("Error: The VetoEfficiency tool could not find the " + object_label
				+ " entry", 0, verbosity);
			return false;
		}
		return true;
	}
	
	template <typename T> inline auto check_that_not_empty(
		const std::string& object_label, T& obj)
		-> decltype( std::declval<T&>().empty() ){
		bool is_empty = obj.empty();
		if ( is_empty ) {
			Log("Error: The VetoEfficiency tool found an empty " + object_label
				+ " entry", 0, verbosity);
		}
		return !is_empty;
	}
	
	///////////////////
	// misc recorded variables for each event
	bool hefty_mode_ = false;
	int hefty_trigger_mask_ = 0;
	uint8_t event_label_ = 0;
	
	uint32_t run_number_ = 0;
	uint32_t subrun_number_ = 0;
	uint32_t event_number_ = 0;
	uint32_t minibuffer_number_ = 0;
	uint32_t spill_number_ = 0;
	
	double event_time_ns_;
	std::vector<unsigned long> veto_l1_ids_; // contains ids of the upstream veto PMTs hit this evt
	std::vector<unsigned long> veto_l2_ids_; // contains ids of the upstream veto PMTs hit this evt
	int num_veto_l1_hits_;
	int num_veto_l2_hits_;
	int num_mrdl1_hits_;
	int num_mrdl2_hits_;
	int coincidence_layer_;
	double tank_charge_;
	int num_unique_water_pmts_;
	CoincidenceInfo coincidence_info_;
	
	// aggregate variables stored to summary tree
	ULong64_t total_POT_ = 0;  // ROOT can't store unsigned longs unless they're *its* version. ¬_¬
	ULong64_t num_source_triggers_ = 0;
	ULong64_t num_cosmic_triggers_ = 0;
	ULong64_t num_soft_triggers_ = 0;
	bool is_cosmic;
        // aggregate count of L1 and L2 hits in through-going events
	std::vector<unsigned long> l1_hits_L1_{0,0,0,0,0,0,0,0,0,0,0,0,0};   // 13, one for each PMT in the veto layers (coincidence condition with L1)
	std::vector<unsigned long> l2_hits_L1_{0,0,0,0,0,0,0,0,0,0,0,0,0};   // 13, one for each PMT in the veto layers (coincidence condition with L1)
	std::vector<double> l2_efficiencies_{0,0,0,0,0,0,0,0,0,0,0,0,0};
	std::vector<unsigned long> l1_hits_L2_{0,0,0,0,0,0,0,0,0,0,0,0,0};   // 13, one for each PMT in the veto layers (coincidence condition with L2)
	std::vector<unsigned long> l2_hits_L2_{0,0,0,0,0,0,0,0,0,0,0,0,0};   // 13, one for each PMT in the veto layers (coincidence condition with L2)
	std::vector<double> l1_efficiencies_{0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	// vector of the actual coincident event details
	std::vector<CoincidenceInfo> coincidences_;		//Coincidence condition with veto layer 1
	std::vector<CoincidenceInfo> coincidencesl2_;		//Coincidence condition with veto layer 2
	
	// Configuration variables for the event selection
	// ============================================
	/// @brief The required number of unique water PMTs
	/// to consider a tank event to have happened
	uint min_unique_water_pmts_ = 0;
	
	/// @brief The required tank charge (in nC)
	// to consider a tank event to have happened
	double min_tank_charge_ = 0.;
	
	/// @brief Alternatively, the required number of
	/// unique MRD L1 PMTs to consider a MRD L1 event
	/// to have happened
	uint min_mrdl1_pmts_ = 0;
	
	/// @brief And the required number of unique MRD L2
	/// PMTs to consider a MRD L2 event to have happened
	/// A coincidence requires both MRD L1 and MRD L2
	uint min_mrdl2_pmts_ = 0;
	
	/// @brief The window length defining the time in which
	/// events must occur to be considered "in coincidence"
	int coincidence_tolerance_ = 0;
	
	/// @brief The duration of pretrigger, before the upstream
	/// veto layer hit, at which to place the event start time
	/// this accounts for scintillation, propagation, PMT transit time
	/// cable delay etc between the upstream veto hit timestmap
	/// and when we define the "start" of the event
	int pre_trigger_ns_ = 0;
	
	// information about the event, extracted from the CoincidenceInfo struct
	// (so that we dont have to deal with structs in the ROOT tree)
	
	// list of phase1 veto and MRD PMT IDs that were populated
	// this is a vector of strings of the format XXYYZZ
	// where XX is the X location, YY .. etc.
	std::vector<std::string> phase1mrdpmts{"1000002", "1000102", "1000202", "1000302", "1000402", "1000502", "1000602", "1000702", "1000802", "1000902", "1001002", "1001102", "1001202", "1010002", "1010102", "1010202", "1010302", "1010402", "1010502", "1010602", "1010702", "1010802", "1010902", "1011002", "1011102", "1011202", "1000003", "1010003", "1020003", "1030003", "1040003", "1050003", "1060003", "1070003", "1080003", "1090003", "1100003", "1110003", "1120003", "1130003", "1000103", "1010103", "1020103", "1030103", "1040103", "1050103", "1060103", "1070103", "1080103", "1090103", "1100103", "1110103", "1120103", "1130103", "1140103", "1000000", "1000100", "1000200", "1000300", "1000400", "1000500", "1000600", "1000700", "1000800", "1000900", "1001000", "1001100", "1001200", "1010000", "1010100", "1010200", "1010300", "1010400", "1010500", "1010600", "1010700", "1010800", "1010900", "1011000", "1011100", "1011200"};
	
	std::vector<unsigned long> vetol1keys;
	std::vector<unsigned long> vetol2keys;
	std::vector<unsigned long> mrdl1keys;
	std::vector<unsigned long> mrdl2keys;
	std::vector<unsigned long> allmrdkeys;
	// populate the above
	void LoadTDCKeys();
	std::vector<double> veto_times;	
	std::vector<unsigned long> veto_chankeys;
	std::vector<double> veto_times_layer2;	
	std::vector<unsigned long> veto_chankeys_layer2;

	// make the output ROOT file, tree, branches etc
	void makeOutputFile(std::string name);
	
	// Geometry - used in compute_tank_charge
	Geometry* anniegeom=nullptr;
	
	// debug plots
	bool drawHistos=false;
	std::string plotDirectory=".";
	TCanvas* plotCanv=nullptr;
	TH1F* h_tdc_times=nullptr;
	TH1F* h_tdc_delta_times=nullptr;
	TH1F* h_tdc_delta_times_L1=nullptr;
	TH1F* h_tdc_delta_times_L2=nullptr;
	TH1F* h_tdc_chankeys_l1=nullptr;
	TH1F* h_tdc_chankeys_l2=nullptr;
	TH2F* h_tdc_chankeys_l1_l2=nullptr;
	TH1F* h_veto_delta_times = nullptr;
	TH1F* h_veto_delta_times_coinc = nullptr;
	TH2F* h_veto_2D=nullptr;
	TH1F* h_adc_times=nullptr;
	TH1F* h_adc_delta_times=nullptr;
	TH2F* h_adc_delta_times_charge = nullptr;
	TH1F* h_adc_charge=nullptr;
	TH1F* h_adc_charge_coinc=nullptr;
	TH1F* h_all_adc_times=nullptr;
	TH1F* h_all_mrd_times=nullptr;
	TH1F* h_all_veto_times=nullptr;
	TH1F* h_all_veto_times_layer2=nullptr;
	TH1F* h_coincidence_event_times=nullptr;
	TH1F* h_tankhit_time_to_coincidence=nullptr;
	TApplication* rootTApp=nullptr;
	std::vector<TH1F*> vector_all_adc_times;	

	// helper function: to_string with a precision
	// particularly useful for printing doubles and floats in the Log function
	template <typename T>
	std::string to_string(T input, int precision=2){
		std::string s1 = std::to_string(input).substr(0, std::to_string(input).find(".") + precision + 1);
		if(s1.length()==0) return std::to_string(input);
		else return s1;
	}
	
	int nvetol1hits_=0, nvetol2hits_=0, nmrdl1hits_=0, nmrdl2hits_=0, notherhits_=0, ntotaltdchits_=0;
};


#endif

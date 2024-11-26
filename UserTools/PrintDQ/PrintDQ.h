#ifndef PrintDQ_H
#define PrintDQ_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Hit.h"


/**
 * \class PrintDQ
 *
 * A tool to print out statistics about processed runs for better Data Quality monitoring
*
* $Author: S.Doran $
* $Date: 2024/10/17 $
* Contact: doran@iastate.edu
*/
class PrintDQ: public Tool {

    public:

        PrintDQ();                                                             ///< Simple constructor
        bool Initialise(std::string configfile,DataModel &data);               ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
        bool Execute();                                                        ///< Execute function used to perform Tool purpose.
        bool Finalise();                                                       ///< Finalise function used to clean up resources.

        bool LoadStores();                                                     ///< Loads all relevant information from the store and some other parameters, away from the Execute function
        void ResetVariables();                                                 ///< Clearing maps and resetting variables
        bool GrabVariables();                                                  ///< Assign values to tricky variables (clusterTime, Grouped Triggers, MRD Tracks)
        void FindCounts();                                                     ///< Loop over extracted event information and count them up
        float CalculateStatError(float numerator, float denominator);          ///< Statistical error calculation for the rates

    private:

        std::map<double, std::vector<Hit>> *fClusterMap = nullptr;             ///< All clusters
        std::vector<std::vector<int>> fMrdTimeClusters;                        ///< All MRD clusters
        std::vector<BoostStore> *ftheMrdTracks;                                ///< the reconstructed MRD tracks
        std::map<std::string, bool> fDataStreams;                              ///< Tank, MRD, LAPPD data streams
        std::map<uint64_t, uint32_t> fGroupedTrigger;                          ///< collection of triggers for a single event
        vector<uint64_t> fGroupedTriggerTime;                                  ///< corresponding timestamp for each trigger
        vector<uint32_t> fGroupedTriggerWord;                                  ///< corresponding trigger number for each trigger
        vector<double> fClusterTime;                                           ///< PMT cluster times
        std::vector<int> fNumClusterTracks;                                    ///< MRD Tracks corresponding to MRD clusters (NumClusterTracks)
        
        int fRunNumber;                                                        ///< run number (tool assumes you're running over 1 run at a time)
        int fExtended;                                                         ///< extended window trigger (0 = none, 1 = charge-based, 2 = forced)
        ULong64_t fEventTimeTank;                                              ///< ADC waveform timestamp
        bool fNoVeto;                                                          ///< was the veto hit in coincidence with tank PMT activity? (NoVeto = 1 is no coincidence activity)
        bool fPMTMRDCoinc;                                                     ///< was there an MRD cluster in coincidence with tank PMT activity?
        double fBRFFirstPeakFit;                                               ///< first peak of the BRF waveform, Gaussian fit
        int fBeamok;                                                           ///< beam "ok" condition: (0.5e12 < pot < 8e12), (172 < horn val < 176), (downstream/upstream toroids within 5%)
        int fnumtracksinev;                                                    ///< number of MRD tracks found in the event
        int fHasLAPPD;                                                         ///< whether DataStreams contains LAPPDs

                                                                               /// *** description of print output from the tool ***
        // counts across all events                                            ///  total counts   (% / rate)    --> other variables expressed as out of "total events" are out of "total events with undelayed beam triggers"
        float_t totalclusters;                                                 ///< total clusters (clusters per event; not a %)
        float_t totalevents;                                                   ///< total events with an undelayed beam trigger (no rate/%)
        float_t totalclusters_in_prompt;                                       ///< total clusters in prompt window (% out of total clusters)
        float_t totalclusters_in_ext;                                          ///< total clusters in ext window (% out of total clusters)
        float_t totalclusters_in_spill;                                        ///< total clusters in spill (190:1750ns) (% out of total clusters)
        float_t totalext_rate_1;                                               ///< total extended windows (CC, charge based) (% out of total events)
        float_t totalext_rate_2;                                               ///< total extended windows (NC, forced readout) (% out of total events)
        float_t totalokay_beam;                                                ///< total events with "beam_ok" condition (% out of total events)
        float_t totaltmrd_coinc;                                               ///< total events with PMT+MRD coincidence (% out of total events)
        float_t totalveto_hit;                                                 ///< total events with PMR+Veto coicidence (% out of total events)
        float_t totalveto_tmrd_coinc;                                          ///< total events with PMT+MRD+Veto coincidence (% out of total events)
        float_t totalhas_track;                                                ///< total events with at least one instance of a single MRD track for a given MRD cluster (% out of total events)
        float_t totalhas_lappd;                                                ///< total events with LAPPD data present in the data stream (% out of total events)
        float_t totalhas_BRF;                                                  ///< total events with a fit to the BRF waveform (% out of total events)
        float_t totaltimezero;                                                 ///< total events with no timestamp for the ADC waveforms (% out of total events)
        


        /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
        int verbosity;
        int v_error=0;            // STOP THE SHOW
        int v_warning=1;          // this and below the show will go on
        int v_message=2;
        int v_debug=3;
        std::string logmessage;

};


#endif
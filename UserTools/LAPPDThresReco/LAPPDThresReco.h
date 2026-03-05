#ifndef LAPPDThresReco_H
#define LAPPDThresReco_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "LAPPDPulse.h"
#include "LAPPDHit.h"
#include "Geometry.h"
#include "TGraph.h"
#include "TF1.h"
#include "Position.h"

/**
 * \class LAPPDThresReco
 *
 * Use an input std::map<unsigned long,vector<Waveform<double>>> lappdData and a threshold to find pulses and construct hits.
 * Output a std::map<unsigned long,vector<LAPPDHit>> lappdHits.
 * Also generate txt files for hits and muon tracks. (need the muon track tools in the tool chain)
 *
 * $Author: Yue Feng $
 * $Date: 2024/01/29 $
 * Contact: yuef@iastate.edu
 */

class LAPPDThresReco : public Tool
{

public:
    LAPPDThresReco();                                         ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    vector<LAPPDPulse> FindPulses(vector<double> wave, int LAPPD_ID, int channel);
    vector<LAPPDHit> FindHit(vector<vector<LAPPDPulse>> pulses);
    LAPPDHit MakeHit(LAPPDPulse pulse0, LAPPDPulse pulse1);
    double GaussianFit(const vector<double> &xData, const vector<double> &yData);
    int LoadMRDTrackReco(int SubEventID);
    void CleanMRDRecoInfo();
    void CleanDataObjects();
    void FillLAPPDPulse();
    void FillLAPPDHit();
    void PrintHitsToTXT();
    void PrintMRDinfoToTXT();
    void WaveformMaximaFinding();

private:
    // This tool, control variables (only used in this tool, every thing that is not an data object)
    // Variables that you get from the config file
    int LAPPDThresRecoVerbosity;
    double threshold;
    int minPulseWidth;
    int printHitsTXT;
    bool useMaxTime;
    double signalSpeedOnStrip;
    double triggerBoardDelay;
    bool savePositionOnStrip;
    int useRange; //-1 averaged peak time, 0 low, 1 high
    bool loadPrintMRDinfo;
    double pulseFollowTimeStandard;
    int baselineStart;
    int baselineEnd;
    // Variables that you need in the tool
    int eventNumber;
    double pulseFollowTime;

    // LAPPD tool chain, control variables (Will be shared in multiple LAPPD tools to show the state of the tool chain in each loop)
    // Variables that you get from the config file
    string ThresRecoInputWaveLabel;
    string ThresRecoOutputPulseLabel;
    string ThresRecoOutputHitLabel;
    // Variables that you need in the tool
    Geometry *_geom;
    bool LAPPDana;
    bool LoadLAPPDMapInfo;

    // LAPPD tool chain, data variables. (Will be used in multiple LAPPD tools)
    // everything you get or set to Store, which means it may be used in other tools or it's from other tools
    std::vector<BoostStore> *theMrdTracks; // the reconstructed tracks
    int numtracksinev;
    std::map<int, int> channelnoToSwitchbit;
    std::map<unsigned long, vector<double>> waveformMax;
    std::map<unsigned long,vector<double>> waveformRMS;
    std::map<unsigned long,vector<double>> waveformMaxLast;
    std::map<unsigned long,vector<double>> waveformMaxNearing;
    std::map<unsigned long,vector<int>> waveformMaxTimeBin;
    vector<int> LAPPD_IDs;

    // This tool, data variables (only used in this tool, every thing that is an data object)
    std::map<unsigned long, vector<Waveform<double>>> lappdData;     // waveform data
    std::map<unsigned long, vector<vector<LAPPDPulse>>> lappdPulses; // save threshold based method found lappd pulses
    std::map<unsigned long, vector<LAPPDHit>> lappdHits;
    std::vector<std::vector<int>> MrdTimeClusters;
    int LAPPD_ID;
    std::vector<double> fMRDTrackAngle;
    std::vector<double> fMRDTrackAngleError;
    std::vector<double> fMRDPenetrationDepth;
    std::vector<double> fMRDTrackLength;
    std::vector<double> fMRDEntryPointRadius;
    std::vector<double> fMRDEnergyLoss;
    std::vector<double> fMRDEnergyLossError;
    std::vector<double> fMRDTrackStartX;
    std::vector<double> fMRDTrackStartY;
    std::vector<double> fMRDTrackStartZ;
    std::vector<double> fMRDTrackStopX;
    std::vector<double> fMRDTrackStopY;
    std::vector<double> fMRDTrackStopZ;
    std::vector<double> fHtrackFitChi2;
    std::vector<double> fHtrackFitCov;
    std::vector<double> fVtrackFitChi2;
    std::vector<double> fVtrackFitCov;
    std::vector<bool> fMRDSide;
    std::vector<bool> fMRDStop;
    std::vector<bool> fMRDThrough;
    std::vector<double> fHtrackOrigin;
    std::vector<double> fHtrackOriginError;
    std::vector<double> fHtrackGradient;
    std::vector<double> fHtrackGradientError;
    std::vector<double> fVtrackOrigin;
    std::vector<double> fVtrackOriginError;
    std::vector<double> fVtrackGradient;
    std::vector<double> fVtrackGradientError;
    std::vector<int> fparticlePID;
    
    // data variables don't need to be cleared in each loop
    ofstream myfile;
    ofstream mrdfile;

};

#endif

#ifndef LAPPDPlots_H
#define LAPPDPlots_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TString.h"
#include "TTree.h"
#include "LAPPDHit.h"
#include "LAPPDPulse.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TColor.h"
#include "TLegend.h"
#include "TMath.h"
#include "Detector.h"
#include "Geometry.h"

/**
 * \class LAPPDPlots
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */
class LAPPDPlots : public Tool
{

public:
    LAPPDPlots();                                             ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    std::map<unsigned long, vector<Waveform<double>>> GetDataForBoard(int boardID);
    int CheckInBeamgateWindow();
    void CleanObjects();
    void PrintWaveformToTxt();

private:
    //**************************** This tool, control variables ***************************************************
    // (only used in this tool, every thing that is not an data object)
    // Variables that you get from the config file
    int CanvasXSubPlotNumber;
    int CanvasYSubPlotNumber; // c->Divide(x, y);
    int CanvasWidth;
    int CanvasHeight;
    int maxDrawEventNumber;
    string LAPPDPlotInputWaveLabel;
    int LAPPDPlotsVerbosity;
    // draw event waveform
    int Side0EventWaveformDrawPosition;
    int Side1EventWaveformDrawPosition;
    bool drawTriggerChannel;
    double drawHighThreshold;
    double drawLowThreshold;
    double titleSize;
    double canvasTitleOffset;
    double canvasMargin;
    int colorPalette;
    bool DrawEventWaveform;
    bool OnlyDrawInBeamWindow;
    int BeamWindowStart;
    int BeamWindowEnd;
    bool DrawBinHist;
    int Side0BinDrawPosition;
    int Side1BinDrawPosition;
    double BinHistMin;
    double BinHistMax;
    int BinHistNumber;
    bool LoadLAPPDMap;


    // Variables that you need in the tool
    int CanvasTotalSubPlotNumber;
    int inBeamWindow;
    double BGTiming;
    //**************************** LAPPD tool chain, control variables ***************************************************
    //(Will be shared in multiple LAPPD tools to show the state of the tool chain in each loop)
    // Variables that you get from the config file

    // Variables that you need in the tool
    bool LAPPDana;

    //**************************** LAPPD tool chain, data variables ***************************************************
    // (Will be used in multiple LAPPD tools)
    // everything you get or set to Store, which means it may be used in other tools or it's from other tools
    Geometry *_geom;
    std::map<unsigned long, vector<Waveform<double>>> lappddata;
    std::vector<int> ReadBoards;
    int LAPPD_ID;
    vector<int> ACDCReadedLAPPDID;
    vector<int> LAPPD_IDs;

    //**************************** This tool, data variables ***************************************************
    // (only used in this tool, every thing that is an data object)
    // data variables don't need to be cleared in each loop
    TCanvas *c;
    TFile *f;
    int eventNumber;

    bool printEventWaveform;
    int printLAPPDNumber;
    int printEventNumber;

    //event number, lappd_id order, map of strip number and waveform
    vector<vector<map<int,vector<double>>>> WaveformToPringSide0;
    vector<vector<map<int,vector<double>>>> WaveformToPringSide1;
};

#endif

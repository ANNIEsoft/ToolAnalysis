#ifndef LAPPDWaveformDisplay_H
#define LAPPDWaveformDisplay_H

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include <string>
#include <iostream>
#include "Tool.h"

#include "TCanvas.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include <TStyle.h>
#include "TPaletteAxis.h"
#include "TPaveText.h"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class LAPPDWaveformDisplay: public Tool {

  public:

    LAPPDWaveformDisplay();
    bool Initialise(std::string configfile,DataModel &data);
    bool Execute();
    bool Finalise();

  private:
    int GetColorForStrip(int stripNo);
    void ThrowRuntimeError(const std::string& errorMessage, int line);
    void SetCanvasStyle();
    void PrintRunInfo();
  
  private:
    //From ANNIEEvent store
    Geometry* mGeo;
    
    //From config file
    int mFirstEventNumber;
    int mLastEventNumber;
    
    std::string mInputWaveformLabel;
    int mChannelOffset;
    int mTriggerChannel1;
    int mTriggerChannel2;
    
    int mVerbosity;
    bool mIsT0SignalRequired;
    bool mIsTriggerChannelDisplayed;
    std::string mOutputFileName;
    
    bool mIsRunInfoPrinted;
    std::string mDate;
    std::string mLAPPDModel;
    std::string mPath;
    std::string mLaserStatus;
    std::string mTriggerMode;
    std::string mNDFilter;
    std::string mRunInfo;
    
    // Class-specific variables
    TCanvas *mCanvas;
    int mEventID;
    bool mIsEventRangeDefined;
    std::string mBoostStoreErrMsg;
        
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif

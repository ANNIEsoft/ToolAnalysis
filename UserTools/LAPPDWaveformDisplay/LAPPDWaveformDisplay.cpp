#include "LAPPDWaveformDisplay.h"

LAPPDWaveformDisplay::LAPPDWaveformDisplay():
Tool(),
mGeo(nullptr),
mFirstEventNumber(0),
mLastEventNumber(100),
mChannelOffset(0),
mTriggerChannel1(1005),
mTriggerChannel2(1035),
mVerbosity(0),
mIsT0SignalRequired(false),
mIsTriggerChannelDisplayed(false),
mIsRunInfoPrinted(false),
mCanvas(nullptr),
mEventID(0),
mIsEventRangeDefined(false)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

bool LAPPDWaveformDisplay::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile); 
  
  m_data= &data; 
  
  { //Get from ANNIEEvent BoostStore
    mBoostStoreErrMsg = "Error: Data retrieval from 'ANNIEEvent' store failed."; 
    if ( !m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", mGeo) )
      ThrowRuntimeError(mBoostStoreErrMsg, __LINE__);
  }
  
  { //Get variables from config file  
    if (m_variables.Get("FirstEventNumber", mFirstEventNumber) && 
    m_variables.Get("LastEventNumber", mLastEventNumber)) {
      mIsEventRangeDefined = true;
    }
    
    std::string storeErrMsg = "Error: Data retrieval from 'Store' failed.";
    
    if ( !m_variables.Get("InputWaveformLabel", mInputWaveformLabel) )
      ThrowRuntimeError(storeErrMsg, __LINE__); 
    
    if ( !m_variables.Get("ChannelOffset", mChannelOffset) )
      ThrowRuntimeError(storeErrMsg, __LINE__);
      
    if ( !m_variables.Get("TrigerChannel1", mTriggerChannel1) )
      ThrowRuntimeError(storeErrMsg, __LINE__); 
      
    if ( !m_variables.Get("TrigerChannel2", mTriggerChannel2) )
      ThrowRuntimeError(storeErrMsg, __LINE__);
      
    if (!m_variables.Get("WaveformDisplayVerbosity", mVerbosity))
      mVerbosity = 0; //default 
      
    if (!m_variables.Get("RequireT0Signal", mIsT0SignalRequired))
      mIsT0SignalRequired = false; 
      
    if (!m_variables.Get("DisplayTriggerChannelInPlot", mIsTriggerChannelDisplayed))
      mIsTriggerChannelDisplayed = true;
      
    if (!m_variables.Get("OutputFileName", mOutputFileName))
      mOutputFileName = "LAPPDWaveformDisplay.pdf"; 
      
    if (!m_variables.Get("PrintRunInfo", mIsRunInfoPrinted))
      mIsRunInfoPrinted = 1;    
      
    if (!m_variables.Get("Date", mDate))
      mDate = "N/A"; 
      
    if (!m_variables.Get("LAPPDModel", mLAPPDModel))
      mLAPPDModel = "N/A"; 
    
    if (!m_variables.Get("Path", mPath))
      mPath = "N/A";   
      
    if (!m_variables.Get("LaserStatus", mLaserStatus))
      mLaserStatus = "N/A"; 
      
    if (!m_variables.Get("TriggerMode", mTriggerMode))
      mTriggerMode = "N/A";  
      
    if (!m_variables.Get("NDFilter", mNDFilter))
      mNDFilter = "N/A"; 
      
    if (!m_variables.Get("RunInfo", mRunInfo))
      mRunInfo = "N/A"; 
         
  } 
     
  { // Create a canvas for event display, apply canvas style,
    // and open the output file in append mode to start saving plots     
    mCanvas = new TCanvas("Event Display Canvas", "Event Display", 1200, 800);
    SetCanvasStyle();
    mCanvas->Print( (mOutputFileName + "[").c_str() );
  }
  
  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

bool LAPPDWaveformDisplay::Execute(){
  // This function is organized into 8 steps for readability and clarity.
  
  // 1. Skip events that are outside the defined event range 
  if (mIsEventRangeDefined){
    if (mEventID < mFirstEventNumber || mEventID > mLastEventNumber) {
      mEventID++;
      return true;
    } 
  }
  
  // 2. Skip events if T0 signal is required but not found in the time window
  bool T0SignalInWindow{false};
  if ( m_data->Stores["ANNIEEvent"]->Get("T0signalInWindow", T0SignalInWindow) )
    T0SignalInWindow = true; 
  if (mIsT0SignalRequired && !T0SignalInWindow){
    mEventID++;
    return true;
  }
  
  // 3. Get lappd waveforms fromv ANNIEEvent BoostStore
  std::map< unsigned long, vector<Waveform<double>>> lappddata;
  if ( !m_data->Stores["ANNIEEvent"]->Get( mInputWaveformLabel, lappddata) )
    ThrowRuntimeError(mBoostStoreErrMsg, __LINE__);

  // 4. Create 2D histogram and multi-graph for each strip side 
  int numSamples{0}; 
  {
    Waveform<double> fwav = (lappddata.begin()->second).at(0);
    numSamples = fwav.GetSamples()->size();
    if (numSamples != 256)
      ThrowRuntimeError("Unexpected waveform size! ", __LINE__);
  }
  TString side0HistoName = "Side 0 Event ";
  side0HistoName += mEventID;
  TH2D* hSideO = new TH2D(side0HistoName, side0HistoName, numSamples, 
                          -0.5, 255.5, 30, -0.5, 29.5);
  TString side1HistoName = "Side 1 Event ";
  side1HistoName += mEventID;
  TH2D* hSide1 = new TH2D(side1HistoName, side1HistoName, numSamples, 
                      -0.5, 255.5, 30, -0.5, 29.5);
  TMultiGraph *side0MultiGraph = new TMultiGraph();                    
  TMultiGraph *side1MultiGraph = new TMultiGraph();
     
  // 5. Fill histograms and graphs  
  std::map<unsigned long, vector<Waveform<double>>> :: iterator itr;
  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){ 
  
    unsigned long channelNo = itr->first;
    channelNo = channelNo + mChannelOffset;
    
    if (channelNo == mTriggerChannel1 || channelNo == mTriggerChannel2) 
      if (!mIsTriggerChannelDisplayed) continue;
    
    Channel* mychannel = mGeo->GetChannel(channelNo);

    if (!mychannel)
      ThrowRuntimeError("mGeo->GetChannel(channelNo) returns null!!", __LINE__);

    int stripNo   = mychannel->GetStripNum();
    int stripSide = mychannel->GetStripSide();

    if (mVerbosity > 1){
      cout<<"ChannelNo: "<<channelNo<<"  StripNo: "<<stripNo<<
        "  StripSide: "<<stripSide<<endl;
    }
    
    vector< Waveform<double> > Vwavs = itr->second;
      
    if (stripSide == 0){
      TGraph *graphSide0 = new TGraph();

      for (int i = 0; i < Vwavs.size(); i++){ 
        Waveform<double> bwav = Vwavs.at(i);

        for (int j = 0; j < bwav.GetSamples()->size(); j++){ 
          hSideO->Fill(j, stripNo, -bwav.GetSamples()->at(j));
          graphSide0->SetPoint(j, j, -bwav.GetSamples()->at(j));
        }
      }

      graphSide0->SetLineColor(GetColorForStrip(stripNo));
      side0MultiGraph->Add(graphSide0);  

    }else if( stripSide == 1){

      TGraph *graphSide1 = new TGraph();
      
      for (int i = 0; i < Vwavs.size(); i++){
        Waveform<double> bwav = Vwavs.at(i);

        for (int j = 0; j < bwav.GetSamples()->size(); j++){
          hSide1->Fill(j, stripNo, -bwav.GetSamples()->at(j));
          graphSide1->SetPoint(j, j, -bwav.GetSamples()->at(j));
        }

      }

      graphSide1->SetLineColor(GetColorForStrip(stripNo));
      side1MultiGraph->Add(graphSide1);  

    }
      
  } 
     
  // 6. Print run info only on the first page if enabled          
  if (mIsRunInfoPrinted && (mEventID == 0 || mEventID == mFirstEventNumber))
    PrintRunInfo();

  // 7. Clear the canvas and divide it into 2x2 grid for displaying 4 plots
  mCanvas->Clear();
  mCanvas->Divide(2, 2); 
  mCanvas->cd(1);
  hSideO->Draw("COLZ");
  //hSideO->SetTitle("");
  hSideO->GetXaxis()->SetTitle("Sample number or Time (x ~0.1 ns)");
  hSideO->GetXaxis()->CenterTitle();
  hSideO->GetYaxis()->SetTitle("Strip number");
  hSideO->GetYaxis()->CenterTitle();
  hSideO->GetZaxis()->SetTitle("Amplitute (mV)");
  hSideO->GetZaxis()->CenterTitle();
  mCanvas->cd(2);
  hSide1->Draw("COLZ");
  //hSide1->SetTitle("");
  hSide1->GetXaxis()->SetTitle("Sample number or Time (x ~0.1 ns)");
  hSide1->GetXaxis()->CenterTitle();
  hSide1->GetYaxis()->SetTitle("Strip number");
  hSide1->GetYaxis()->CenterTitle();
  hSide1->GetZaxis()->SetTitle("Amplitute (mV)");
  hSide1->GetZaxis()->CenterTitle();   
  mCanvas->cd(3);
  side0MultiGraph->Draw("AL"); 
  //side0MultiGraph->SetTitle(mOutputWavLabel.c_str());
  //side0MultiGraph->GetYaxis()->SetRangeUser(-200, 200);
  side0MultiGraph->GetXaxis()->SetTitle("Sample number or Time (x ~0.1 ns)");
  side0MultiGraph->GetXaxis()->CenterTitle();
  side0MultiGraph->GetYaxis()->SetTitle("Amplitude (mV)");
  side0MultiGraph->GetYaxis()->CenterTitle();
  side0MultiGraph->GetXaxis()->SetNdivisions(10); 
  mCanvas->cd(4);
  side1MultiGraph->Draw("AL"); 
  //side1MultiGraph->SetTitle(mOutputWavLabel.c_str());
  //side1MultiGraph->GetYaxis()->SetRangeUser(-400, 400);
  side1MultiGraph->GetXaxis()->SetTitle("Sample number or Time (x ~0.1 ns)");
  side1MultiGraph->GetXaxis()->CenterTitle();
  side1MultiGraph->GetYaxis()->SetTitle("Amplitude (mV)");
  side1MultiGraph->GetYaxis()->CenterTitle();
  side1MultiGraph->GetXaxis()->SetNdivisions(10); 
  mCanvas->cd();
  mCanvas->Print( mOutputFileName.c_str() );
  
  //8. Cleanup memory
  delete hSideO;
  delete hSide1;
  // The TMultiGraph owns the graphs added to it, so deleting the multi-graph
  // will automatically delete all the graphs contained within it.
  delete side0MultiGraph;
  delete side1MultiGraph;
  
  mEventID++;

  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

bool LAPPDWaveformDisplay::Finalise(){
  mCanvas->Print( (mOutputFileName + "]").c_str() );
  
  delete mCanvas;
  
  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int LAPPDWaveformDisplay::GetColorForStrip(int stripNo) {

  static std::map<unsigned long, int> stripColors;

  if ( stripColors.find(stripNo) != stripColors.end() ) {
      return stripColors[stripNo];
  }

  static const int numColors = 30; //strip number

 int colorPalette[numColors] = {
    TColor::GetColor(255, 0, 0),     // Red
    TColor::GetColor(0, 0, 255),     // Blue
    TColor::GetColor(0, 255, 0),     // Green
    TColor::GetColor(255, 165, 0),   // Orange
    TColor::GetColor(128, 0, 128),   // Purple
    TColor::GetColor(255, 20, 147),  // Deep Pink
    TColor::GetColor(0, 255, 255),   // Cyan (Light Blue)
    TColor::GetColor(75, 0, 130),    // Indigo
    TColor::GetColor(210, 105, 30),  // Chocolate Brown
    TColor::GetColor(255, 255, 0),   // Yellow
    TColor::GetColor(0, 128, 128),   // Dark Turquoise
    TColor::GetColor(128, 128, 0),   // Olive Green
    TColor::GetColor(139, 0, 0),     // Dark Red
    TColor::GetColor(0, 139, 139),   // Dark Cyan
    TColor::GetColor(144, 238, 144), // Light Green
    TColor::GetColor(70, 130, 180),  // Steel Blue
    TColor::GetColor(255, 140, 0),   // Dark Orange
    TColor::GetColor(186, 85, 211),  // Orchid
    TColor::GetColor(147, 112, 219), // Medium Purple
    TColor::GetColor(60, 179, 113),  // Medium Sea Green
    TColor::GetColor(205, 92, 92),   // Dark Salmon
    TColor::GetColor(255, 105, 180), // Hot Pink
    TColor::GetColor(72, 61, 139),   // Dark Slate Blue
    TColor::GetColor(0, 191, 255),   // Deep Sky Blue
    TColor::GetColor(255, 228, 196), // Wheat
    TColor::GetColor(85, 107, 47),   // Dark Olive Green
    TColor::GetColor(255, 99, 71),   // Tomato Red
    TColor::GetColor(123, 104, 238), // Medium Slate Blue
    TColor::GetColor(34, 139, 34),   // Forest Green
    TColor::GetColor(250, 128, 114)  // Salmon
  };

  int colorIndex    = stripNo % numColors;
  int assignedColor = colorPalette[colorIndex];

  stripColors[stripNo] = assignedColor;
  
  if (mVerbosity > 2)
    cout<<"StripNo: "<<stripNo<<"  ColorIndex: "<<colorIndex<<endl;
  
  return assignedColor;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void LAPPDWaveformDisplay::SetCanvasStyle(){

  gStyle->SetCanvasColor(kWhite);
  gStyle->SetPadColor(kWhite);
  gStyle->SetFrameFillColor(kWhite);

  gStyle->SetFrameLineWidth(2);
  gStyle->SetFrameLineColor(kBlack);

  // gStyle->SetGridColor(kGray+1);
  // gStyle->SetGridStyle(3);
  // gStyle->SetGridWidth(1);
  // gStyle->SetPadGridX(true);
  // gStyle->SetPadGridY(true);

  // Axis style
  gStyle->SetLabelSize(0.045, "XYZ");  
  gStyle->SetLabelFont(42, "XYZ");     
  gStyle->SetLabelColor(kBlack, "XYZ");

  gStyle->SetTitleSize(0.05, "XYZ");   
  gStyle->SetTitleFont(42, "XYZ");
  gStyle->SetTitleColor(kBlack, "XYZ");

  gStyle->SetTitleOffset(1.2, "X");    
  gStyle->SetTitleOffset(1.2, "Y");    

  // Margins
  gStyle->SetPadLeftMargin(0.16);
  gStyle->SetPadRightMargin(0.15);
  gStyle->SetPadBottomMargin(0.13);
  gStyle->SetPadTopMargin(0.1);

  gStyle->SetPalette(112); // kViridis 
  // gStyle->SetPalette(55); // Alternative: kBird

  // Thicken the frame for better readability
  gStyle->SetHistLineWidth(2);

  // Align axis titles for better clarity
  gStyle->SetTitleAlign(23);  // Center align
  
  gStyle->SetOptStat(0);
  gStyle->SetPalette(kBird); 

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void LAPPDWaveformDisplay::PrintRunInfo()
{
  //Header
  TPaveText *headerBox = new TPaveText(0.05, 0.92, 0.95, 1.0, "NDC");  
  headerBox->SetTextAlign(22);  
  headerBox->SetTextSize(0.04); 
  headerBox->SetFillColor(kGray);  
  headerBox->SetTextColor(kWhite);  
  headerBox->AddText("LAPPD Event Waveform Display"); 
  headerBox->Draw();

  TPaveText *infoBox = new TPaveText(0.05, 0.2, 0.95, 0.9, "NDC"); 
  infoBox->SetTextAlign(12); 
  infoBox->SetTextSize(0.03);  

  std::string date = "Date : " + mDate;
  infoBox->AddText(date.c_str()); 

  std::string lappdModel = "LAPPD model : " + mLAPPDModel;
  infoBox->AddText(lappdModel.c_str()); 

  std::string path = "Path : " + mPath;
  infoBox->AddText(path.c_str()); 

  std::string laserStatus = "Laser status: " + mLaserStatus;
  infoBox->AddText(laserStatus.c_str()); 

  std::string triggerMode = "Trigger mode: " + mTriggerMode;
  infoBox->AddText(triggerMode.c_str());

  std::string ndFilter = "ND filter: " + mNDFilter;
  infoBox->AddText(ndFilter.c_str());

  std::string runInfo = "Run info: " + mRunInfo; 
  infoBox->AddText(runInfo.c_str()); 

  //infoBox->AddText("");  //Empty line
  infoBox->Draw();

  mCanvas->Print( mOutputFileName.c_str() );
   
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void LAPPDWaveformDisplay::ThrowRuntimeError
(
  const std::string& errorMessage, 
  int line
)
{
  throw std::runtime_error(errorMessage + " at " + __FILE__ + 
                             ":" + std::to_string(line));
}

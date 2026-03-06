#include "LAPPDPlots.h"

LAPPDPlots::LAPPDPlots() : Tool() {}

bool LAPPDPlots::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);

  m_variables.Get("LAPPDPlotInputWaveLabel", LAPPDPlotInputWaveLabel);
  LAPPDPlotsVerbosity = 0;
  m_variables.Get("LAPPDPlotsVerbosity", LAPPDPlotsVerbosity);
  CanvasXSubPlotNumber = 2;
  m_variables.Get("CanvasXSubPlotNumber", CanvasXSubPlotNumber);
  CanvasYSubPlotNumber = 3;
  m_variables.Get("CanvasYSubPlotNumber", CanvasYSubPlotNumber);
  CanvasTotalSubPlotNumber = CanvasXSubPlotNumber * CanvasYSubPlotNumber;
  Side0EventWaveformDrawPosition = 1;
  m_variables.Get("Side0EventWaveformDrawPosition", Side0EventWaveformDrawPosition);
  Side1EventWaveformDrawPosition = 2;
  m_variables.Get("Side1EventWaveformDrawPosition", Side1EventWaveformDrawPosition);
  drawTriggerChannel = true;
  m_variables.Get("drawTriggerChannel", drawTriggerChannel);
  drawHighThreshold = 50;
  m_variables.Get("drawHighThreshold", drawHighThreshold);
  drawLowThreshold = -20;
  m_variables.Get("drawLowThreshold", drawLowThreshold);
  titleSize = 0.05;
  m_variables.Get("titleSize", titleSize);
  canvasTitleOffset = 1.05;
  m_variables.Get("canvasTitleOffset", canvasTitleOffset);
  canvasMargin = 0.15;
  m_variables.Get("canvasMargin", canvasMargin);
  CanvasWidth = 800;
  m_variables.Get("CanvasWidth", CanvasWidth);
  CanvasHeight = 1200;
  m_variables.Get("CanvasHeight", CanvasHeight);
  maxDrawEventNumber = 20;
  m_variables.Get("maxDrawEventNumber", maxDrawEventNumber);
  colorPalette = 112;
  m_variables.Get("colorPalette", colorPalette);
  DrawEventWaveform = true;
  m_variables.Get("DrawEventWaveform", DrawEventWaveform);
  OnlyDrawInBeamWindow = false;
  m_variables.Get("OnlyDrawInBeamWindow", OnlyDrawInBeamWindow);
  BeamWindowStart = 8000;
  m_variables.Get("BeamWindowStart", BeamWindowStart);
  BeamWindowEnd = 10000;
  m_variables.Get("BeamWindowEnd", BeamWindowEnd);
  Side0BinDrawPosition = 3;
  m_variables.Get("Side0BinDrawPosition", Side0BinDrawPosition);
  Side1BinDrawPosition = 4;
  m_variables.Get("Side1BinDrawPosition", Side1BinDrawPosition);
  DrawBinHist = true;
  m_variables.Get("DrawBinHist", DrawBinHist);
  BinHistMin = -20;
  m_variables.Get("BinHistMin", BinHistMin);
  BinHistMax = 50;
  m_variables.Get("BinHistMax", BinHistMax);
  BinHistNumber = 100;
  m_variables.Get("BinHistNumber", BinHistNumber);

  printEventWaveform = 0;
  m_variables.Get("printEventWaveform", printEventWaveform);
  printLAPPDNumber = 2;
  m_variables.Get("printLAPPDNumber", printLAPPDNumber);
  printEventNumber = 10;
  m_variables.Get("printEventNumber", printEventNumber);

  LoadLAPPDMap = false;
  m_variables.Get("LoadLAPPDMap", LoadLAPPDMap);

  eventNumber = 0;

  f = new TFile("LAPPDPlots.root", "RECREATE");
  c = new TCanvas("c", "c", CanvasWidth, CanvasHeight);

  return true;
}

bool LAPPDPlots::Execute()
{
  m_data->CStore.Get("LAPPDana", LAPPDana);
  if (!LAPPDana)
    return true;
  if (eventNumber > maxDrawEventNumber)
    return true;

  inBeamWindow = CheckInBeamgateWindow();
  if (OnlyDrawInBeamWindow && (inBeamWindow == 0 || inBeamWindow == -1))
    return true;

  m_data->Stores["ANNIEEvent"]->Get("ACDCReadedLAPPDID", ACDCReadedLAPPDID);
  m_data->Stores["ANNIEEvent"]->Get("LAPPD_IDs", LAPPD_IDs);

  if (LoadLAPPDMap)
    CanvasXSubPlotNumber = *max_element(ACDCReadedLAPPDID.begin(), ACDCReadedLAPPDID.end()) - *min_element(ACDCReadedLAPPDID.begin(), ACDCReadedLAPPDID.end()) + 1;
  CanvasXSubPlotNumber = CanvasXSubPlotNumber * 2;
  CanvasTotalSubPlotNumber = CanvasXSubPlotNumber * CanvasYSubPlotNumber;

  c->Clear();
  c->Divide(CanvasXSubPlotNumber, CanvasYSubPlotNumber);

  if (LAPPDPlotsVerbosity > 0)
    cout << "Divide canvas into " << CanvasXSubPlotNumber << "x" << CanvasYSubPlotNumber << " subplots" << endl;
  c->SetTitle("Event " + TString::Itoa(eventNumber, 10));
  c->SetName("Event" + TString::Itoa(eventNumber, 10));
  // c->cd(1);
  gStyle->SetPalette(colorPalette);

  bool gotdata = m_data->Stores["ANNIEEvent"]->Get(LAPPDPlotInputWaveLabel, lappddata);
  m_data->Stores["ANNIEEvent"]->Get("ACDCboards", ReadBoards);
  m_data->CStore.Get("LAPPD_ID", LAPPD_ID);
  if (LAPPDPlotsVerbosity > 0)
  {
    cout << "LAPPDPlots, got ACDCboards size = " << ReadBoards.size() << " with id = ";
    for (auto i : ReadBoards)
      cout << i << " ";
    cout << endl;
    if (LoadLAPPDMap)
    {
      cout << "ACDCReadedLAPPDID size = " << ACDCReadedLAPPDID.size() << ": ";
      for (auto i : ACDCReadedLAPPDID)
        cout << i << " ";
      cout << endl;
    }
  }

  vector<int> drawPositions = ReadBoards;
  if (LoadLAPPDMap)
  {
    vector<int> drawPositions = ACDCReadedLAPPDID;
    int minID = *min_element(ACDCReadedLAPPDID.begin(), ACDCReadedLAPPDID.end());
    for (int i = 0; i < drawPositions.size(); i++)
    {
      drawPositions.at(i) = (drawPositions.at(i) - minID) * 2 + 1;
      if (i % 2 == 1)
        drawPositions.at(i) = drawPositions.at(i) + 1;
    }
  }
  if (LAPPDPlotsVerbosity > 0)
  {
    cout << "drawPositions size = " << drawPositions.size() << ": ";
    for (auto i : drawPositions)
      cout << i << " ";
    cout << endl;
  }

  vector<int> drawBoardID = ReadBoards;

  if (LoadLAPPDMap)
  {
    drawBoardID = ACDCReadedLAPPDID;
    for (int i = 0; i < drawBoardID.size(); i++)
    {
      drawBoardID.at(i) = drawBoardID.at(i) * 2;
      if (i % 2 == 1)
        drawBoardID.at(i) = (drawBoardID.at(i) + 1);
    }
  }
  if (LAPPDPlotsVerbosity > 0)
  {
    cout << "drawBoardID size = " << drawBoardID.size() << ": ";
    for (auto i : drawBoardID)
      cout << i << " ";
    cout << endl;
    cout << "LAPPDPlots execute with data " << LAPPDPlotInputWaveLabel << ", got data " << gotdata << ", data size " << lappddata.size() << ", Board IDs size " << ReadBoards.size() << ", (single) ID " << LAPPD_ID << endl;
  }

  if (DrawEventWaveform)
  {
    vector<int> DrawPosition = {Side0EventWaveformDrawPosition, Side1EventWaveformDrawPosition};
    if (LoadLAPPDMap)
    {
      if (ACDCReadedLAPPDID.size() > DrawPosition.size())
      {
        DrawPosition = drawPositions;
      }
    }
    for (int i = 0; i < drawBoardID.size(); i++)
    {
      const int drawPosition = DrawPosition[i];
      TPad *p = (TPad *)c->cd(drawPosition);
      // c->cd(drawPosition);
      p->cd();
      p->SetRightMargin(canvasMargin);
      p->SetLeftMargin(canvasMargin);
      p->SetTopMargin(canvasMargin);
      p->SetBottomMargin(canvasMargin);
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << drawBoardID[i] << " at canvas position " << drawPosition << " start" << endl;
      std::map<unsigned long, vector<Waveform<double>>> boarddata = GetDataForBoard(drawBoardID[i]);
      TString HistoName = "Event" + TString::Itoa(eventNumber, 10) + "_B" + drawBoardID[i] + "_ID";
      if (LoadLAPPDMap)
        HistoName += ACDCReadedLAPPDID[i];
      // convert BGTiming to string and add to HistoName
      // if (OnlyDrawInBeamWindow)
      HistoName += "_BG" + TString::Itoa(BGTiming, 10);

      // DrawEventWaveform(c, DrawPosition[i], HistoName, boarddata);
      // TH2D h = DrawEventWaveform(HistoName, boarddata);

      int nstrips = 30;
      if (!drawTriggerChannel)
        nstrips = 28;
      TH2D *h = new TH2D(HistoName, HistoName, 256, 0, 256, nstrips, 0 - 0.5, nstrips - 0.5);
      if (LAPPDPlotsVerbosity > 3)
        cout << "Start Drawing event waveform with data size = " << boarddata.size() << endl;
      std::map<unsigned long, vector<Waveform<double>>>::iterator it;
      for (it = boarddata.begin(); it != boarddata.end(); it++)
      {
        unsigned long originalChannelNo = it->first;
        unsigned long channelNo = ((it->first % 1000)) % 60 + 1000;
        Waveform<double> w = it->second[0];
        Channel *ch = _geom->GetChannel(channelNo);
        int stripNo = ch->GetStripNum();
        if (LAPPDPlotsVerbosity > 3)
          cout << "Drawing channel " << originalChannelNo << " as channelNo " << channelNo << " strip " << stripNo << ", with sample size " << w.GetSamples()->size() << ", data at bin 0 = " << -w.GetSamples()->at(0) << endl;
        if (!drawTriggerChannel)
          if ((channelNo % 1000) % 30 == 5)
            continue;

        for (int i = 0; i < w.GetSamples()->size(); i++)
        {
          h->SetBinContent(i, stripNo + 1, -w.GetSamples()->at(i));
          if (LAPPDPlotsVerbosity > 0)
          {
            if (i < 5)
              cout << -w.GetSamples()->at(i) << ", ";
          }
        }
      }
      if (LAPPDPlotsVerbosity > 3)
        cout << " Finish Drawing event waveform" << endl;

      h->SetMaximum(drawHighThreshold);
      h->SetMinimum(drawLowThreshold);
      h->SetStats(0);
      h->GetXaxis()->SetTitle("Time (0.1ns)");
      h->GetXaxis()->SetTitleSize(titleSize);
      h->GetXaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetYaxis()->SetTitle("Strip Number");
      h->GetYaxis()->SetTitleSize(titleSize);
      h->GetYaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetZaxis()->SetTitle("Amplitude (mV)");

      h->Draw("colz");
      f->cd();
      h->Write();
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << drawBoardID[i] << " finished for Draw Event waveform" << endl;
    }
  }

  if (DrawBinHist)
  {

    vector<int> DrawPositionBinHist = {Side0BinDrawPosition, Side1BinDrawPosition};
    if (drawBoardID.size() > DrawPositionBinHist.size())
    {
      DrawPositionBinHist.clear();
      for (int i = 0; i < drawBoardID.size(); i++)
      {
        DrawPositionBinHist.push_back(drawBoardID.at(i) + CanvasXSubPlotNumber + 1);
      }
    }
    for (int i = 0; i < drawBoardID.size(); i++)
    {
      const int drawPosition = DrawPositionBinHist[i];
      TPad *p = (TPad *)c->cd(drawPosition);
      // c->cd(drawPosition);
      p->cd();
      p->SetRightMargin(canvasMargin);
      p->SetLeftMargin(canvasMargin);
      p->SetTopMargin(canvasMargin);
      p->SetBottomMargin(canvasMargin);
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << drawBoardID[i] << " at position " << DrawPositionBinHist[i] << " start" << endl;
      std::map<unsigned long, vector<Waveform<double>>> boarddata = GetDataForBoard(drawBoardID[i]);
      TString HistoName = "Event" + TString::Itoa(eventNumber, 10) + "_Bin_B" + drawBoardID[i] + "_ID";
      if (LoadLAPPDMap)
        HistoName += ACDCReadedLAPPDID[i];
      // convert BGTiming to string and add to HistoName
      // if (OnlyDrawInBeamWindow)
      HistoName += "_BG" + TString::Itoa(BGTiming, 10);

      int nstrips = 30;
      if (!drawTriggerChannel)
        nstrips = 28;
      const double BinHistMinConst = BinHistMin;
      //cout<<"min is "<<BinHistMin<<", "<<BinHistMinConst<<endl;
      const int BinHistMaxConst = BinHistMax;
      //cout<<"max is "<<BinHistMax<<", "<<BinHistMaxConst<<endl;
      const int BinHistNumberConst = BinHistNumber;
      TH2D *h = new TH2D(HistoName, HistoName, BinHistNumberConst, BinHistMinConst, BinHistMaxConst, nstrips, 0 - 0.5, nstrips - 0.5);
      if (LAPPDPlotsVerbosity > 3)
        cout << "Start Drawing event waveform with data size = " << boarddata.size() << endl;
      std::map<unsigned long, vector<Waveform<double>>>::iterator it;
      for (it = boarddata.begin(); it != boarddata.end(); it++)
      {

        unsigned long originalChannelNo = it->first;
        unsigned long channelNo = ((it->first % 1000)) % 60 + 1000;
        Waveform<double> w = it->second[0];
        Channel *ch = _geom->GetChannel(channelNo);
        int stripNo = ch->GetStripNum();
        if (LAPPDPlotsVerbosity > 3)
          cout << "Drawing channel " << originalChannelNo << " as channelNo" << channelNo << " strip " << stripNo << ", with sample size " << w.GetSamples()->size() << endl;

        if (!drawTriggerChannel)
          if ((channelNo % 1000) % 30 == 5)
            continue;

        for (int i = 0; i < w.GetSamples()->size(); i++)
        {
          h->Fill(-w.GetSamples()->at(i), stripNo + 1);
        }
      }
      if (LAPPDPlotsVerbosity > 3)
        cout << "Finish Drawing event waveform" << endl;

      h->SetStats(0);
      h->GetXaxis()->SetTitle("Amplitude (mV)");
      h->GetXaxis()->SetTitleSize(titleSize);
      h->GetXaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetYaxis()->SetTitle("Strip Number");
      h->GetYaxis()->SetTitleSize(titleSize);
      h->GetYaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetZaxis()->SetTitle("Entries");
      h->Draw("colz");

      f->cd();
      h->Write();
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << drawBoardID[i] << ", i=" << i << " in " << drawBoardID.size() << " finished for Draw Bin Hist" << endl;
    }
  }

  c->Modified();
  c->Update();

  if (eventNumber == 0)
    c->Print("LAPPDPlots.pdf(");
  else if (eventNumber == maxDrawEventNumber)
    c->Print("LAPPDPlots.pdf)");
  else if (eventNumber < maxDrawEventNumber)
    c->Print("LAPPDPlots.pdf");

  // if printEventNumber was set to be zero, print all events
  if (printEventWaveform && (eventNumber < printEventNumber || printEventNumber == 0))
  {
    vector<map<int, vector<double>>> LAPPDOnSide0;
    vector<map<int, vector<double>>> LAPPDOnSide1;
    LAPPDOnSide0.resize(printLAPPDNumber);
    LAPPDOnSide1.resize(printLAPPDNumber);

    vector<int> savedBoard;
    for (int i = 0; i < printLAPPDNumber * 2; i++)
    {
      savedBoard.push_back(0);
    }

    for (int i = 0; i < drawBoardID.size(); i++)
    {
      // data on board i, in key as channel number
      std::map<unsigned long, vector<Waveform<double>>> boarddata = GetDataForBoard(drawBoardID[i]);
      std::map<unsigned long, vector<Waveform<double>>>::iterator it;
      for (it = boarddata.begin(); it != boarddata.end(); it++)
      {
        unsigned long channelNo = it->first;
        Waveform<double> w = it->second[0];
        Channel *ch = _geom->GetChannel(channelNo);
        int stripSide = ch->GetStripSide();
        int stripNo = ch->GetStripNum();
        int lappd_id = static_cast<int>((channelNo - 1000) / 60);
        if (lappd_id < printLAPPDNumber)
        {
          vector<double> printW;

          for (int i = 0; i < w.GetSamples()->size(); i++)
          {
            printW.push_back(-w.GetSamples()->at(i));
          }
          if (LAPPDPlotsVerbosity > 0)
            cout << "LAPPD ID " << lappd_id << ", strip number " << stripNo << ", waveform size " << printW.size() << endl;
          if (stripSide == 0)
            LAPPDOnSide0[lappd_id][stripNo] = printW;
          else if (stripSide == 1)
            LAPPDOnSide1[lappd_id][stripNo] = printW;
          if (LAPPDPlotsVerbosity > 0)
            cout << "Saved waveform " << endl;

          savedBoard[drawBoardID[i]] = 1;
        }
      }
    }

    // loop savedBoard, if the board was not saved, add empty vector with 256 0 in it.
    for (int i = 0; i < savedBoard.size(); i++)
    {
      if (savedBoard[i] == 0)
      {
        int id = static_cast<int>(i / 2);

        vector<double> printW;
        for (int k = 0; k < 256; k++)
        {
          printW.push_back(0);
        }
        if (i % 2 == 0)
          LAPPDOnSide0[id][i] = printW;
        else
          LAPPDOnSide1[id][i] = printW;
      }
    }

    WaveformToPringSide0.push_back(LAPPDOnSide0);
    WaveformToPringSide1.push_back(LAPPDOnSide1);
  }

  if (printEventWaveform && printEventNumber != 0 && eventNumber == printEventNumber)
    PrintWaveformToTxt();

  eventNumber++;
  return true;
}

bool LAPPDPlots::Finalise()
{
  if (eventNumber < maxDrawEventNumber)
    c->Print("LAPPDPlots.pdf)");
  c->Clear();
  c->Close();
  delete c;

  f->cd();
  f->Close();
  delete f;

  if (printEventWaveform && printEventNumber == 0)
    PrintWaveformToTxt();

  return true;
}

std::map<unsigned long, vector<Waveform<double>>> LAPPDPlots::GetDataForBoard(int boardID)
{

  std::map<unsigned long, vector<Waveform<double>>> boarddata;
  for (auto &it : lappddata)
  {
    unsigned long channelNo = it.first;
    unsigned long forStripChannelNo = ((it.first % 1000)) % 60 + 1000;

    Channel *ch = _geom->GetChannel(forStripChannelNo);
    int stripSide = ch->GetStripSide();

    int thisLAPPDID = static_cast<int>((channelNo % 1000) / 60);

    int targetLAPPDID = static_cast<int>(boardID / 2);
    if (LoadLAPPDMap)
    {
      targetLAPPDID = thisLAPPDID;
    }

    if (LAPPDPlotsVerbosity > 4)
      cout << "GetData, channelNo: " << channelNo << ", side number" << stripSide << ", with input boardID " << boardID << ", targetLAPPDID " << targetLAPPDID << endl;

    if (static_cast<int>((channelNo % 1000) / 60) == targetLAPPDID && stripSide == boardID % 2)
    {
      boarddata[channelNo] = it.second;
      if (LAPPDPlotsVerbosity > 4)
        cout << "insearting data for channel " << channelNo << endl;
    }
  }

  return boarddata;
}

void LAPPDPlots::CleanObjects()
{
  lappddata.clear();
  ReadBoards.clear();
  LAPPD_ID = -1;
  inBeamWindow = -1;
  BGTiming = -1;
}

int LAPPDPlots::CheckInBeamgateWindow()
{
  unsigned long LAPPDDataBeamgateUL;
  unsigned long LAPPDDataTimeStampUL;
  bool got = m_data->CStore.Get("LAPPDBeamgate_Raw", LAPPDDataBeamgateUL);
  got = m_data->CStore.Get("LAPPDTimestamp_Raw", LAPPDDataTimeStampUL);

  if (got)
  {
    unsigned long Timing = (LAPPDDataTimeStampUL - LAPPDDataBeamgateUL) * 3.125;
    BGTiming = Timing;
    if (Timing > BeamWindowStart && Timing < BeamWindowEnd)
      return 1;
    else
      return 0;
  }
  else
    return -1;
}

void LAPPDPlots::PrintWaveformToTxt()
{
  // print all information in WaveformToPringSide0 and WaveformToPringSide1 to two txt files
  // print a header of printLAPPDNumber, printEventNumber
  ofstream LAPPDPlot_side0_eventWaveform;
  LAPPDPlot_side0_eventWaveform.open("LAPPDPlot_side0_eventWaveform.txt");
  LAPPDPlot_side0_eventWaveform << "printLAPPDNumber: " << printLAPPDNumber << " printEventNumber: " << printEventNumber << ", there should be " << printLAPPDNumber << " * " << printEventNumber << " * 256 lines" << endl;
  for (int i = 0; i < WaveformToPringSide0.size(); i++)
  {
    // print i_th event
    for (int j = 0; j < WaveformToPringSide0[i].size(); j++)
    {
      // print j_th LAPPD
      // loop the map, in each line, print i, j, key, and all values in vector
      for (auto it = WaveformToPringSide0[i][j].begin(); it != WaveformToPringSide0[i][j].end(); it++)
      {
        LAPPDPlot_side0_eventWaveform << i << " " << j << " " << it->first << " ";
        for (int k = 0; k < it->second.size(); k++)
        {
          LAPPDPlot_side0_eventWaveform << it->second[k] << " ";
        }
        LAPPDPlot_side0_eventWaveform << endl;
      }
    }
  }
  LAPPDPlot_side0_eventWaveform.close();

  ofstream LAPPDPlot_side1_eventWaveform;
  LAPPDPlot_side1_eventWaveform.open("LAPPDPlot_side1_eventWaveform.txt");
  LAPPDPlot_side1_eventWaveform << "printLAPPDNumber: " << printLAPPDNumber << " printEventNumber: " << printEventNumber << ", there should be " << printLAPPDNumber << " * " << printEventNumber << " * 256 lines" << endl;
  for (int i = 0; i < WaveformToPringSide1.size(); i++)
  {
    // print i_th event
    for (int j = 0; j < WaveformToPringSide1[i].size(); j++)
    {
      // print j_th LAPPD
      // loop the map, in each line, print i, j, key, and all values in vector
      for (auto it = WaveformToPringSide1[i][j].begin(); it != WaveformToPringSide1[i][j].end(); it++)
      {
        LAPPDPlot_side1_eventWaveform << i << " " << j << " " << it->first << " ";
        for (int k = 0; k < it->second.size(); k++)
        {
          LAPPDPlot_side1_eventWaveform << it->second[k] << " ";
        }
        LAPPDPlot_side1_eventWaveform << endl;
      }
    }
  }
}
//----------Author's Name:F-X Gentit
//----------Copyright:Those valid for ROOT software
//----------Modified:1/15/2004
#include "TDatime.h"
#include "TStyle.h"
#include "TOnePadDisplay.h"

TOnePadDisplay *gOneDisplay = 0;

//ClassImp(TOnePadDisplay)
//______________________________________________________________________________
//
//  This class is a general purpose class to display graphs or histograms on
//one pad only. Besides providing a name and a title, it also provides 3 labels
//whose content may be changed by the user.
//  Notice that all class variables are public. So it is easy for the user to
//change everything, until the display looks ok for his tastes. Changes have
//to be made between the call to the constructor and the call to BookCanvas().
//  Change of the 3 labels can be made AFTER the call to BookCanvas by the
//methods NewLabelXXX();
//
TOnePadDisplay::TOnePadDisplay() {
// Default constructor.
  Init();
}
TOnePadDisplay::TOnePadDisplay(const char *name,const char *title,Bool_t small):
  TNamed(name,title) {
// Name and title of TOnePadDisplay will also be name and title of fCanvas
// If small is true [default false], canvas will be small, adapted to small
//screens
  Init();
  if (small) SetSmall();
}
TOnePadDisplay::TOnePadDisplay(const char *name,const char *title,const char *t1,Bool_t small):
  TNamed(name,title) {
// Name and title of TOnePadDisplay will also be name and title of fCanvas
// Constructor allowing to set yourself the text inside labels 1.
// If small is true [default false], canvas will be small, adapted to small
//screens
  Init();
  fTextT1 = t1;
  if (small) SetSmall();
}
TOnePadDisplay::TOnePadDisplay(const char *name,const char *title,const char *t1,
  const char *t2,Bool_t small):TNamed(name,title) {
// Name and title of TOnePadDisplay will also be name and title of fCanvas
// Constructor allowing to set yourself the text inside labels 1 and 2.
// If small is true [default false], canvas will be small, adapted to small
//screens
  Init();
  fTextT1 = t1;
  fTextT2 = t2;
  if (small) SetSmall();
}
TOnePadDisplay::TOnePadDisplay(const char *name,const char *title,const char *t1,
  const char *t2,const char *t3,Bool_t small):TNamed(name,title) {
// Name and title of TOnePadDisplay will also be name and title of fCanvas
// Constructor allowing to set yourself the text inside labels 1, 2 and 3.
// If small is true [default false], canvas will be small, adapted to small
//screens
  Init();
  fTextT1 = t1;
  fTextT2 = t2;
  fTextT3 = t3;
  if (small) SetSmall();
}
TOnePadDisplay::~TOnePadDisplay() {
// Not sure whether I should delete fPad or the delete of fCanvas will delete
//fPad. Idem for fTexN. If crash, remove delete fPad!
  if (fTex1)   delete fTex1;
  if (fTex2)   delete fTex2;
  if (fTex3)   delete fTex3;
  if (fPad)    delete fPad;
  if (fCanvas) delete fCanvas;
  gOneDisplay = 0;
}
void TOnePadDisplay::BookCanvas() {
//Booking of labels, canvasand pad and setting the styles
  BookLabels();
//canvas
  fCanvas = new TCanvas(GetName(),GetTitle(),fCanTopX,fCanTopY,fCanWidth,fCanHeigth);
  fCanvas->Range(0,0,1,1);
  fCanvas->SetFillColor(fCanColor);
  fCanvas->SetBorderSize(fCanBsz);
  fCanvas->SetFillStyle(fCanStyle);
  DrawLabels();
//pad
  fPad = new TPad("OnePad","OnePad",fPadXlow,fPadYlow,fPadXup,fPadYup);
  fPad->SetFillColor(fPadColor);
  fPad->SetBorderSize(fPadBsz);
  fPad->SetGridx();
  fPad->SetGridy();
  fPad->SetFrameFillColor(fFrameColor);
  fPad->SetFillStyle(fPadStyle);
  fPad->SetLogx(fPadLogX);
  fPad->SetLogy(fPadLogY);
//styles
  gStyle->SetOptStat(fStyleStat);
  gStyle->SetStatFont(fStyleFont);
  gStyle->SetStatColor(fStyleColor);
  gStyle->SetStatH(fStyleH);
  gStyle->SetTitleXSize(fStyleTXSize);
  gStyle->SetTitleXOffset(fStyleTXOffset);
  gStyle->SetTitleYSize(fStyleTYSize);
  gStyle->SetTitleYOffset(fStyleTYOffset);
  gStyle->SetTitleH(fStyleTitleH);
  gStyle->SetTitleW(fStyleTitleW);
  gStyle->SetTitleX(fStyleTitleX);
  gStyle->SetTitleY(fStyleTitleY);
  gStyle->SetTitleBorderSize(fStyleTBSize);
  gStyle->SetTitleFillColor(fStyleTFColor);
  gStyle->SetTitleW(fStyleW);
  gStyle->SetHistFillColor(fStyleHistColor);
  gStyle->SetTitleFont(fStyleTitleFont);
  gStyle->SetTitleColor(fStyleTitleColor);
  gStyle->SetTitleTextColor(fStyleTTextColor);
  gStyle->SetLabelSize(fStyleLabelSize,fStyleLabelAxis);
  fPad->Draw();
  fPad->cd();
}
void TOnePadDisplay::BookLabels() {
//Booking of the 3 labels
  fTex1 = new TLatex(fXTex1,fYTex1,fTextT1.Data());
  fTex2 = new TLatex(fXTex2,fYTex2,fTextT2.Data());
  fTex3 = new TLatex(fXTex3,fYTex3,fTextT3.Data());
  fTex1->SetTextFont(fFontTex);
  fTex1->SetTextSize(fSizeTex);
  fTex1->SetLineWidth(fWidthTex);
  fTex2->SetTextFont(fFontTex);
  fTex2->SetTextSize(fSizeTex);
  fTex2->SetLineWidth(fWidthTex);
  fTex3->SetTextFont(fFontTex);
  fTex3->SetTextSize(fSizeTex);
  fTex3->SetLineWidth(fWidthTex);
}
void TOnePadDisplay::DrawLabels() const {
//Draws the 2 labels on the Canvas
  fCanvas->cd();
  fTex1->Draw();
  fTex2->Draw();
  fTex3->Draw();
}
void TOnePadDisplay::Init() {
//Initialization and default options
  Int_t day,month,year,date;
  TDatime *td;
//Date
  td = new TDatime();
  date  = td->GetDate();
  day   = date % 100;
  date /= 100;
  month = date % 100;
  date /= 100;
  year  = date;
  delete td;
  fCanDate  = "";
  fCanDate += day;
  fCanDate.Append(" / ");
  fCanDate += month;
  fCanDate.Append(" / ");
  fCanDate += year;
//Pointers to 0
  fCanvas          = 0;
  fTex1            = 0;
  fTex2            = 0;
  fTex3            = 0;
  fPad             = 0;
//initialization for main canvas
  fCanTopX         = 2;
  fCanTopY         = 2;
  fCanWidth        = 1000; //old 1178
  fCanHeigth       = 700;  //old 770
  fCanColor        = 20;
  fCanBsz          = 12; //4
  fCanStyle        = 1000;
//initialization for pad
  fPadXlow         = 0.015;
  fPadXup          = 0.985;
  fPadYlow         = 0.05;
  fPadYup          = 0.95;
  fPadColor        = 11;
  fPadBsz          = 6;
  fPadStyle        = 1000;
  fPadLogX         = 0;
  fPadLogY         = 0;
//initialization for frame in pad
  fFrameColor      = 171;
//initialization for Style
  fStyleStat       = 1111;
  fStyleFont       = 22;
  fStyleColor      = 171;
  fStyleH          = 0.1;
  fStyleW          = 0.65;//0.76
  fStyleHistColor  = 42;
  fStyleTXSize     = 0.035;//0.035
  fStyleTYSize     = 0.035;//0.035
  fStyleTitleH     = 0.04;//0.04
  fStyleTitleW     = 0.95;
  fStyleTitleX     = 0.12;
  fStyleTXOffset   = 1.0;
  fStyleTitleY     = 0.975;
  fStyleTYOffset   = 1.0;
  fStyleTBSize     = 4;
  fStyleTitleFont  = 22;
  fStyleTitleColor = 1;
  fStyleTFColor    = 191;
  fStyleTTextColor = 1;
  fStyleLabelSize  = 0.035;
  fStyleLabelAxis  = "XYZ";
//labels
  fTextT1          = "SplineFit : General Purpose Fit System";
  fXTex1           = 0.04;
  fYTex1           = 0.96;
  fTextT2          = "F.X.Gentit DAPNIA/SPP CEA Saclay";
  fXTex2           = 0.04;
  fYTex2           = 0.025;
  fTextT3          = "TOnePadDisplay   ";
  fTextT3.Append(fCanDate);
  fXTex3           = 0.8;
  fYTex3           = 0.025;
  fFontTex         = 72;
  fSizeTex         = 0.022419;
  fWidthTex        = 2;
  gOneDisplay      = this;
}
void TOnePadDisplay::NewLabel1(const char *t) {
// Modifies text of first label
  fTex1->SetTitle(t);
  fCanvas->cd();
  fCanvas->Modified();
  fCanvas->Update();
  fPad->cd();
}
void TOnePadDisplay::NewLabel2(const char *t) {
// Modifies text of first label
  fTex2->SetTitle(t);
  fCanvas->cd();
  fCanvas->Modified();
  fCanvas->Update();
  fPad->cd();
}
void TOnePadDisplay::NewLabel3(const char *t) {
// Modifies text of first label
  fTex3->SetTitle(t);
  fCanvas->cd();
  fCanvas->Modified();
  fCanvas->Update();
  fPad->cd();
}
void TOnePadDisplay::NewLabel12(const char *t1,const char *t2) {
// Modifies text of first label
  fTex1->SetTitle(t1);
  fTex2->SetTitle(t2);
  fCanvas->cd();
  fCanvas->Modified();
  fCanvas->Update();
  fPad->cd();
}
void TOnePadDisplay::NewLabels(const char *t1,const char *t2,const char *t3) {
// Modifies text of first label
  fTex1->SetTitle(t1);
  fTex2->SetTitle(t2);
  fTex3->SetTitle(t3);
  fCanvas->cd();
  fCanvas->Modified();
  fCanvas->Update();
  fPad->cd();
}
void TOnePadDisplay::SetSmall() {
//Choose a small canvas, adequate for sceens with few pixels
  fCanWidth        = 749;
  fCanHeigth       = 450;
  fPadXlow         = 0.025;
  fPadXup          = 0.975;
  fPadYlow         = 0.07;
  fPadYup          = 0.93;
  fYTex1           = 0.94;
  fYTex2           = 0.035;
  fYTex3           = 0.035;
}

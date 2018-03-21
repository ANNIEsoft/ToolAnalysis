#ifndef SPLF_TOnePadDisplay
#define SPLF_TOnePadDisplay
#include "TCanvas.h"
#include "TPad.h"
#include "TLatex.h"

class TOnePadDisplay : public TNamed {

protected:

  void Init();

public:

  TCanvas  *fCanvas;          //Main canvas
  Int_t     fCanTopX;         //top x  of general canvas fCanvas
  Int_t     fCanTopY;         //top y  of general canvas fCanvas
  Int_t     fCanWidth;        //width  of general canvas fCanvas
  Int_t     fCanHeigth;       //heigth of general canvas fCanvas
  Color_t   fCanColor;        //color  of general canvas fCanvas
  Short_t   fCanBsz;          //border size of main canvas fCanvas
  Short_t   fCanStyle;        //style of canvas
  TString   fCanDate;         //date of to-day put in 3rd label

  TPad     *fPad;             //pad inside fCanvas
  Double_t  fPadXlow;         //x low of fPad inside fCanvas
  Double_t  fPadXup;          //x up of fPad inside fCanvas
  Double_t  fPadYlow;         //y low of fPad inside fCanvas
  Double_t  fPadYup;          //y low of fPad inside fCanvas
  Color_t   fPadColor;        //color of Pad
  Short_t   fPadBsz;          //border size of fPad
  Short_t   fPadStyle;        //style of fPad
  Int_t     fPadLogX;         //log display in x
  Int_t     fPadLogY;         //log display in x
  Color_t   fFrameColor;      //color of frame of fPad

  Int_t     fStyleStat;       //stat
  Style_t   fStyleFont;       //font of stat
  Int_t     fStyleColor;      //color of stat
  Float_t   fStyleH;          //height of font in stat
  Float_t   fStyleW;          //width of font in stat
  Color_t   fStyleHistColor;  //fill color of histograms
  Float_t   fStyleTXSize;     //size of title
  Float_t   fStyleTYSize;     //size of title
  Float_t   fStyleTitleH;     //height of title
  Float_t   fStyleTitleW;     //width of title
  Float_t   fStyleTitleX;     //x of title
  Float_t   fStyleTXOffset;   //offset of x title
  Float_t   fStyleTitleY;     //H of title
  Float_t   fStyleTYOffset;   //offset of x title
  Width_t   fStyleTBSize;     //title Border size
  Style_t   fStyleTitleFont;  //font of title
  Color_t   fStyleTitleColor; //color of title
  Color_t   fStyleTFColor;    //fill color for title
  Color_t   fStyleTTextColor; //color text of title
  Float_t   fStyleLabelSize;  //size of label
  Option_t *fStyleLabelAxis;  //axis type of label

  TLatex   *fTex1;            //user updatable label 1 on top    left  of main canvas
  TString   fTextT1;          //text in fTex1
  Double_t  fXTex1;           //x position of label 1
  Double_t  fYTex1;           //y position of label 1
  TLatex   *fTex2;            //user updatable label 2 on bottom left  of main canvas
  TString   fTextT2;          //text in fTex2
  Double_t  fXTex2;           //x position of label 2
  Double_t  fYTex2;           //y position of label 2
  TLatex   *fTex3;            //fixed          label 3 on bottom right of main canvas
  TString   fTextT3;          //text in fTex3
  Double_t  fXTex3;           //x position of label 3
  Double_t  fYTex3;           //y position of label 3
  Font_t    fFontTex;         //font used for label
  Float_t   fSizeTex;         //size of label font
  Width_t   fWidthTex;        //line width for label text

  TOnePadDisplay();
  TOnePadDisplay(const char*,const char*,Bool_t);
  TOnePadDisplay(const char*,const char*,const char*,Bool_t=kFALSE);
  TOnePadDisplay(const char*,const char*,const char*,const char*,Bool_t=kFALSE);
  TOnePadDisplay(const char*,const char*,const char*,const char*,const char*,Bool_t=kFALSE);
  virtual ~TOnePadDisplay();
  virtual void BookCanvas();
  virtual void BookLabels();
  virtual void DrawLabels() const;
  void NewLabel1(const char*);
  void NewLabel2(const char*);
  void NewLabel3(const char*);
  void NewLabel12(const char*,const char*);
  void NewLabels(const char*,const char*,const char*);
  virtual void SetSmall();
  //ClassDef(TOnePadDisplay,1) //Class for displaying results on one pad
};

R__EXTERN TOnePadDisplay *gOneDisplay;

#endif

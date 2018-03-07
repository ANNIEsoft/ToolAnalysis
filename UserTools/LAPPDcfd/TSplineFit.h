#ifndef SPLF_TSplineFit
#define SPLF_TSplineFit
#include "TArrayI.h"
#include "TArrayD.h"
#include "TNamed.h"
#include "TGraphErrors.h"
#include "TMatrixD.h"
#include "TObjArray.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TSpline.h"
#include "TZigZag.h"

Double_t SplineFitFunc(Double_t *,Double_t *);

enum KindOfFit {NotDefined,LinInterpol,SplineInterpol,SplineFit1D,SplineFit2D,SplineFit3D};

class TSplineFit : public TNamed {

protected:

  KindOfFit          fType;          //Type of fit or interpolation
  Int_t              fCat;           //Category to which this spline fit belongs (arbitrary)
  Int_t              fNbInFamily;    //-1 if fit not in a family,  0<=fFamylyNb<n if n elements in the family
  Int_t              fM;             //Number of measurements (xi,yi,si)
  Int_t              fMi;            //Number of measurements (xi,yi,si) per spline (or per sub-interval)
  Int_t              fMl;            //Number of measurements (xi,yi,si) for the last spline
  Int_t              fN;             //Number of splines (or sub-intervals)
  Int_t              fNs2;           //Biggest power of 2 less than fN;
  Double_t           fSlope;         //Slope of linear transformation to go to normalized x [-1,1]
  Double_t           fCst;           //Constant term of linear transformation to go to normalized x [-1,1]
  Bool_t             fBoundedLow;    //True if y has a lower bound
  Double_t           fLowBound;      //Lower bound for y
  Bool_t             fBoundedUp;     //True if y has an upper bound
  Double_t           fUpBound;       //High bound for y
  TMatrixD           fA;             //! Band matrix of the problem, in compact form. See TBandedLE
  TMatrixD           fB;             //! Matrix of the right-hand side of the problem
  TArrayD            fX;             //solution of the problem
  TArrayD            fKhi;           //fKhi[k-1] = xmin for sub-interval k. fKhi[k] =  xmax for sub-interval k.
  TArrayD            fMt;            //coordinate t of measurements. In case of 1D fit t==x
  TArrayD            fMv;            //measured values at x
  TArrayD            fMs;            //errors on y measurements
  Bool_t             fUseForRandom;  //If true, fit used to generate random numbers
  Double_t           fParameter;     //Variable or parameter associated with this fit
  TString            fParameterDef;  //Definition of the parameter [the same for all members of a family]
  TZigZag           *fZigZag;        //For use with 2D or 3D fits
  TString            fDate;          //Date of creation of this fit
  TString            fSource;        //Source of the measurements
  TString            fMacro;         //Name of CINT macro having produced this fit
  TString            fXLabel;        //Label of the x axis
  TString            fYLabel;        //Label of the x axis
  TString            fZLabel;        //Label of the x axis
  TString            fVLabel;        //Label of the x axis
  TSpline3          *fInterpolation; //! TSpline3 used in case the user asks for an interpolation, not a fit
  TH1D              *fHGenRandom;    //! Histogram used to get random numbers according to the fitted distribution
  TH1D              *fHShowRandom;   //! Histogram used to display the random numbers generated
  Bool_t             fMemoryReduced; //! Everything deleted, except the fit. In case you lack of memory
  TString            fProvidedName;  //! Name of fProvidedH1D, kept in case user delete histo
  TF1               *fSplineFitFunc; //! function given to provided histo, to show fit on histo
  TGraphErrors      *fPointsGraph;   //! Graph for drawing measurements
  TGraph            *fSplineGraph;   //! Graph for drawing fit
  TMultiGraph       *fPS;            //! Multi graph containing both
  Bool_t             f2Drestored;    //! True if fProvidedH2D has been restored
  Bool_t             f3Drestored;    //! True if fProvidedH3D has been restored
  TH1D              *fProvidedH1D;   //! Histo provided in 4th or 5th constructor. Not owned by TSplineFit
  TH2D              *fProvidedH2D;   //! Histo provided in 7th constructor. Not owned by TSplineFit
  TH2D              *fH2Dfit;        //! Histo to show the 2D fit
  TH3D              *fProvidedH3D;   //! Histo provided in 8th constructor. Not owned by TSplineFit
  TH3D              *fH3Dfit;        //! Histo to show the 3D fit

  static Int_t       fgNextDraw;     //Next fit to be drawn
  static Int_t       fgM;            //order M of band matrix fA, always equal to 6
  static Int_t       fgU;            //number of unknowns for one spline, with Lagrange parameters, always equal to 7
  static TArrayI    *fgCat;          //Array of categories
  static TFile      *fgFitFile;      //"Database" file containing fits
  static TTree      *fgFitTree;      //Tree of fits
  static TBranch    *fgFitBranch;    //Branch of fits

  static void        AddFit(TSplineFit*);
  virtual void       AddThisFit();
  virtual void       AdjustErrors();
  static void        AdjustfgCat();
  Double_t           Alpha(Int_t,Int_t,Int_t) const;
  Bool_t             AlreadySeen(TString&,TString&) const;
  void               ApplyLowBound(TArrayD&) const;
  Double_t           Beta(Int_t,Int_t) const;
  virtual void       ClearGraphs(Bool_t = kTRUE);
  virtual void       CutLinkWithHisto();
  Bool_t             ExtractPrefixN(TString&) const;
  Bool_t             ExtractPrefixT(TString&) const;
  void               Fill();
  void               FindDate();
  Int_t 		   GetDimensionHist(Double_t &,Double_t &,Double_t,TH1D *); //XZ
  virtual void       GetDataFromHist(Double_t,Double_t,Double_t,TH1D *);
  void               Init();
  virtual Bool_t     InitCatAndBounds(Int_t,Bool_t,Double_t,Bool_t,Double_t);
  virtual Bool_t     InitCheckBounds();
  virtual void       InitDimensions(Int_t,Int_t);
  virtual void       InitIntervals(Double_t,Double_t);
  virtual Int_t      Interpolation();
  virtual Int_t      Interval(Double_t) const;
  virtual void       MinMax(Int_t,Bool_t&,Double_t&,Double_t&,Bool_t&,Double_t&,Double_t&) const;
  void               Regenerate();
  virtual Bool_t     RestoreHisto();
  Int_t              Solve(Bool_t=kFALSE);
  void               VerifyNT();

public:

  static TObjArray  *fgFits;         //Collection of all TSplineFits
  static TString    *fgProgName;     //Name of this software
  static TString    *fgWebAddress;   //Web address for SplineFit
  static TString    *fgFileName;     //Name or TreeName of "database" file containing fits
  static Int_t       fgNChanRand;    //number of channels for the 2 histograms fHGenRandom and fHShowRandom
  static Int_t       fgCounter;      //new versus delete

  TSplineFit();
  TSplineFit(Text_t*,Text_t*,Int_t,Int_t,Int_t,Double_t*,Double_t*,Double_t* = 0,
    Bool_t=kFALSE, Double_t = 0.0,Bool_t=kFALSE, Double_t = 1.0,
    Double_t = 1.0, Double_t = -1.0,Bool_t = kFALSE);
  TSplineFit(Text_t*,Text_t*,Int_t,Int_t,Double_t*,Double_t*,
    Double_t = 1.0, Double_t = -1.0);
  TSplineFit(Text_t*,Text_t*,Int_t,Int_t,TGraphErrors *,
    Bool_t=kFALSE, Double_t = 0.0,Bool_t=kFALSE, Double_t = 1.0,
    Double_t = 1.0, Double_t = -1.0,Bool_t = kFALSE);
  TSplineFit(Text_t*,Text_t*,Int_t,Int_t,TH1D *,Double_t = 0, Double_t = 1032, //5th construction
    Bool_t=kFALSE, Double_t = 0.0,Bool_t=kFALSE, Double_t = 1.0,Bool_t = kFALSE);
  TSplineFit(TH1D *,Int_t,Int_t,
    Bool_t=kFALSE, Double_t = 0.0,Bool_t=kFALSE, Double_t = 1.0,Bool_t = kFALSE);
  TSplineFit(Text_t*,Text_t*,Int_t,Int_t,TH2D *,
    Bool_t=kFALSE, Double_t = 0.0,Bool_t=kFALSE, Double_t = 1.0,
    Bool_t = kFALSE);
  TSplineFit(Text_t*,Text_t*,Int_t,Int_t,TH3D *,
    Bool_t=kFALSE, Double_t = 0.0,Bool_t=kFALSE, Double_t = 1.0,
    Bool_t = kFALSE);
  TSplineFit(const TSplineFit&);
  virtual ~TSplineFit();
  static void        AddNumbering(Int_t,TString&);
  void               BelongsToFamily(Int_t,Double_t,Text_t*);
  static Bool_t      CheckHistErrors(TH1*,Bool_t=kFALSE);
  virtual Double_t   Chi2(Bool_t = kFALSE) const;
  Int_t              Compare(const TObject*) const;
  virtual void       DrawData(Option_t *option="LEGO2");
  virtual void       DrawFit(Option_t *option="LEGO2",Int_t=1,Int_t=1,Int_t=1);
  static  void       DrawFitsInCollection();
  static  void       DrawFitsInFile();
  virtual void       DrawHere(Double_t =1.0,Double_t = -1.0);
  virtual void       DrawHisto(Option_t *option="LEGO2");
  static  void       DrawNextInCollection();
  virtual void       ErrorsFromFit();
  virtual void       FillGraphs(Int_t=200,Color_t=4,Style_t=21);
  virtual Bool_t     FillH2D3D(Int_t=1,Int_t=1,Int_t=1);
  static TSplineFit *FindFit(const Text_t*,Int_t=-1,Bool_t = kTRUE);
  TSplineFit        *FindFirstInFamily(Int_t&);
  static TSplineFit *FindFirstInFamily(const Text_t *,Int_t&);
  static TSplineFit *FindFitInFamily(Int_t,Int_t,Int_t);
  Int_t              GetCategory() const            { return fCat;                 }
  Double_t           GetCst() const                 { return fCst;                 }
  virtual void       GetDataFromHist(Double_t);
  const char        *GetDate() const                { return fDate.Data();         }
  Text_t            *GetFamilyName() const;
  TH1D              *GetHistGen()                   { return fHGenRandom;          }
  TH1D              *GetHistShow()                  { return fHShowRandom;         }
  KindOfFit          GetKindOfFit()                 { return fType;                }
  Bool_t             GetLowBound(Double_t&) const;
  const char        *GetMacro() const               { return fMacro.Data();        }
  Double_t           GetMeasErr(Int_t i) const      { return fMs[i];               }
  Double_t           GetMeasT(Int_t i) const        { return fMt[i];               }
  Double_t           GetMeasV(Int_t i) const        { return fMv[i];               }
  Int_t              GetNbOfMeas() const            { return fM;                   }
  Int_t              GetNbOfMeasLast() const        { return fMl;                  }
  Int_t              GetNbOfMeasSpline() const      { return fMi;                  }
  Int_t              GetNbOfSplines() const         { return fN;                   }
  Double_t           GetParameter() const           { return fParameter;           }
  const char        *GetParameterDef()              { return fParameterDef.Data(); }
  Int_t              GetPosInFamily() const         { return fNbInFamily;          }
  const char        *GetProvidedName() const        { return fProvidedName.Data(); }
  Double_t           GetRandom() const;
  Double_t           GetRandom(Int_t,Double_t);
  Double_t           GetSlope() const               { return fSlope;               }
  const char        *GetSource() const              { return fSource.Data();       }
  virtual Bool_t     GetSpline(Int_t,Double_t&,Double_t&,Double_t&,Double_t&) const;
  virtual Bool_t     GetSpline(Double_t,Double_t&,Double_t&,Double_t&,Double_t&) const;
  virtual Bool_t     GetSplineNN(Int_t,Double_t&,Double_t&,Double_t&,Double_t&) const;
  virtual Bool_t     GetSplineNN(Double_t,Double_t&,Double_t&,Double_t&,Double_t&) const;
  Bool_t             GetUpBound(Double_t&) const;
  const char        *GetVLabel() const              { return fVLabel.Data();       }
  const char        *GetXLabel() const              { return fXLabel.Data();       }
  Double_t           GetXLowInterval(Int_t i) const;
  Double_t           GetXmax() const                { return fKhi[fN];             }
  Double_t           GetXmin() const                { return fKhi[0];              }
  Double_t           GetXUpInterval(Int_t i) const;
  const char        *GetYLabel() const              { return fYLabel.Data();       }
  const char        *GetZLabel() const              { return fZLabel.Data();       }
  static void        InitStatic();
  Double_t           Integral(Double_t,Double_t);
  Bool_t             IsEqual(const TObject*) const;
  Bool_t             IsInCollection() const;
  static Bool_t      IsInCollection(TSplineFit*);
  Bool_t             IsSortable() const { return kTRUE; }
  Int_t              LoadFamily() const;
  static Int_t       LoadFamily(const Text_t *);
  virtual void       MinMax(Bool_t&,Double_t&,Double_t&,Bool_t&,Double_t&,Double_t&) const;
  static void        MultinomialAsWeight(TH1 *);
//  static void        NameFile(Text_t* = "SplineFitDB.rdb");
//  static void        NameProg(Text_t* = "SplineFit");
//  static void        NameWeb(Text_t*  = "SplineFit gentit.home.cern.ch/gentit/splinefit/index.html");
  static void        NameFile(TString name = "SplineFitDB.rdb");
  static void        NameProg(TString name = "SplineFit");
  static void        NameWeb(TString name = "SplineFit gentit.home.cern.ch/gentit/splinefit/index.html");
  static Bool_t      OrderFile(Bool_t = kTRUE);
  Double_t           Pedestal(Double_t,Double_t);
  virtual void       Print();
  static void        Purge();
  static void        PurgeStatic();
  virtual Int_t      RedoFit(Bool_t = kFALSE);
  virtual void       ReduceMemory();
  static void        RemoveDisplay();
  static Bool_t      RemoveFitFromFile(Text_t*,Bool_t=kTRUE);
  virtual void       SetDefaultLabels();
  void               SetMacro(Text_t*);
  void               SetMeasErr(Int_t i,Double_t E) { fMs[i] = E;             }
  void               SetMeasX(Int_t i,Double_t x)   { fMt[i] = x;             }
  void               SetMeasY(Int_t i,Double_t y)   { fMv[i] = y;             }
  void               SetParameter(Double_t,Text_t*);
  void               SetSource(Text_t*);
  void               SetVLabel(Text_t*);
  void               SetXLabel(Text_t*);
  void               SetYLabel(Text_t*);
  void               SetZLabel(Text_t*);
  static void        ShowFitsInFile();
  virtual void       ShowRandom() const;
  Bool_t             SolveLeft(Double_t&,Double_t=0.0,Bool_t=kFALSE,Double_t = -1.0,Double_t=1.0);
  virtual Bool_t     UpdateFile(Bool_t=kFALSE);
  Bool_t             UseForRandom(Bool_t = kTRUE);
  Double_t           V(Double_t) const;
  Double_t           V(Double_t,Double_t) const;
  Double_t           V(Double_t,Double_t,Double_t);
  Double_t           V(Int_t,Double_t,Double_t);
  Bool_t             VerifyNotInFile() const;
  Double_t           XNorm(Double_t x) const        { return fSlope*x + fCst; }
  static Double_t    XpowerM(Double_t,Int_t);
  Double_t           XUser(Double_t x) const        { return (x-fCst)/fSlope; }
  //ClassDef(TSplineFit,1)  //General Handling of Spline fits
};

R__EXTERN TSplineFit *gSplineFit;

#endif

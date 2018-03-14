#ifndef SPLF_TZigZag
#define SPLF_TZigZag
#include "TObject.h"
#include "TArrayI.h"
#include "TArrayD.h"

class TZigZag {

protected:

  Short_t  fDim;     //dimension: 1,2 or 3!
  Int_t    fNx;      //Nb. of points along x
  Int_t    fNy;      //Nb. of points along y
  Int_t    fNz;      //Nb. of points along z
  Double_t fXmin;    //low bound in x
  Double_t fYmin;    //low bound in y
  Double_t fZmin;    //low bound in z
  Double_t fXmax;    //up  bound in x
  Double_t fYmax;    //up  bound in y
  Double_t fZmax;    //up  bound in z

  void Init();

public:

  TZigZag();
  TZigZag(Int_t,Double_t,Double_t);
  TZigZag(Int_t,Double_t,Double_t,Int_t,Double_t,Double_t);
  TZigZag(Int_t,Double_t,Double_t,Int_t,Double_t,Double_t,Int_t,Double_t,Double_t);
  TZigZag(const TZigZag&);
  virtual ~TZigZag() { ; }
  Int_t          GetNx()   const { return fNx;   }
  Int_t          GetNy()   const { return fNy;   }
  Int_t          GetNz()   const { return fNz;   }
  Double_t       GetXmin() const { return fXmin; }
  Double_t       GetXmax() const { return fXmax; }
  Double_t       GetYmin() const { return fYmin; }
  Double_t       GetYmax() const { return fYmax; }
  Double_t       GetZmin() const { return fZmin; }
  Double_t       GetZmax() const { return fZmax; }
  virtual Bool_t IsInside(Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,
                    Double_t,Double_t) const;
  virtual Bool_t IsInside(Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,
                    Double_t,Double_t,Double_t,Double_t,Double_t,Double_t,Double_t) const;
  virtual Bool_t NearestPoints(Double_t, TArrayI&, TArrayD&) const;
  virtual Bool_t NearestPoints(Double_t, Double_t, Double_t, TArrayI&, TArrayD&) const;
  Int_t          NToZZ(Int_t) const;
  Int_t          NToZZ(Int_t,Int_t) const;
  Int_t          NToZZ(Int_t,Int_t,Int_t) const;
  void           Order(Int_t,TArrayI&, TArrayD&, TArrayD&, TArrayD&) const;
  virtual Bool_t PointsNear(Double_t,Double_t,Double_t&,TArrayD&, TArrayD&) const;
  Double_t       T(Int_t i) const;
  Double_t       TMax() const;
  //ClassDef(TZigZag,1)  //Labelling of points such that point i+1 always near from point i
};
#endif

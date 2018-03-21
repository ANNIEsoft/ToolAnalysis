#ifndef SPLF_TBandedLE
#define SPLF_TBandedLE
#include "TVectorD.h"
#include "TMatrixD.h"

class TBandedLE {

protected:

  Int_t     fF;    //0  : everything ok. | -1 : the matrix was singular | -2 : the provided arguments of the constructor were bad
  Int_t     fN;    //Number of equations
  Int_t     fM;    //Band parameter. Band size is 2*fM + 1;
  Int_t     fK;    //Number of right-hand sides (columns) in matrix fB
  Bool_t    fOwnA; //! True if "this" is owner of fA: for instance when "this" read from file
  Bool_t    fOwnB; //! True if "this" is owner of fB: for instance when "this" read from file or fB has not been provided by the user

  void Compactify();
  void Init();

public:

  TMatrixD *fA;    //User provided matrix A. Remains untouched.
  TMatrixD *fB;    //User provided matrix B. Remains untouched
  TMatrixD  fX;    //Solution of the problem
  TVectorD  fV;    //Solution of the problem in case fK==1

  TBandedLE();
  TBandedLE(Int_t,Int_t,Int_t,TMatrixD&,TMatrixD&,Bool_t = kTRUE);
  TBandedLE(TMatrixD&,TMatrixD&,Int_t,Bool_t = kTRUE);
  TBandedLE(TMatrixD&,Int_t,Bool_t = kTRUE);
  virtual ~TBandedLE();
  TMatrixD *GetfA() { return fA; }
  TMatrixD *GetfB() { return fB; }
  Int_t     GetfF() { return fF; }
  Int_t     GetfK() { return fK; }
  Int_t     GetfM() { return fM; }
  Int_t     GetfN() { return fN; }
  Int_t     Solve();
  Double_t  Verify() const;
  //ClassDef(TBandedLE,1)  //CERN program F406 DBEQN translated to C++
};
#endif

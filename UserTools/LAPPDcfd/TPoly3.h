#ifndef LIT_TPoly3
#define LIT_TPoly3
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TPoly3  Handling of 3rd order polynoms                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
#include "TObject.h"

class TPoly3 {

protected:

  Double_t fA[4];     //Coefficients of the polynom

  void Init();
  void Solve2(Double_t = 0.0);
  void Solve3(Double_t = 0.0);

public:

  Short_t  fN;        //! True degree of polynom. -1 if not solved!
  Bool_t   fComp;     //! True if there are 2 complex solutions
  Double_t fY0;       //! Value of y0 in y0 = fA[0] + fA[1]*x + fA[2]*x^2 + fA[3]*x^3 for which problem was solved
  Double_t fX1;       //! First solution, always real
  Double_t fX2;       //! 2nd solution, if real. Else real part of second complex solution
  Double_t fX3;       //! 3rd solution, if real. Else im part of 2nd solution or minus im part of 3rd solution
  Bool_t   fMin;      //! True if there is a minimum
  Double_t fXMin;     //! Value of x at minimum, if any
  Double_t fYMin;     //! Value of y at minimum, if any
  Bool_t   fMax;      //! True if there is a maximum
  Double_t fXMax;     //! Value of x at maximum, if any
  Double_t fYMax;     //! Value of y at maximum, if any
  Bool_t   fInfl;     //! True if there is an inflection point
  Double_t fXInfl;    //! Value of x at inflexion point
  Double_t fYInfl;    //! Value of y at inflexion point

  TPoly3() { Init(); }
  TPoly3(Double_t,Double_t,Double_t,Double_t);
  TPoly3(Double_t*);
  Short_t  Degree();
  void     Extrema();
  Double_t Integral(Double_t,Double_t);
  void     Set(Double_t,Double_t,Double_t,Double_t);
  void     Set(Double_t*);
  Short_t  Solution(Double_t&,Double_t&,Double_t&,Double_t&,Bool_t&);
  void     Solve(Double_t=0.0);
  Short_t  Solve(Double_t&,Double_t&,Double_t&,Bool_t&,Double_t = 0.0);
  Bool_t   SolveLeft(Double_t&,Double_t=0.0,Bool_t=kFALSE,Double_t = -1.0,Double_t=1.0);
  Bool_t   SolveRight(Double_t&,Double_t=0.0,Bool_t=kFALSE,Double_t = -1.0,Double_t=1.0);
  Double_t Y(Double_t);
  //ClassDef(TPoly3,1)  //Handling of 3rd order polynoms
};
#endif

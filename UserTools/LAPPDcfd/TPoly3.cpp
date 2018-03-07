//----------Author's Name:F.X.Gentit  DAPNIA/SPP CEN Saclay
//----------Copyright:Those valid for CNRS sofware
//----------Modified:21/03/2005
#include "TMath.h"
#include "TPoly3.h"

//ClassImp(TPoly3)
//______________________________________________________________________________
//
//  Handling of 3rd order polynoms
//
TPoly3::TPoly3(Double_t a0,Double_t a1,Double_t a2,Double_t a3) {
  // Constructor with coefficients of the polynom.
  //  a0 constant term
  //  a1 factor of x
  //  a2 factor of x^2
  //  a3 factor of x^3
  //
  Init();
  fA[0] = a0;
  fA[1] = a1;
  fA[2] = a2;
  fA[3] = a3;
}
TPoly3::TPoly3(Double_t *a) {
  // Constructor with array
  Short_t k;
  Init();
  for (k=0;k<4;k++) fA[k] = a[k];
}
Short_t TPoly3::Degree() {
  // Truee degree of polynom
  const Double_t toosmall = 1.0e-300;
  Short_t n;
  Double_t a0,a1,a2,a3;
  a0 = TMath::Abs(fA[0]);
  a1 = TMath::Abs(fA[1]);
  a2 = TMath::Abs(fA[2]);
  a3 = TMath::Abs(fA[3]);
  if (a3 < toosmall) {
    if (a2 < toosmall) {
      if (a1 < toosmall) n = 0;
      else               n = 1;
    }
    else n = 2;
  }
  else n = 3;
  return n;
}
void TPoly3::Extrema() {
  // Finds extrema and inflection point
  const Double_t zero = 0.0;
  Double_t a,bp,c,D;
  Double_t x1,x2,y1,y2;
  fMin    = kFALSE;
  fMax    = kFALSE;
  fInfl   = kFALSE;
  switch (fN) {
    case 2:
      x1 = -fA[1]/(2*fA[2]);
      y1 = fA[0] + fA[1]*x1 + fA[2]*x1*x1;
      if (fA[2]>=zero) {
        fMin  = kTRUE;
        fXMin = x1;
        fYMin = y1;
      }
      else {
        fMax  = kTRUE;
        fXMax = x1;
        fYMax = y1;
      }
      break;
    case 3:
      a  = 3*fA[3];
      bp = fA[2];
      c  = fA[1];
      D  = bp*bp - a*c;
      if (D>=zero) {
        fMin = kTRUE;
        fMax = kTRUE;
        D    = TMath::Sqrt(D);
        x1   = (-bp + D)/a;
        x2   = (-bp - D)/a;
        y1   = Y(x1);
        y2   = Y(x2);
        if (y1<=y2) {
          fXMin = x1;
          fYMin = y1;
          fXMax = x2;
          fYMax = y2;
        }
        else {
          fXMin = x2;
          fYMin = y2;
          fXMax = x1;
          fYMax = y1;
        }
      }
      else {
        D = TMath::Sqrt(-D);
        fXMin    = -bp/a;
        fXMax    = D/a;
      }
      fInfl  = kTRUE;
      fXInfl = -fA[2]/(3*fA[3]);
      fYInfl = Y(fXInfl);
      break;
    default:
      break;
  }
}
void TPoly3::Init() {
// Default values for variables before solution
  const Double_t zero = 0.0;
  fN     = -1;
  fComp  = kFALSE;
  fY0    = zero;
  fX1    = zero;
  fX2    = zero;
  fX3    = zero;
  fMin   = kFALSE;
  fXMin  = zero;
  fYMin  = zero;
  fMax   = kFALSE;
  fXMax  = zero;
  fYMax  = zero;
  fInfl  = kFALSE;
  fXInfl = zero;
  fYInfl = zero;
}
Double_t TPoly3::Integral(Double_t x1,Double_t x2) {
// Finds the integral between x1 and x2
  Double_t b0,b1,b2,b3,Y1,Y2;
  Double_t u2,u3,u4;
  b0 = fA[0];
  b1 = fA[1]/2.0;
  b2 = fA[2]/3.0;
  b3 = fA[3]/4.0;
  u2 = x1*x1;
  u3 = x1*u2;
  u4 = u2*u2;
  Y1 = b0*x1 + b1*u2 + b2*u3 + b3*u4;
  u2 = x2*x2;
  u3 = x2*u2;
  u4 = u2*u2;
  Y2 = b0*x2 + b1*u2 + b2*u3 + b3*u4;
  return Y2-Y1;
}
void TPoly3::Set(Double_t a0,Double_t a1,Double_t a2,Double_t a3) {
// Set a different polynom and leave it as unsolved
  //  a0 constant term
  //  a1 factor of x
  //  a2 factor of x^2
  //  a3 factor of x^3
  //
  Init();
  fA[0] = a0;
  fA[1] = a1;
  fA[2] = a2;
  fA[3] = a3;
}
void TPoly3::Set(Double_t *a) {
// Set a different polynom and leave it as unsolved
  //  a0 constant term
  //  a1 factor of x
  //  a2 factor of x^2
  //  a3 factor of x^3
  //
  Short_t k;
  Init();
  for (k=0;k<4;k++) fA[k] = a[k];
}
Short_t TPoly3::Solution(Double_t &y0,Double_t &x1,Double_t &x2,Double_t &x3,Bool_t &comp) {
  // Solution x1,x2,x3 of y0 = fA[0] + fA[1]*x + fA[2]*x^2 + fA[3]*x^3
  y0   = fY0;
  x1   = fX1;
  x2   = fX2;
  x3   = fX3;
  comp = fComp;
  return fN;
}
void TPoly3::Solve(Double_t y0) {
  // Solutions of the equation y0 = fA[0] + fA[1]*x + fA[2]*x^2 + fA[3]*x^3
  // For y0==0, roots of polynom.
  // Solution inside variables of the class, but not returned in arguments
  Double_t x1,x2,x3;
  Bool_t comp;
  Short_t n;
  n = Solve(x1,x2,x3,comp,y0);
}
Short_t TPoly3::Solve(Double_t &x1,Double_t &x2,Double_t &x3,Bool_t &comp,Double_t y0) {
  // Solutions of the equation y0 = fA[0] + fA[1]*x + fA[2]*x^2 + fA[3]*x^3
  // For y0==0, roots of polynom.
  // Solution inside variables of the class and returned in arguments
  fY0 = y0;
  fN  = Degree();
  Extrema();
  switch (fN) {
    case 0:
      fN = -1;
      break;
    case 1:
      fX1   = (y0-fA[0])/fA[1];
      fComp = kFALSE;
      break;
    case 2:
      Solve2(y0);
      break;
    case 3:
      Solve3(y0);
      break;
    default:
      fN = -1;
      break;
  }
  x1   = fX1;
  x2   = fX2;
  x3   = fX3;
  comp = fComp;
  return fN;
}
void TPoly3::Solve2(Double_t y0) {
  // Case fA[3] == 0. Polynom of 2nd degree
  const Double_t zero = 0.0;
  Double_t d,b;
  d = fA[1]*fA[1] - 4*fA[2]*(fA[0] - y0);
  b = 2*fA[2];
  if (d>=zero) {
    fComp = kFALSE;
    d     = TMath::Sqrt(d);
    fX1   = (-fA[1] + d)/b;
    fX2   = (-fA[1] - d)/b;
  }
  else {
    fComp = kTRUE;
    d     = TMath::Sqrt(-d);
    fX1   = -fA[1]/b;
    fX2   = d/b;
  }
}
void TPoly3::Solve3(Double_t y0) {
  // Polynom truly of 3rd order
  const Double_t zero  = 0.0;
  const Double_t z1    = 1.0;
  const Double_t z2    = 2.0;
  const Double_t z3    = 3.0;
  const Double_t z27   = 27.0;
  Double_t a,b,c;
  Double_t a3,d,qs2,ps3,rps3,as3,us,vs,q2,p3;
  Double_t pi,phi,cphi,phis3,pis3,phi1,phi2,cphi0,cphi1,cphi2;
  Double_t p,q,u,v;
  a3   = fA[3];
  a    = fA[2]/a3;
  b    = fA[1]/a3;
  c    = (fA[0] - y0)/a3;
  p    = b - (a*a)/z3;
  q    = c - (a*b)/z3 + (z2*a*a*a)/z27;
  as3  = a/z3;
  qs2  = q/z2;
  ps3  = p/z3;
  q2   = qs2*qs2;
  p3   = ps3*ps3*ps3;
  d    = q2 + p3;
  if (d>=zero) {
    fComp = kTRUE;
    d     = TMath::Sqrt(d);
    u     = -qs2 + d;
    v     = -qs2 - d;
    us    = TMath::Sign(z1,u);
    vs    = TMath::Sign(z1,v);
    u     = TMath::Abs(u);
    v     = TMath::Abs(v);
    u     = us*TMath::Exp(TMath::Log(u)/z3);
    v     = vs*TMath::Exp(TMath::Log(v)/z3);
    fX1   = u + v;
    fX2   = -(u+v)/z2;
    fX3   =  ((u-v)*TMath::Sqrt(z3))/z2;
    fX1  -= as3;
    fX2  -= as3;
  }
  else {
    fComp = kFALSE;
    rps3  = TMath::Sqrt(-ps3);
    pi    = TMath::Pi();
    pis3  = pi/z3;
    cphi  = -qs2/TMath::Sqrt(-p3);
    phi   = TMath::ACos(cphi);
    phis3 = phi/z3;
    phi1  = pis3 + phis3;
    phi2  = pis3 - phis3;
    cphi0 = TMath::Cos(phis3);
    cphi1 = TMath::Cos(phi1);
    cphi2 = TMath::Cos(phi2);
    fX1   = z2*rps3*cphi0;
    fX2   = -z2*rps3*cphi1;
    fX3   = -z2*rps3*cphi2;
    fX1  -= as3;
    fX2  -= as3;
    fX3  -= as3;
  }
}
Bool_t TPoly3::SolveLeft(Double_t &x,Double_t y0,Bool_t interval,Double_t xmin,Double_t xmax) {
// Finds the smallest x for which y0 = fA[0] + fA[1]*x + fA[2]*x^2 + fA[3]*x^3
// In case interval is true, true is returned only if xmin <= x <= xmax.
// This method is useful for instance if the polynom is a rising signal, and one is interested
//in knowing where the signal reaches 10% or 90% of the pulse.
  const Double_t zero = 0.0;
  Bool_t ok  = kTRUE;
  Bool_t ok1 = kTRUE;
  Bool_t ok2 = kTRUE;
  Bool_t ok3 = kTRUE;
  Short_t N;
  Double_t x1 = zero;
  Double_t x2 = zero;
  Double_t x3 = zero;
  x = zero;
  Solve(y0);
  N = 0;
  switch (fN) {
    case 1:
      if (interval) ok1 = ((fX1>=xmin) && (fX1<=xmax));
      if (ok1) {
        N++;
        x1 = fX1;
      }
      else ok = kFALSE;
      break;
    case 2:
      if (!fComp) {
        if (interval) {
          ok1 = ((fX1>=xmin) && (fX1<=xmax));
          ok2 = ((fX2>=xmin) && (fX2<=xmax));
        }
        if (ok1) {
          x1 = fX1;
          N++;
          if (ok2) {
            x2 = fX2;
            N++;
          }
        }
        else {
          if (ok2) {
            x1 = fX2;
            N++;
          }
        }
      }//end if (!fComp)
      else ok = kFALSE;
      break;
    case 3:
      if (fComp) {
        if (interval) ok1 = ((fX1>=xmin) && (fX1<=xmax));
        if (ok1) {
          x1 = fX1;
          N++;
        }
      }
      else {
        if (interval) {
          ok1 = ((fX1>=xmin) && (fX1<=xmax));
          ok2 = ((fX2>=xmin) && (fX2<=xmax));
          ok3 = ((fX3>=xmin) && (fX3<=xmax));
        }
        if (ok1) {
          x1 = fX1;
          N++;
          if (ok2) {
            x2 = fX2;
            N++;
            if (ok3) {
// yyy
              x3 = fX3;
              N++;
            }
          }//end if ok2 && ok1
          else {
            if (ok3) {
// yny
              x2 = fX3;
              N++;
            }//end if (ok3)
          }//end else if (ok2)
        }
        else {
          if (ok2) {
            x1 = fX2;
            N++;
            if (ok3) {
// nyy
              x2 = fX3;
              N++;
            }
          }//end if (ok2)
          else {
            if (ok3) {
//nny
              x1 = fX3;
              N++;
            }//end if (ok3)
          }//end else if (ok2)
        }//end else if (ok1)
      }//end else if (fComp)
      break;
    default:
      ok = kFALSE;
      break;
  }
  if (ok) {
    switch (N) {
      case 0:
        ok = kFALSE;
        break;
      case 1:
        x = x1;
        break;
      case 2:
        x = TMath::Min(x1,x2);
        break;
      case 3:
        x = TMath::Min(x1,x2);
        x = TMath::Min(x,x3);
        break;
      default:
        ok = kFALSE;
        break;
    }
  }
  if (ok && interval) ok = ((x>=xmin) && (x<=xmax));
  return ok;
}
Bool_t TPoly3::SolveRight(Double_t &x,Double_t y0,Bool_t interval,Double_t xmin,Double_t xmax) {
// Finds the bigest x for which y0 = fA[0] + fA[1]*x + fA[2]*x^2 + fA[3]*x^3
// In case interval is true, true is returned only if xmin <= x <= xmax.
// This method is useful for instance if the polynom is a fallinf signal, and one is interested
//in knowing where the signal reaches 10% or 90% of the pulse.
  const Double_t zero = 0.0;
  Bool_t ok  = kTRUE;
  Bool_t ok1 = kTRUE;
  Bool_t ok2 = kTRUE;
  Bool_t ok3 = kTRUE;
  Short_t N;
  Double_t x1 = zero;
  Double_t x2 = zero;
  Double_t x3 = zero;
  x = zero;
  Solve(y0);
  N = 0;
  switch (fN) {
    case 1:
      if (interval) ok1 = ((fX1>=xmin) && (fX1<=xmax));
      if (ok1) {
        N++;
        x1 = fX1;
      }
      else ok = kFALSE;
      break;
    case 2:
      if (!fComp) {
        if (interval) {
          ok1 = ((fX1>=xmin) && (fX1<=xmax));
          ok2 = ((fX2>=xmin) && (fX2<=xmax));
        }
        if (ok1) {
          x1 = fX1;
          N++;
          if (ok2) {
            x2 = fX2;
            N++;
          }
        }
        else {
          if (ok2) {
            x1 = fX2;
            N++;
          }
        }
      }//end if (!fComp)
      else ok = kFALSE;
      break;
    case 3:
      if (fComp) {
        if (interval) ok1 = ((fX1>=xmin) && (fX1<=xmax));
        if (ok1) {
          x1 = fX1;
          N++;
        }
      }
      else {
        if (interval) {
          ok1 = ((fX1>=xmin) && (fX1<=xmax));
          ok2 = ((fX2>=xmin) && (fX2<=xmax));
          ok3 = ((fX3>=xmin) && (fX3<=xmax));
        }
        if (ok1) {
          x1 = fX1;
          N++;
          if (ok2) {
            x2 = fX2;
            N++;
            if (ok3) {
// yyy
              x3 = fX3;
              N++;
            }
          }//end if ok2 && ok1
          else {
            if (ok3) {
// yny
              x2 = fX3;
              N++;
            }//end if (ok3)
          }//end else if (ok2)
        }
        else {
          if (ok2) {
            x1 = fX2;
            N++;
            if (ok3) {
// nyy
              x2 = fX3;
              N++;
            }
          }//end if (ok2)
          else {
            if (ok3) {
//nny
              x1 = fX3;
              N++;
            }//end if (ok3)
          }//end else if (ok2)
        }//end else if (ok1)
      }//end else if (fComp)
      break;
    default:
      ok = kFALSE;
      break;
  }
  if (ok) {
    switch (N) {
      case 0:
        ok = kFALSE;
        break;
      case 1:
        x = x1;
        break;
      case 2:
        x = TMath::Max(x1,x2);
        break;
      case 3:
        x = TMath::Max(x1,x2);
        x = TMath::Max(x,x3);
        break;
      default:
        ok = kFALSE;
        break;
    }
  }
  if (ok && interval) ok = ((x>=xmin) && (x<=xmax));
  return ok;
}
Double_t TPoly3::Y(Double_t x) {
  // Value of polynom at x
  Double_t x2,x3,y;
  x2 = x*x;
  x3 = x2*x;
  y  = fA[0] + fA[1]*x + fA[2]*x2 + fA[3]*x3;
  return y;
}

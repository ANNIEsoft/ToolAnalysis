//----------Author's Name:F-X Gentit
//----------Copyright:Those valid for ROOT sofware
//----------Modified:15/1/2004
#include "Riostream.h"
#include "TBandedLE.h"

//ClassImp(TBandedLE)
//______________________________________________________________________________
//
//  This is a C++ translation of the F406 DBEQN program of the CERN packlib
//library, originally written by G.A.Erskine.
//  BandedLE stands for Banded Linear Equations. Notice that in this C++
//versions, all arrays have 0 as first index, not 1 as in the Fortran version.
//  Class TBandedLE solve a system of N simultaneous linear equations with K
//right-hand sides, the coefficient matrix being a band matrix with bandwidth
//2*M+1:
//
//  Sum A(i,j)*X(j,k) = B(i,k)    [0<=i<N], 0<=j<N, 0<=k<K]
//
//  Only those coefficients A(i,j) for which |i-j| <= M need be supplied.
//On entry, A must contain the packed form of the coefficient matrix as
//described below, and B must contain the matrix of right-hand sides
//B(i,k). Then, provided the coefficient matrix is non-singular, ifail, the
//value returned by the method Solve(), will be set to 0 and the solution X
//will be found. Contrary to the Fortran version of this program, the
//matrices A and B remain intact, EXCEPT in the case you provide a Matrix A
//not in compact form. If you do so, matrix A will be compactified, and the
//supplementary lines and columns of A not participating to the problem will
//be lost. TBandedLE works on copies of A and B.
//The solution is to be found in matrix fX (or in vector fV in case K==1).
//If the coefficient matrix is singular, ifail will be set to -1.
//  The storage convention for A is that it must contain, on entry, those
//coefficients A(i,j) for which |i-j| <= M, stored "left-justified" as an
//array of N rows and at most 2M+1 columns. For example, if N=4 and M=1, the
// coefficient matrix:
//
// | a00  a01    0   0 |                       | a00  a01    X |
// | a10  a11  a12   0 |                       | a10  a11  a12 |
// |   0  a21  a22 a23 |  is stored as         | a21  a22  a23 |
// |   0    0  a32 a33 |                       | a32  a33    X |
//
//where X denotes elements whose value need not to be set.
//
// The possibility exists to give matrix A NOT in compact form if the
//user prefers. In that case, put the last parameter of the constructor
//to false.
//
//  If alpha(i,j) is a function subprogram or statement function which
//computes A(i,j), the following statements will set A in compact form:
//
//  Int_t i,j,js,jmin,jmax;
//  for (i=0;i<N;i++) {
//    js = 0;
//    jmin = TMath::Max(i-fM,0);
//    jmax = TMath::Min(i+fM,fN-1);
//    for (j=jmin;j<=jmax;j++) {
//      A(i,js) = alpha(i,j);
//      js++;
//    }
//  }
//
//  In case fK==1, one surely prefers to have the solution in the form of
//a vector. So the solution is copied into vector fV.
//
//  Method used: Gaussian elimination with row interchanges. The storage
//organization is as described in the reference.
//  Error handling: If the integer arguments do not satisfy the conditions:
//
//    N>=1 | M>0 | M<N | K>0
//    nrowsA >= N | ncolsA>= 2*M+1 | nrowsB >= N | ncolsB >=K
//
// a message is printed and the problem rejected: method Solve will return
//ifail = -2
//
//References: J.H. Wilkinson and C. Reinsch (eds.), Handbook for automatic
//computation, Vol.2: Linear algebra (Springer-Verlag, New York 1971) 54.
//
//Example of use from CINT code:
//
//{
//  const Double_t zero = 0.0;
//  const Int_t N = 8;
//  const Int_t M = 2;
//  const Int_t K = 1;
//  Double_t eps;
//  Int_t mband = 2*M+1;
//  Int_t i,j,k,ifail;
//  TMatrixD A(N,N);
//  TMatrixD B(N,K);
//  for (i=0;i<N;i++) B(i,0) = i+1;
//  k=0;
//  for (i=0;i<N;i++) {
//    for (j=0;j<N;j++) {
//      if (TMath::Abs(i-j)>M) A(i,j) = zero;
//      else {
//        k++;
//        A(i,j) = k;
//      }
//    }
//  }
//  A.Print();
//  std::cout << std::endl;
//  TBandedLE ble(A,B,M,kFALSE); //A NOT given in compact form !
//  A.Print();
//  ifail = ble.Solve();
//  std::cout << "ifail : " << ifail << std::endl;
//  for (i=0;i<N;i++) std::cout << "fV(" << i << ") : " << ble.fV(i) << std::endl;
//  eps = ble.Verify();
//  std::cout << "Verify : " << eps << std::endl;
//}
//
TBandedLE::TBandedLE() {
// Default constructor, called when reading this class from a file. In that
//case, the class is owner of fA and fB.
  fOwnA = kTRUE;
  fOwnB = kTRUE;
  Init();
}
TBandedLE::TBandedLE(Int_t N,Int_t M,Int_t K,TMatrixD &A,TMatrixD &B,Bool_t cpt) {
// Constructor in the most general case.
//
// N    : Number of equations. Notice that in case this constructor is used,
//         the number of lines of matrix A or B MAY be bigger then N. If it is
//         the case, the supplementary lines are simply not part of the
//         problem.
// M    : Band parameter. Band size is 2*M + 1.
// K    : Number of right-hand sides in matrix B.  Notice that in case this
//         constructor is used, the number of columns of matrix B MAY be
//         bigger then K. If it is the case, the supplementary columns are
//         simply not part of the problem.
// A    : Band matrix in compressed format [default] or not.
//         compressed,   the used portion of A has dimensions N*2M+1 if N>=2M+1
//                       N*N otherwise. A will remain untouched.
//         uncompressed, the used portion of A has dimensions N*N. Be careful
//                       that in case you provide A in uncompressed form,
//                       matrix A will be compressed, and the
//                       supplementary lines and columns of A not participating
//                       to the problem will be lost.
// B    : Matrix of right-hand size. The used portion of B has dimensions N*K
// cpt  : true [default]: A matrix is provided in compact form
//      : false         : A matrix is NOT in compact form.
//
// Notice that with this constructor, TBandedLE is not owner of A, B. They must
//be reserved, filled and deleted by the user.
//
  Int_t ncolsA,nrowsA,ncolsB,nrowsB;
  Int_t mband;
  fOwnA   = kFALSE;
  fOwnB   = kFALSE;
  fF      = 0;
  fN      = N;
  fM      = M;
  fK      = K;
  fA      = &A;
  fB      = &B;
  mband   = 2*fM+1;
  if (mband>fN) mband = fN;
  if (!cpt) Compactify();
  ncolsA  = fA->GetNcols();
  nrowsA  = fA->GetNrows();
  ncolsB  = fB->GetNcols();
  nrowsB  = fB->GetNrows();
  if (fN<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | unacceptable value for N     : " << fN     << std::endl;
  }
  if ((fM<=0) || (fM>=fN)) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | unacceptable value for M     : " << fM     << std::endl;
  }
  if (fK<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | unacceptable value for K     : " << fK     << std::endl;
  }
  if (nrowsA<fN) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | A : nb. of rows too small    : " << nrowsA << std::endl;
  }
  if (ncolsA<mband) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | A : nb. of columns too small : " << ncolsA << std::endl;
  }
  if (nrowsB<fN) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | B : nb. of rows too small    : " << nrowsB << std::endl;
  }
  if (ncolsB<fK) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | B : nb. of columns too small : " << ncolsB << std::endl;
  }
}
TBandedLE::TBandedLE(TMatrixD &A,TMatrixD &B,Int_t M,Bool_t cpt) {
// In this constructor, it is assumed that
//
// - the number of lines   of A and B is equal to the number of equations N
// - if A is compressed and N>=2M+1 then the number of columns of A is 2M+1.
// - if A is compressed and N <2M+1 then the number of columns of A is N.
// - if A is not compressed,        then the number of columns of A is N.
// - the number of columns of B is K
//
//so that it is no more necessary to give N,K !
//
// A    : Band matrix in compressed format [default] or not.
//         compressed,   A has dimensions N*2M+1 if N>=2M+1
//                       N*N otherwise. A will remain untouched.
//         uncompressed, A has dimensions N*N. Be careful
//                       that in case you provide A in uncompressed form,
//                       matrix A will be compressed and by that does not
//                       remain untouched
// B    : Matrix of right-hand size. The used portion of B has dimensions N*K
// M    : Band parameter. Band size is 2*M + 1. You have to give M in any case.
// cpt  : true [default]: A matrix is provided in compact form
//      : false         : A matrix is NOT in compact form.
//
// Notice that with this constructor, TBandedLE is not owner of fA, fB. They must
//be reserved, filled and deleted by the user.
//
  Int_t ncolsA,nrowsA,ncolsB,nrowsB;
  fOwnA   = kFALSE;
  fOwnB   = kFALSE;
  fF      = 0;
  fA      = &A;
  fB      = &B;
  nrowsA  = fA->GetNrows();
  fN      = nrowsA;
  fM      = M;
  if (!cpt) Compactify();
  ncolsA  = fA->GetNcols();
  ncolsB  = fB->GetNcols();
  nrowsB  = fB->GetNrows();
  fK      = ncolsB;
//Number of lines in A must be >= 1
  if (nrowsA<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of lines in A must be >=1" << std::endl;
  }
//Number of lines in A and B must be equal
  if (nrowsA != nrowsB) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of lines in A and B must be the same" << std::endl;
  }
//Number of columns in A must be >= 1
  if (ncolsA<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of columnss in A must be >=1" << std::endl;
  }
//Number of lines in B must be >= 1
  if (nrowsB<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of lines in B must be >=1" << std::endl;
  }
//Number of columns in B must be >= 1
  if (ncolsB<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of columns in B must be >=1" << std::endl;
  }
}
TBandedLE::TBandedLE(TMatrixD &A,Int_t M,Bool_t cpt) {
// In this constructor, it is assumed that
//
// - the number of lines   of A is equal to the number of equations N
// - if A is compressed and N>=2M+1 then the number of columns of A is 2M+1.
// - if A is compressed and N <2M+1 then the number of columns of A is N.
// - if A is not compressed,        then the number of columns of A is N.
// - the matrix B is the unit matrix of dimension N*N, not provided by
//    the user, so that the solution fX is the inverse of A (not in
//    compact form)
//
//so that is it no more necessary to give N,K,&B !
//
// A    : Band matrix in compressed format [default] or not.
//         compressed,   A has dimensions N*2M+1 if N>=2M+1
//                       N*N otherwise. A will remain untouched.
//         uncompressed, A has dimensions N*N. Be careful
//                       that in case you provide A in uncompressed form,
//                       matrix A will be compressed and by that does not
//                       remain untouched
// M    : Band parameter. Band size is 2*M + 1.
// cpt  : true [default]: A matrix is provided in compact form
//      : false         : A matrix is NOT in compact form.
//
//
// Notice that with this constructor, TBandedLE is not owner of fA, but is
//owner of fB.
//
  Int_t ncolsA,nrowsA,rowlwbA,collwbA;
  Init();
  fOwnA   = kFALSE;
  fOwnB   = kTRUE;
  fF      = 0;
  fA      = &A;
  nrowsA  = fA->GetNrows();
  fN      = nrowsA;
  fM      = M;
  if (!cpt) Compactify();
  ncolsA  = fA->GetNcols();
  rowlwbA = fA->GetRowLwb();
  collwbA = fA->GetColLwb();
//Number of lines in A must be >= 1
  if (nrowsA<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of lines in A must be >=1" << std::endl;
  }
//Number of columns in A must be >= 1
  if (ncolsA<1) {
    fF = -2;
    std::cout << "TBandedLE::TBandedLE | Nb. of columnss in A must be >=1" << std::endl;
  }
  if (!fF) {
    fB    = new TMatrixD(fN,fN);
    fB->UnitMatrix();
    fK    = fN;
  }
}
TBandedLE::~TBandedLE() {
//Destructor
  if (fOwnA) delete fA;
  if (fOwnB) delete fB;
}
void TBandedLE::Compactify() {
// Put matrix fA in compact form
  Int_t i,j,is,js,jmin,jmax;
  Int_t rowlwbA,collwbA,rowupbA,colupbA;
  rowlwbA = fA->GetRowLwb();
  collwbA = fA->GetColLwb();
  rowupbA = fN + rowlwbA - 1;
  colupbA = 2*fM + collwbA;
  fX.ResizeTo(fN,fN);
  for (i=0;i<fN;i++) {
    is = i + rowlwbA;
    for (j=0;j<fN;j++) {
      js = j + collwbA;
      fX(i,j) = (*fA)(is,js);
    }//end for (j=0;j<fN;j++)
  }//end for (i=0;i<fN;i++)
  fA->ResizeTo(rowlwbA,rowupbA,collwbA,colupbA);
  fA->Zero();
  for (i=0;i<fN;i++) {
    js = 0;
    jmin = TMath::Max(i-fM,0);
    jmax = TMath::Min(i+fM,fN-1);
    for (j=jmin;j<=jmax;j++) {
      (*fA)(i,js) = fX(i,j);
      js++;
    }
  }
}
void TBandedLE::Init() {
//Put pointers to 0
  fA = 0;
  fB = 0;
}
Int_t TBandedLE::Solve() {
//
//  Solve the problem. The results is in fX. If fK==1, the result is also in fV.
//
//  If returned value =  0 : everything ok
//  If returned value = -1 : the matrix was singular
//  If returned value = -2 : the provided arguments of the constructor
//                           were bad
//
  const Double_t zero = 0.0;
  const Double_t epsA = 1.0e-24;
  if (fF>=0) {
    Int_t i;       //Fortran  I        |  i      = I-1
    Int_t j;       //Fortran  J        |  j      = J-1
    Int_t ll;      //Fortran  L        |  ll     = L-1
    Int_t jrhs;    //Fortran  JRHS     |  jrhs   = JRHS   - 1
    Int_t icomp;   //Fortran  ICOMP    |  icomp  = ICOMP  - 1
    Int_t imax;    //Fortran  IMAX     |  imax   = IMAX   - 1
    Int_t jmax;    //Fortran  JMAX     |  jmax   = JMAX   - 1
    Int_t jmin;    //Fortran  JMIN     |  jmin   = JMIN   - 1
    Int_t nminus;  //Fortran  NMINUS   |  nminus = NMINUS - 1
    Int_t lplus;   //Fortran  LPLUS    |  lplus  = LPLUS  - 1
    Int_t lcut;    //Fortran  LCUT     |  lcut   = LCUT   - 1
    Int_t ipiv;    //Fortran  IPIV     |  ipiv   = IPIV   - 1
    Int_t mband;   //Fortran  MBAND    |  mband  = MBAND
    Double_t t;    //Fortran  T        |  t      = T           | an element of matrix fA
    Double_t p;    //Fortran  P        |  p      = P           | pivot of matrix fA
    Double_t tmax; //Fortran  TMAX     |  tmax   = TMAX        | max element of matrix fA
    Double_t eps;  //smaller than eps is considered 0
    Double_t biggest = zero; //will contain biggest element of fA in abs. value
//
// Copy matrix fA into cAc and fB into fX. cAc will be destroyed and fX will
//contain the solution.
    Int_t is,js;
    Int_t rowlwbA,collwbA,rowlwbB,collwbB;
    mband   = 2*fM + 1;
    if (mband>fN) mband = fN;
    TMatrixD cAc(fN,mband);
    rowlwbA = fA->GetRowLwb();
    collwbA = fA->GetColLwb();
    rowlwbB = fB->GetRowLwb();
    collwbB = fB->GetColLwb();
    fX.ResizeTo(fN,fK);
    fV.ResizeTo(fN);
    fV.Zero();
    for (i=0;i<fN;i++) {
      is = i + rowlwbA;
      for (j=0;j<mband;j++) {
        js = j + collwbA;
        cAc(i,j) = (*fA)(is,js);
      }//end for (j=0;j<mband;j++)
    }//end for (i=0;i<fN;i++)
    for (i=0;i<fN;i++) {
      is = i + rowlwbB;
      for (j=0;j<fK;j++) {
        js = j + collwbB;
        fX(i,j) = (*fB)(is,js);
      }//end for (j=0;j<fK;j++)
    }//end for (i=0;i<fN;i++)
//
    fF = -1;
    for (i=0;i<fN;i++)
      for (j=0;j<mband;j++) {
        t = TMath::Abs(cAc(i,j));
        if (t>biggest) biggest = t;
      }
    eps = epsA*biggest;
    mband   = 2*fM + 1;
    if (fM) {
//
// Set 0 in upper-right triangle of fA
//
      jmax = TMath::Min(mband,fN) - 1;
      imax = jmax - fM - 1;
      jmin = fM;
      if (imax >= 0) {
        for (i=0;i<=imax;i++) {
          jmin += 1;
          for (j=jmin;j<=jmax;j++) cAc(i,j) = zero;
        }//end for (i=0;i<imax;i++)
      }//end if (imax >= 1)
//
// Gaussian elimination to reduce matrix to upper triangular form.
// (within this section, imax=MIN(ll+fM,fN), jmax=MIN(fN-ll+1,mband))
//
      imax   = fM - 1;         // -1 added because acts on loop index
      lcut   = fN - mband - 1; // -1 added because acts on loop index
      nminus = fN - 2;         // -1 added because acts on loop index
      for (ll=0;ll<=nminus;ll++) {
        lplus = ll + 1;
        if (imax<(fN-1)) imax++;
// Pivot search.  Set tmax to abs(pivot)
        tmax = TMath::Abs(cAc(ll,0));
        ipiv = ll;
        for (i=lplus;i<=imax;i++) {
          t = TMath::Abs(cAc(i,0));
          if (t>tmax) {
            tmax = t;
            ipiv = i;
          }//end if (t>tmax)
        }//end for (i=lplus;i<imax;i++)  21
//
// Interchange rows ll and ipiv
//
        if (ipiv != ll) {
          for (j=0;j<=jmax;j++) {
            t = cAc(ipiv,j);
            cAc(ipiv,j) = cAc(ll,j);
            cAc(ll,j) = t;
          }//end for (j=0;j<jmax;j++)  22
          for (jrhs=0;jrhs<fK;jrhs++) {
            t = fX(ipiv,jrhs);
            fX(ipiv,jrhs) = fX(ll,jrhs);
            fX(ll,jrhs) = t;
          }//end for (jrhs=0;jrhs<fK;jrhs++)  23
        }//end if (ipiv != ll)
//
// Eliminate
//
        p = cAc(ll,0);
        if (TMath::Abs(p)<eps) return fF;
        for (i=lplus;i<=imax;i++) {
          t = cAc(i,0)/p;
          for (j=1;j<=jmax;j++) cAc(i,j-1) = cAc(i,j) - t*cAc(ll,j);
          cAc(i,jmax) = zero;
          for(jrhs=0;jrhs<fK;jrhs++) fX(i,jrhs) -= t*fX(ll,jrhs);
        }//end for (i=lplus;i<imax;i++)  27
        if (ll>lcut) jmax--;
      }// end for (ll=0;ll<nminus;ll++)  28
      if (TMath::Abs(cAc(fN-1,0))<eps) return fF;
//
// Back-substitution
//
      for(jrhs=0;jrhs<fK;jrhs++) {
        jmax = -1;
        i    = fN - 1;
        for (icomp=0;icomp<fN;icomp++) {
          if ((jmax+1)<mband) jmax++;
          ll = i;
          t = fX(i,jrhs);
          if (jmax>=1) {
            for (j=1;j<=jmax;j++) {
              ll++;
              t -= cAc(i,j)*fX(ll,jrhs);
            }//end for (j=1;j<jmax;j++)  31
          }//end if (jmax>=2)  32
          fX(i,jrhs) = t/cAc(i,0);
          i--;
        }//end for (icomp=0;icomp<fN;icomp++)  33
      }//end for(jrhs=0;jrhs<fK;jrhs++)  34
      fF = 0;
    }//end if (fM)
    else {
      for (jrhs=0;jrhs<fK;jrhs++) {
        for (i=0;i<fN;i++) {
          t = cAc(i,0);
          if (TMath::Abs(t)<eps) return fF;
          fX(i,jrhs) /= t;
        }//end for (i=0;i<fN;i++)  41
      }//end for (jrhs=0;jrhs<fK;jrhs++)  42
      fF = 0;
    }//end else if (fM)
    if ((!fF) && (fK==1)) for (i=0;i<fN;i++) fV(i) = fX(i,0);
  }//end if (fF>=0)
  return fF;
}
Double_t TBandedLE::Verify() const {
// Verifies how close A*X is from B
  const Double_t zero = 0.0;
  Int_t i,j,k,isA,jsA,jsX,isB,ksB;
  Int_t rowlwbA,collwbA;
  Int_t rowlwbB,collwbB;
  Int_t mband;
  Double_t t;
  Double_t eps = zero;
  mband = 2*fM + 1;
  if (mband>fN) mband = fN;
  rowlwbA = fA->GetRowLwb();
  collwbA = fA->GetColLwb();
  rowlwbB = fB->GetRowLwb();
  collwbB = fB->GetColLwb();
  for (k=0;k<fK;k++) {
    ksB = k + collwbB;
    for (i=0;i<fN;i++) {
      isA = i + rowlwbA;
      isB = i + rowlwbB;
      t   = zero;
      for (j=0;j<mband;j++) {
        jsA = j + collwbA;
        if (i>fM) jsX = j + i - fM;
        else      jsX = j;
        if (jsX<fN) t  += (*fA)(isA,jsA)*fX(jsX,k);
      }//end for (j=0;j<fK;j++)
      eps += TMath::Abs(t-(*fB)(isB,ksB));
    }//end for (i=0;i<fN;i++)
  }//end for (k=0;k<fK;k++)
  return eps;
}

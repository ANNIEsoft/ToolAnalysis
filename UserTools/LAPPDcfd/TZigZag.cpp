//----------Author's Name:F-X Gentit
//----------Copyright:Those valid for ROOT software
//----------Modified:1/15/2004
#include "Riostream.h"
#include "TMath.h"
#include "TZigZag.h"

//ClassImp(TZigZag)
//______________________________________________________________________________
//
//  This class is intended to allow using a one-dimensional method of fitting
//for fits in 2 or 3 dimensions, in case of points equally spaced in x, y and z.
//To make this possible, it is necessary, in order to avoid discontinuities
//in the one dimensional fit, that point i+1 is always near point i, for all i.
//  Taking for instance a 2 dimensional structure of dimensions (5,3), if one
//numbers the points in the usual way:
//
//  Y
//  ^
//  |     11  12  13  14  15
//  |      6   7   8   9  10
//  |      1   2   3   4   5
//  |________________________> X
//
//   one sees that point 5 is very far from point 6 and that point 10 is very far
//from point 11. If one tries a 1D fit following this ordering, the 1D fit will be
//bad due to big jumps in value between points 5 and 6 and between points 10 and 11.
//On the other hand, if one numbers the points like this:
//
//  Y
//  ^
//  |     11  12  13  14  15
//  |     10   9   8   7   6
//  |      1   2   3   4   5
//  |________________________> X
//
//  in a "zigzag" way, points 5 is close to point 6, and 10 close to 11. It is
//exactly what TZigZag does, excepts that numbering of TZigZag begins with 0
//  The same trick is applied in 3D. Here is an example of the zigzag numbering
//with 3 planes in z:
//
//        1st plane in z
//  Y
//  ^
//  |     10  11  12  13  14
//  |      9   8   7   6   5
//  |      0   1   2   3   4
//  |________________________> X
//
//
//        2nd plane in z
//  Y
//  ^
//  |     19  18  17  16  15
//  |     20  21  22  23  24
//  |     29  28  27  26  25
//  |________________________> X
//
//
//        3rd plane in z
//  Y
//  ^
//  |     40  41  42  43  44
//  |     39  38  37  36  35
//  |     30  31  32  33  34
//  |________________________> X
//
//
TZigZag::TZigZag() {
// Default constructor.
  Init();
}
TZigZag::TZigZag(Int_t nx,Double_t xmin,Double_t xmax) {
// Constructor in case of 1 dimension. [no interest]
  Init();
  if (nx>0) {
    fDim  = 1;
    fNx   = nx;
    fXmin = xmin;
    fXmax = xmax;
  }
}
TZigZag::TZigZag(Int_t nx,Double_t xmin,Double_t xmax,
                 Int_t ny,Double_t ymin,Double_t ymax) {
// Constructor in case of 2 dimensions.
  Init();
  if (nx>0) {
    fDim = 1;
    fNx   = nx;
    fXmin = xmin;
    fXmax = xmax;
    if (ny>0) {
      fDim  = 2;
      fNy   = ny;
      fYmin = ymin;
      fYmax = ymax;
    }
  }
}
TZigZag::TZigZag(Int_t nx,Double_t xmin,Double_t xmax,
                 Int_t ny,Double_t ymin,Double_t ymax,
                 Int_t nz,Double_t zmin,Double_t zmax) {
// Constructor in case of 3 dimensions.
  Init();
  if (nx>0) {
    fDim = 1;
    fNx   = nx;
    fXmin = xmin;
    fXmax = xmax;
    if (ny>0) {
      fDim  = 2;
      fNy   = ny;
      fYmin = ymin;
      fYmax = ymax;
      if (nz>0) {
        fDim = 3;
        fNz   = nz;
        fZmin = zmin;
        fZmax = zmax;
      }
    }
  }
}
TZigZag::TZigZag(const TZigZag &zigzag) {
// Copy constructor
  fDim  = zigzag.fDim;     //dimension: 1,2 or 3!
  fNx   = zigzag.fNx;      //Nb. of points along x
  fNy   = zigzag.fNy;      //Nb. of points along y
  fNz   = zigzag.fNz;      //Nb. of points along z
  fXmin = zigzag.fXmin;    //low bound in x
  fYmin = zigzag.fYmin;    //low bound in y
  fZmin = zigzag.fZmin;    //low bound in z
  fXmax = zigzag.fXmax;    //up  bound in x
  fYmax = zigzag.fYmax;    //up  bound in y
  fZmax = zigzag.fZmax;    //up  bound in z
}
void TZigZag::Init() {
//Initialize to undefined
  const Double_t zero = 0.0;
  fDim  = 0;
  fNx   = 1;
  fNy   = 1;
  fNz   = 1;
  fXmin = zero;
  fYmin = zero;
  fZmin = zero;
  fXmax = zero;
  fYmax = zero;
  fZmax = zero;
}
Bool_t TZigZag::IsInside(Double_t xi,Double_t yi,Double_t x, Double_t y,
  Double_t xc,Double_t yc,Double_t xl,Double_t xr,Double_t yl,Double_t yr) const {
// Two-dimensional case. Test whether point (xi,yi) is between (xc,yc) and (x,y)
//and whether (xi,yi) is inside the square (xl,xr,yl,yr)
  const Double_t zero = 0.0;
  Double_t w;
  Bool_t ok = kTRUE;
  w = (x-xi)*(xc-xi);
  if (w>zero) ok = kFALSE;
  if (ok) {
    w = (y-yi)*(yc-yi);
    if (w>zero) ok = kFALSE;
    if (ok) {
      if (xi<xl) ok = kFALSE;
      if (xi>xr) ok = kFALSE;
      if (yi<yl) ok = kFALSE;
      if (yi>yr) ok = kFALSE;
    }
  }
  return ok;
}
Bool_t TZigZag::IsInside(Double_t xi,Double_t yi,Double_t zi,Double_t x, Double_t y,Double_t z,
                         Double_t xc,Double_t yc,Double_t zc,Double_t xl,Double_t xr,
                         Double_t yl,Double_t yr,Double_t zl,Double_t zr) const {
// 3-dimensional case. Test whether point (xi,yi,zi) is between (xc,yc,zc) and (x,y,z)
//and whether (xi,yi,zi) is inside the cube (xl,xr,yl,yr,zl,zr)
  const Double_t zero = 0.0;
  Double_t w;
  Bool_t ok = kTRUE;
  w = (x-xi)*(xc-xi);
  if (w>zero) ok = kFALSE;
  if (ok) {
    w = (y-yi)*(yc-yi);
    if (w>zero) ok = kFALSE;
    if (ok) {
      w = (z-zi)*(zc-zi);
      if (w>zero) ok = kFALSE;
      if (ok) {
        if (xi<xl) ok = kFALSE;
        if (xi>xr) ok = kFALSE;
        if (yi<yl) ok = kFALSE;
        if (yi>yr) ok = kFALSE;
        if (zi<zl) ok = kFALSE;
        if (zi>zr) ok = kFALSE;
      }
    }
  }
  return ok;
}
Bool_t TZigZag::NearestPoints(Double_t x, TArrayI &I, TArrayD &W) const {
// One-dimensional case. Gives the 2 nearest points from point x in zigzag numbering.
//W are weights for the points, calculated according to the inverse of their distances
//from x
  const Double_t un  = 1.0;
  const Double_t eps = 1.0e-12;
  Int_t k;
  Double_t xl,xr,x1,x2,dx,dxs2,w0,w1,wt;
  I.Set(2);
  W.Set(2);
  dx   = (fXmax-fXmin)/fNx;
  dxs2 = dx/2;
  xl   = dxs2;
  xr   = dxs2 + (fNx-1)*dx;
  if (x<xl) {
    I[0] = 0;
    I[1] = 1;
    x1   = dxs2;
    x2   = dxs2 + dx;
  }
  else {
    if (x>xr) {
      I[0] = fNx - 1;
      I[1] = fNx - 2;
      x1   = dxs2 + (fNx-1)*dx;
      x2   = x1 - dx;
    }
    else {
      k    = Int_t((x-dxs2)/dx);
      I[0] = k;
      I[1] = k+1;
      x1   = dxs2 + k*dx;
      x2   = x1 + dx;
    }
  }
  w0   = TMath::Abs(x1-x);
  if (w0<eps) w0 = eps;
  w0   = un/w0;
  w1   = TMath::Abs(x2-x);
  if (w1<eps) w1 = eps;
  w1   = un/w1;
  wt   = w0 + w1;
  W[0] = w0/wt;
  W[1] = w1/wt;
  return kTRUE;
}
Bool_t TZigZag::NearestPoints(Double_t x, Double_t y, Double_t z, TArrayI &I, TArrayD &W) const {
// 3-dimensional case. Gives the 8 nearest points from point x in zigzag numbering.
//W are weights for the points, calculated according to the inverse of their distances
//from x. If point (x,y) not inside [fXmin,fXmax],[fYmin,fYmax],[fZmin,fZmax], make a
//projection towards center and stops at point just after entry and gives the 8 points
//for it.
  const Double_t un   = 1.0;
  const Double_t aeps = 1.0e-6;
  Bool_t ok;
  Int_t i;
  Int_t kx=0;
  Int_t ky=0;
  Int_t kz=0;
  Double_t mx,my,mz,eps;
  Double_t xc,xl,xr,dx,dxs2;
  Double_t yc,yl,yr,dy,dys2;
  Double_t zc,zl,zr,dz,dzs2;
  Double_t xi,yi,zi,ti,xp,yp,zp;
  Double_t xle,xre,yle,yre,zle,zre;
  Double_t w0,w1,w2,w3,w4,w5,w6,w7,wt;
  I.Set(8);
  W.Set(8);
  for (i=0;i<8;i++) {
    I[i] = -1;
    W[i] = -un;
  }
  dx   = (fXmax-fXmin)/fNx;
  dxs2 = dx/2;
  dy   = (fYmax-fYmin)/fNy;
  dys2 = dy/2;
  dz   = (fZmax-fZmin)/fNy;
  dzs2 = dz/2;
  xl   = dxs2;
  xr   = dxs2 + (fNx-1)*dx;
  yl   = dys2;
  yr   = dys2 + (fNy-1)*dy;
  zl   = dzs2;
  zr   = dzs2 + (fNz-1)*dz;
  w0   = xr - xl;
  w1   = yr - yl;
  w2   = zr - zl;
  w0   = TMath::Max(w0,w1);
  w0   = TMath::Max(w0,w2);
  eps  = aeps*w0;
  if ((x>=xl) && (x<=xr) && (y>=yl) && (y<=yr) && (z>=zl) && (z<=zr)) {
    xi = x;
    yi = y;
    zi = z;
    kx = Int_t((xi-dxs2)/dx) + 1;
    ky = Int_t((yi-dys2)/dy) + 1;
    kz = Int_t((zi-dzs2)/dz) + 1;
    ok = kTRUE;
  }//end if ((x>xl) && (x<xr) && (y>yl) && (y<yr) && (z>zl) && (z<zr))
  else {
    xc = (fXmax-fXmin)/2;
    yc = (fYmax-fYmin)/2;
    zc = (fZmax-fZmin)/2;
    xle = xl + eps;
    xre = xr - eps;
    yle = yl + eps;
    yre = yr - eps;
    zle = zl + eps;
    zre = zr - eps;
    mx  = x - xc;
    my  = y - yc;
    mz  = z - zc;
//intercept with xle
    xi = xle;
    ti = (xi-xc)/mx;
    yi = yc + my*ti;
    zi = zc + mz*ti;
    ok = IsInside(xi,yi,zi,x,y,z,xc,yc,zc,xl,xr,yl,yr,zl,zr);
    if (!ok) {
//intercept with xre
      xi = xre;
      ti = (xi-xc)/mx;
      yi = yc + my*ti;
      zi = zc + mz*ti;
      ok = IsInside(xi,yi,zi,x,y,z,xc,yc,zc,xl,xr,yl,yr,zl,zr);
      if (!ok) {
//intercept with yle
        yi = yle;
        ti = (yi-yc)/my;
        xi = xc + mx*ti;
        zi = zc + mz*ti;
        ok = IsInside(xi,yi,zi,x,y,z,xc,yc,zc,xl,xr,yl,yr,zl,zr);
        if (!ok) {
//intercept with yre
          yi = yre;
          ti = (yi-yc)/my;
          xi = xc + mx*ti;
          zi = zc + mz*ti;
          ok = IsInside(xi,yi,zi,x,y,z,xc,yc,zc,xl,xr,yl,yr,zl,zr);
          if (!ok) {
//intercept with zle
            zi = zle;
            ti = (zi-zc)/mz;
            xi = xc + mx*ti;
            yi = yc + my*ti;
            ok = IsInside(xi,yi,zi,x,y,z,xc,yc,zc,xl,xr,yl,yr,zl,zr);
            if (!ok) {
//intercept with zre
              zi = zre;
              ti = (zi-zc)/mz;
              xi = xc + mx*ti;
              yi = yc + my*ti;
              ok = IsInside(xi,yi,zi,x,y,z,xc,yc,zc,xl,xr,yl,yr,zl,zr);
            }//end if (!ok) intercept with zre
          }//end if (!ok) intercept with zle
        }//end if (!ok) intercept with yre
      }//end if (!ok) intercept with yle
    }//end if (!ok) intercept with xre
    if (ok) {
      kx = Int_t((xi-dxs2)/dx) + 1;
      ky = Int_t((yi-dys2)/dy) + 1;
      kz = Int_t((zi-dzs2)/dz) + 1;
    }
    else {
      std::cout << "TZigZag::NearestPoints : ERROR point inside not found" << std::endl;
    }
  }//end else if ((x>xl) && (x<xr) && (y>yl) && (y<yr) && (z>zl) && (z<zr))
  if (ok) {
    Double_t fx,fy,fz;
//Point kx,ky,kz
    xp   = dxs2 + (kx-1)*dx;
    yp   = dys2 + (ky-1)*dy;
    zp   = dzs2 + (kz-1)*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx,ky,kz);
    I[0] = i;
    w0   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w0   = un/w0;
//Point kx+1,ky,kz
    xp   = dxs2 + kx*dx;
    yp   = dys2 + (ky-1)*dy;
    zp   = dzs2 + (kz-1)*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx+1,ky,kz);
    I[1] = i;
    w1   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w1   = un/w1;
//Point kx,ky+1,kz
    xp   = dxs2 + (kx-1)*dx;
    yp   = dys2 + ky*dy;
    zp   = dzs2 + (kz-1)*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx,ky+1,kz);
    I[2] = i;
    w2   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w2   = un/w2;
//Point kx+1,ky+1,kz
    xp   = dxs2 + kx*dx;
    yp   = dys2 + ky*dy;
    zp   = dzs2 + (kz-1)*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx+1,ky+1,kz);
    I[3] = i;
    w3   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w3   = un/w3;
//Point kx,ky,kz+1
    xp   = dxs2 + (kx-1)*dx;
    yp   = dys2 + (ky-1)*dy;
    zp   = dzs2 + kz*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx,ky,kz+1);
    I[4] = i;
    w4   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w4   = un/w4;
//Point kx+1,ky,kz+1
    xp   = dxs2 + kx*dx;
    yp   = dys2 + (ky-1)*dy;
    zp   = dzs2 + kz*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx+1,ky,kz+1);
    I[5] = i;
    w5   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w5   = un/w5;
//Point kx,ky+1,kz+1
    xp   = dxs2 + (kx-1)*dx;
    yp   = dys2 + ky*dy;
    zp   = dzs2 + kz*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx,ky+1,kz+1);
    I[6] = i;
    w6   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w6   = un/w6;
//Point kx+1,ky+1,kz+1
    xp   = dxs2 + kx*dx;
    yp   = dys2 + ky*dy;
    zp   = dzs2 + kz*dz;
    fx   = x - xp;
    fy   = y - yp;
    fz   = z - zp;
    i    = NToZZ(kx+1,ky+1,kz+1);
    I[7] = i;
    w7   = TMath::Sqrt(fx*fx + fy*fy + fz*fz) + eps;
    w7   = un/w7;
//
    wt   = w0 + w1 + w2 + w3 + w4 + w5 + w6 + w7;
    W[0] = w0/wt;
    W[1] = w1/wt;
    W[2] = w2/wt;
    W[3] = w3/wt;
    W[4] = w4/wt;
    W[5] = w5/wt;
    W[6] = w6/wt;
    W[7] = w7/wt;
  }
  return ok;
}
Int_t TZigZag::NToZZ(Int_t i) const {
// Goes from "normal" numbering (beginning with 1) to "zigzag" numbering (beginning
//with 0 and proceeding in a zigzag way).
  Int_t k,n,j,ix,iy,iz,i1;
  i1 = i - 1;
  switch (fDim) {
    case 1:
      k = i-1;
      break;
    case 2:
      ix = i1%fNx + 1;
      iy = i1/fNx + 1;
      k  = NToZZ(ix,iy);
      break;
    case 3:
      n  = fNx*fNy;
      iz = i1/n + 1;
      j  = i - n*(iz-1) - 1;
      ix = j%fNx + 1;
      iy = j/fNx + 1;
      k  = NToZZ(ix,iy,iz);
      break;
    default:
      k = -1;
      std::cout << "TZigZag::NTOZZ ERROR bad dimension" << std::endl;
      break;
  }
  return k;
}
Int_t TZigZag::NToZZ(Int_t ix,Int_t iy) const {
// Case of dimension 2. ix and iy begin with 1, not 0!
// Goes from "normal" numbering (beginning with 1) to "zigzag" numbering (beginning
//with 0 and proceeding in a zigzag way).
  Int_t ix1,iy1,jx;
  Int_t k;
  ix1 = ix - 1;
  iy1 = iy - 1;
  if (iy%2) jx = ix1;
  else      jx = fNx - ix;
  k = fNx*iy1 + jx;
  return k;
}
Int_t TZigZag::NToZZ(Int_t ix,Int_t iy,Int_t iz) const {
// Case of dimension 3. ix,iy and iz begin with 1, not 0!
// Goes from "normal" numbering (beginning with 1) to "zigzag" numbering (beginning
//with 0 and proceeding in a zigzag way).
  Int_t ix1,iy1,iz1,jy,n;
  Int_t k;
  n = fNx*fNy;
  ix1 = ix - 1;
  iy1 = iy - 1;
  iz1 = iz - 1;
  if (iz%2) jy = NToZZ(ix,iy);
  else      jy = n - NToZZ(ix,iy) - 1;
  k = n*iz1 + jy;
  return k;
}
void TZigZag::Order(Int_t n, TArrayI &Ii, TArrayD &Ti, TArrayD &Xi, TArrayD &Yi) const {
// Put points in order of increasing zigzag numbering
  Bool_t ok;
  Int_t i,i1,n1,it;
  Double_t tt,xt,yt;
  n1 = n - 1;
  ok = kFALSE;
  while (!ok) {
    ok = kTRUE;
    for (i=0;i<n1;i++) {
      i1 = i + 1;
      if (Ii[i1]<Ii[i]) {
        ok = kFALSE;
        it = Ii[i1];
        tt = Ti[i1];
        xt = Xi[i1];
        yt = Yi[i1];
        Ii[i1] = Ii[i];
        Ti[i1] = Ti[i];
        Xi[i1] = Xi[i];
        Yi[i1] = Yi[i];
        Ii[i]  = it;
        Ti[i]  = tt;
        Xi[i]  = xt;
        Yi[i]  = yt;
      }
    }
  }
}
Bool_t TZigZag::PointsNear(Double_t x, Double_t y, Double_t &yi,
  TArrayD &Tn, TArrayD &Yn) const {
// Two-dimensional case. Finds in I,T,X,Y the 4 points of the grid around point (x,y).
//Then finds the 2 closest points along the zigzag in In,Tn,Xn,Yn.
// If point (x,y) not inside [fXmin,fXmax] and [fYmin,fYmax], make a projection
//towards center and stops at point just after entry and gives the 4 points for it.
  const Double_t un   = 1.0;
  const Double_t aeps = 1.0e-6;
  TArrayI Ii(4);
  TArrayD Ti(4);
  TArrayD Xi(4);
  TArrayD Yi(4);
  Bool_t ok;
  Int_t i;
  Int_t kx=0;
  Int_t ky=0;
  Double_t mx,my,eps;
  Double_t xc,xl,xr,dx,dxs2;
  Double_t yc,yl,yr,dy,dys2;
  Double_t xi,xp,yp;
  Double_t xle,xre,yle,yre;
  Tn.Set(2);
  Yn.Set(2);
  dx   = (fXmax-fXmin)/fNx;
  dxs2 = dx/2;
  dy   = (fYmax-fYmin)/fNy;
  dys2 = dy/2;
  xl   = dxs2;
  xr   = dxs2 + (fNx-1)*dx;
  yl   = dys2;
  yr   = dys2 + (fNy-1)*dy;
  if ((yr-yl) > (xr-xl)) eps = aeps*(yr-yl);
  else                   eps = aeps*(xr-xl);
  xle = xl + eps;
  xre = xr - eps;
  yle = yl + eps;
  yre = yr - eps;
  if ((x>=xle) && (x<=xre) && (y>=yle) && (y<=yre)) {
    xi = x;
    yi = y;
    kx = Int_t((xi-dxs2)/dx) + 1;
    ky = Int_t((yi-dys2)/dy) + 1;
    ok = kTRUE;
  }//end if ((x>xl) && (x<xr) && (y>yl) && (y<yr))
  else {
    xc = (fXmax-fXmin)/2;
    yc = (fYmax-fYmin)/2;
    mx  = (y-yc)/(x-xc);
    my  = un/mx;
    xi = xle;
    yi = yc + mx*(xi-xc);
    ok = IsInside(xi,yi,x,y,xc,yc,xl,xr,yl,yr);
    if (!ok) {
      xi = xre;
      yi = yc + mx*(xi-xc);
      ok = IsInside(xi,yi,x,y,xc,yc,xl,xr,yl,yr);
      if (!ok) {
        yi = yle;
        xi = xc + my*(yi-yc);
        ok = IsInside(xi,yi,x,y,xc,yc,xl,xr,yl,yr);
        if (!ok) {
          yi = yre;
          xi = xc + my*(yi-yc);
          ok = IsInside(xi,yi,x,y,xc,yc,xl,xr,yl,yr);
        }//end if (!ok)
      }//end if (!ok)
    }//end if (!ok)
    if (ok) {
      kx = Int_t((xi-dxs2)/dx) + 1;
      ky = Int_t((yi-dys2)/dy) + 1;
    }
    else {
      std::cout << "TZigZag::PointsNear : ERROR point inside not found" << std::endl;
    }
  }//end else if ((x>xl) && (x<xr) && (y>yl) && (y<yr))
  if (ok) {
//Point kx,ky
    xp    = dxs2 + (kx-1)*dx;
    yp    = dys2 + (ky-1)*dy;
    i     = NToZZ(kx,ky);
    Ii[0] = i;
    Ti[0] = T(i);
    Xi[0] = xp;
    Yi[0] = yp;
//Point kx+1,ky
    xp    = dxs2 + kx*dx;
    yp    = dys2 + (ky-1)*dy;
    i     = NToZZ(kx+1,ky);
    Ii[1] = i;
    Ti[1] = T(i);
    Xi[1] = xp;
    Yi[1] = yp;
//Point kx,ky+1
    xp    = dxs2 + (kx-1)*dx;
    yp    = dys2 + ky*dy;
    i     = NToZZ(kx,ky+1);
    Ii[2] = i;
    Ti[2] = T(i);
    Xi[2] = xp;
    Yi[2] = yp;
//Point kx+1,ky+1
    xp    = dxs2 + kx*dx;
    yp    = dys2 + ky*dy;
    i     = NToZZ(kx+1,ky+1);
    Ii[3] = i;
    Ti[3] = T(i);
    Xi[3] = xp;
    Yi[3] = yp;
//Finding the 2 points along the zigzag
    Order(4,Ii,Ti,Xi,Yi);
    Yn[0] = Yi[0];
    Tn[0] = Ti[0] + ((Ti[1] - Ti[0])/(Xi[1] -Xi[0]))*(xi - Xi[0]);
    Yn[1] = Yi[2];
    Tn[1] = Ti[2] + ((Ti[3] - Ti[2])/(Xi[3] -Xi[2]))*(xi - Xi[2]);
  }
  return ok;
}
Double_t TZigZag::T(Int_t i) const {
// Given the point i (in the zigzag numbering), find the coordinate t, i.e. the path
//length along the zigzag.
  Double_t t,tp;
  Int_t ix,iy,iz,n,j;
  Double_t dx,dy,dz,dxs2;
  dx   = (fXmax-fXmin)/fNx;
  dxs2 = dx/2;
  switch (fDim) {
    case 1:
      t = dxs2 + i*dx;
      break;
    case 2:
      dy   = (fYmax-fYmin)/fNy;
      iy   = i/fNx;
      ix   = i - fNx*iy;
      t    = dxs2 + ((fNx-1)*dx + dy)*iy + ix*dx;
      break;
    case 3:
      dy   = (fYmax-fYmin)/fNy;
      dz   = (fZmax-fZmin)/fNz;
      n    = fNx*fNy;
      iz   = i/n;
      tp   = fNy*(fNx-1)*dx + (fNy-1)*dy + dz;
      j    = i - n*iz;
      iy   = j/fNx;
      ix   = j - fNx*iy;
      t    = dxs2 + tp*iz + ((fNx-1)*dx + dy)*iy + ix*dx;
      break;
    default:
      t = -1;
      std::cout << "TZigZag::T() ERROR bad dimension" << std::endl;
      break;
  }
  return t;
}
Double_t TZigZag::TMax() const {
// Extremity of t interval
  Double_t dxs2;
  Double_t tmax = -1;
  if ((fDim>0) && (fDim<=3)) {
    dxs2 = (fXmax-fXmin)/(2*fNx);
    tmax = T(fNx*fNy*fNz-1) + dxs2;
  }
  return tmax;
}

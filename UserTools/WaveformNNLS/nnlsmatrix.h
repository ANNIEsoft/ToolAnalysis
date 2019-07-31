// File: nnlsmatrix.h        -*- c++ -*-
// Author: Suvrit Sra
// Time-stamp: <04 March 2011 07:35:51 PM CET --  suvrit>
// Base class for both sparse and dense matrices....

// nnlsmatrix.h  - minimal base class for dense and sparse matrices
// Copyright (C) 2010 Suvrit Sra (suvrit@tuebingen.mpg.de)

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#ifndef nnlsmatrix_H
#define nnlsmatrix_H

#include <cstring>

#include "nnlsvector.h"

class nnlsmatrix {
size_t M;               /// Num of rows
size_t N;               /// Num of colums

protected:
size_t memory;
public:

static const bool TRAN = true;
static const bool NOTRAN = false;

nnlsmatrix();

/// Create an empty (unallocated) nnlsmatrix of specified size.
nnlsmatrix (size_t rows, size_t cols);

virtual ~nnlsmatrix();


virtual int load(const char* f, bool asbin) = 0;

void setsize(size_t r, size_t c) { M = r; N = c;}
/// Returns number of rows. 
size_t nrows() const { return M;}

/// Returns number of colums
size_t ncols() const { return N;}

/// Sets the row and column dimensionality of the nnlsmatrix
void nnlsmatrix_setsize(size_t m, size_t n) { M = m; N = n; }

/// Get the (i,j) entry of the nnlsmatrix
virtual double operator()   (size_t i, size_t j            ) = 0;
/// Get the (i,j) entry of the nnlsmatrix
virtual double get          (size_t i, size_t j            ) = 0;
/// Set the (i,j) entry of the nnlsmatrix. Not all matrices can support this.
virtual int set            (size_t i, size_t j, double val) = 0;

/// Returns 'r'-th row as a nnlsvector
virtual int get_row (size_t, nnlsvector*&) = 0;
/// Returns 'c'-th col as a nnlsvector
virtual int get_col (size_t, nnlsvector*&) = 0;
/// Returns main or second diagonal (if p == true)
virtual int get_diag(bool p, nnlsvector*& d  ) = 0;

/// Sets the specified row to the given nnlsvector
virtual int set_row(size_t r, nnlsvector*&) = 0;
/// Sets the specified col to the given nnlsvector
virtual int set_col(size_t c, nnlsvector*&) = 0;
/// Sets the specified diagonal to the given nnlsvector
virtual int set_diag(bool p, nnlsvector*&)  = 0;

/// nnlsvector l_p norms for this nnlsmatrix, p > 0
virtual double norm (double p) = 0;
/// nnlsvector l_p norms, p is 'l1', 'l2', 'fro', 'inf'
virtual double norm (const char*  p) = 0;

/// Apply an arbitrary function elementwise to this nnlsmatrix. Changes the nnlsmatrix.
virtual int apply (double (* fn)(double)) = 0;

/// Scale the nnlsmatrix so that x_ij := s * x_ij
virtual int scale (double s) = 0;

/// Adds a const 's' so that x_ij := s + x_ij
virtual int add_const(double s) = 0;

/// r = a*row(i) + r
virtual int    row_daxpy(size_t i, double a, nnlsvector* r) = 0;
/// c = a*col(j) + c
virtual int  col_daxpy(size_t j, double a, nnlsvector* c) = 0;

/// Let r := this * x or  this^T * x depending on tranA
virtual int dot (bool transp, nnlsvector* x, nnlsvector*r) = 0;

/// get statistics about storage
virtual size_t memoryUsage() = 0;
};

#endif

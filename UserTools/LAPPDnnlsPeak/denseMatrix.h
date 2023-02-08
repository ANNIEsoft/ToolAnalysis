// File: denseMatrix.h -*- c++ -*-
// Author: Suvrit Sra <suvrit@tuebingen.mpg.de>
// (c) Copyright 2010   Suvrit Sra
// Max-Planck-Institute for Biological Cybernetics

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

#ifndef denseMatrix_H
#define denseMatrix_H

#include "nnlsmatrix.h"

#define forall(x) for (size_t x = 0; x < size; x++)

class denseMatrix : public nnlsmatrix {
size_t size;
double* data;
bool external;              // is data managed externally
public:


denseMatrix();

denseMatrix(size_t r, size_t c);

~denseMatrix();


int load(const char* fn, bool asbin);

private:
int load_as_bin(const char*);
int load_as_txt(const char*);
public:
/// Get the (i,j) entry of the matrix
double operator()   (size_t i, size_t j) { return data[i*ncols()+j];}

/// Get the (i,j) entry of the matrix
double get (size_t i, size_t j) { return data[i*ncols()+j];}

/// Set the (i,j) entry of the matrix. Not all matrices can support this.
int set (size_t i, size_t j, double val) { data[i*ncols()+j] = val; return 0;}


/// Returns 'r'-th row into pre-alloced nnlsvector
int get_row (size_t, nnlsvector*&);
/// Returns 'c'-th col as a nnlsvector
int get_col (size_t, nnlsvector*&);
/// Returns main or second diagonal (if p == true)
int get_diag(bool p, nnlsvector*& d);

/// Sets the specified row to the given nnlsvector
int set_row(size_t r, nnlsvector*&);
/// Sets the specified col to the given nnlsvector
int set_col(size_t c, nnlsvector*&);
/// Sets the specified diagonal to the given nnlsvector
int set_diag(bool p, nnlsvector*&);

/// nnlsvector l_p norms for this matrix, p > 0
double norm (double p);
/// nnlsvector l_p norms, p is 'l1', 'l2', 'fro', 'inf'
double norm (const char*  p);

/// Apply an arbitrary function elementwise to this matrix. Changes the matrix.
int apply (double (* fn)(double)) { forall(i) data[i] = fn(data[i]); return 0;};

/// Scale the matrix so that x_ij := s * x_ij
int scale (double s) { forall(i) data[i] *= s; return 0;}

/// Adds a const 's' so that x_ij := s + x_ij
int add_const(double s) { forall(i) data[i] += s; return 0;};

/// r = a*row(i) + r
 int    row_daxpy(size_t i, double a, nnlsvector* r);
/// c = a*col(j) + c
 int  col_daxpy(size_t j, double a, nnlsvector* c);

/// Let r := this * x or  this^T * x depending on tranA
int dot (bool transp, nnlsvector* x, nnlsvector*r);

size_t memoryUsage() { return nrows()*ncols()*sizeof(double);}
};

#endif

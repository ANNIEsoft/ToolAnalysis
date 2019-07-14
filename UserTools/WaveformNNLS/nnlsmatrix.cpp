#include "nnlsmatrix.h"


nnlsmatrix::nnlsmatrix() {}

/// Create an empty (unallocated) nnlsmatrix of specified size.
nnlsmatrix::nnlsmatrix (size_t rows, size_t cols) { M = rows;   N = cols;   }

nnlsmatrix::~nnlsmatrix() {}

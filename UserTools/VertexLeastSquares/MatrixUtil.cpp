#include "MatrixUtil.h"
#include <iomanip>

namespace util
{
  //------------------------------------------------------------------------------
  void vector_show(const Vector&  m, std::string str)
  {
    std::cout << std::fixed;
    std::cout << std::setprecision(3);
    for(int i = 0; i < m.size; i++) {
      std::cout << m(i) << " ";
      }
    std::cout << "\n";
  }

  //------------------------------------------------------------------------------
  // c = a + b * s
  void vmadd(const Vector& a, const Vector& b, double s, Vector& c)
  {
    if (c.size != a.size or c.size != b.size) {
      std::cerr << "[vmadd]: vector sizes don't match\n";
      return;
    }
  
    for (int i = 0; i < c.size; i++)
      c(i) = a(i) + s * b(i);
  }

  void mult_matvec(const Matrix& a, const Vector& b, Vector &c)
  {
    if (a.n != b.size) {
      std::cerr << "[matvecmult]: Matrix-Vector multiplication not possible, sizes don't match !\n";
      return;
    }

    if (a.m != c.size) {
      std::cerr << "[matvecmult]: vector sizes don't match\n";
      return;
    }
    
    for (int i = 0; i < a.m; i++)
      for (int k = 0; k < a.n; k++)
	c(i) += a(i,k) * b(k);
  }

  //------------------------------------------------------------------------------
  // mat = I - 2*v*v^T
  // !!! m is allocated here !!!
  void compute_householder_factor(Matrix& mat, const Vector& v)
  {

    int n = v.size;
    mat.allocate(n,n);
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
	mat(i,j) = -2 *  v(i) * v(j);
    for (int i = 0; i < n; i++)
      mat(i,i) += 1;  
  }

  //------------------------------------------------------------------------------
  // take c-th column of a matrix, put results in Vector v
  void Matrix::extract_column(Vector& v, int c) {
    if (m != v.size) {
      std::cerr << "[Matrix::extract_column]: Matrix and Vector sizes don't match\n";
      return;
    }
  
    for (int i = 0; i < m; i++)
      v(i) = (*this)(i,c);
  }

  //------------------------------------------------------------------------------
  void matrix_show(const Matrix&  m, std::string str)
  {
    std::cout << std::fixed;
    std::cout << std::setprecision(3);
    for(int i = 0; i < m.m; i++) {
      for (int j = 0; j < m.n; j++) {
	std::cout << m(i,j) << " ";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  //------------------------------------------------------------------------------
  // L2-norm ||A-B||^2
  double matrix_compare(const Matrix& A, const Matrix& B)
  {
    // matrices must have same size
    if (A.m != B.m or  A.n != B.n)
      return std::numeric_limits<double>::max();

    double res=0;
    for(int i = 0; i < A.m; i++) {
      for (int j = 0; j < A.n; j++) {
	res += (A(i,j)-B(i,j)) * (A(i,j)-B(i,j));
      }
    }

    res /= A.m*A.n;
    return res;
  }

  //------------------------------------------------------------------------------
  // Householder transformation to obtain QR matrices
  void householder(const Matrix &mat, Matrix& R, Matrix& Q)
  {

    int m = mat.m;
    int n = mat.n;

    // array of factor Q1, Q2, ... Qm
    std::vector<Matrix> qv(m);

    // temp array
    Matrix z(mat);
    Matrix z1;
  
    for (int k = 0; k < n && k < m - 1; k++) {

      Vector e(m), x(m);
      double a;
    
      // compute minor
      z1.compute_minor(z, k);
    
      // extract k-th column into x
      z1.extract_column(x, k);
    
      a = x.norm();
      if (mat(k,k) > 0) a = -a;
    
      for (int i = 0; i < e.size; i++)
	e(i) = (i == k) ? 1 : 0;

      // e = x + a*e
      vmadd(x, e, a, e);

      // e = e / ||e||
      e.rescale_unit();
    
      // qv[k] = I - 2 *e*e^T
      compute_householder_factor(qv[k], e);

      // z = qv[k] * z1
      z.mult(qv[k], z1);

    }
  
    Q = qv[0];

    // after this loop, we will obtain Q (up to a transpose operation)
    for (int i = 1; i < n && i < m - 1; i++) {

      z1.mult(qv[i], Q);
      Q = z1;
    
    }
  
    R.mult(Q, mat);
    Q.transpose();
  }

  //------------------------------------------------------------------------------
  void solve_upper_triangular(const Matrix &r, const Vector &b, Vector &solution)
  {
    const int32_t ncolumns = solution.size;

    for ( int32_t k = ncolumns - 1; k >= 0; --k ) {
      double total = 0.0;
      for ( int32_t j = k + 1; j < ncolumns; ++j ) {
	total += r(k, j) * solution(j);
      }
      solution(k) =( b(k) - total ) / r(k, k);
    }
  }

  //------------------------------------------------------------------------------
  void least_squares(const Matrix &A, const Vector &b, Vector& solution, bool verbose)
  {
    Matrix Q(A.n, A.n);
    Matrix R(A.m, A.n);
    
    householder(A, R, Q);

    Q.transpose();

    Vector c(Q.m);
    mult_matvec(Q, b, c);

    if (verbose) {
      std::cout << "A" << std::endl;
      matrix_show(A, "");
      std::cout << "b" << std::endl;
      vector_show(b, "");
      std::cout << "Q" << std::endl;
      matrix_show(Q, "");
      std::cout << "R" << std::endl;
      matrix_show(R, "");
      std::cout << "c" << std::endl;
      vector_show(c, "");
    }

    solve_upper_triangular(R, c, solution);
  }
}

#include <cstdio>
#include <cstdlib>
#include <cstring> // for memset
#include <limits>
#include <iostream>
#include <vector>

#include <math.h>

namespace util
{

  // column vector
  class Vector {

  public:
    // default constructor (don't allocate)
  Vector() : size(0), data(nullptr) {}
  
    // constructor with memory allocation, initialized to zero
  Vector(int size_) : Vector() {
      size = size_;
      allocate(size_);
    }

    // destructor
    ~Vector() {
      deallocate();
    }

    // access data operators
    double& operator() (int i) {
      return data[i]; }
    double  operator() (int i) const {
      return data[i]; }

    // operator assignment
    Vector& operator=(const Vector& source) {
    
      // self-assignment check
      if (this != &source) { 
	if ( size != (source.size) ) {   // storage cannot be reused
	  allocate(source.size);         // re-allocate storage
	}
	// storage can be used, copy data
	std::copy(source.data, source.data + source.size, data);
      }
      return *this;
    }

    // memory allocation
    void allocate(int size_) {

      deallocate();
    
      // new sizes
      size = size_;
    
      data = new double[size_];
      memset(data,0,size_*sizeof(double));

    } // allocate

    // memory free
    void deallocate() {

      if (data)
	delete[] data;

      data = nullptr;

    }    

    //   ||x||
    double norm() {
      double sum = 0;
      for (int i = 0; i < size; i++) sum += (*this)(i) * (*this)(i);
      return sqrt(sum);
    }

    // divide data by factor
    void rescale(double factor) {
      for (int i = 0; i < size; i++) (*this)(i) /= factor;
    }

    void rescale_unit() {
      double factor = norm();
      rescale(factor);
    }
  
    int size;
  
  private:
    double* data;

  }; // class Vector

  //------------------------------------------------------------------------------
  class Matrix {
    
  public:
    // default constructor (don't allocate)
  Matrix() : m(0), n(0), data(nullptr) {}
  
    // constructor with memory allocation, initialized to zero
  Matrix(int m_, int n_) : Matrix() {
      m = m_;
      n = n_;
      allocate(m_,n_);
    }

    // copy constructor
  Matrix(const Matrix& mat) : Matrix(mat.m,mat.n) {

      for (int i = 0; i < m; i++)
	for (int j = 0; j < n; j++)
	  (*this)(i,j) = mat(i,j);
    }
  
    // constructor from array
    template<int rows, int cols>
      Matrix(double (&a)[rows][cols]) : Matrix(rows,cols) {

      for (int i = 0; i < m; i++)
	for (int j = 0; j < n; j++)
	  (*this)(i,j) = a[i][j];
    }

    // constructor from Vector, creates a vertical matrix
  Matrix(const Vector& vec) : Matrix(vec.size, 1) {

      for (int i = 0; i < m; i++)
	(*this)(i,0) = vec(i);
    }


    // destructor
    ~Matrix() {
      deallocate();      
    }


    // access data operators
    double& operator() (int i, int j) {
      return data[i+m*j]; }
    double  operator() (int i, int j) const {
      return data[i+m*j]; }

    // operator assignment
    Matrix& operator=(const Matrix& source) {
    
      // self-assignment check
      if (this != &source) { 
	if ( (m*n) != (source.m * source.n) ) { // storage cannot be reused
	  allocate(source.m,source.n);          // re-allocate storage
	}
	// storage can be used, copy data
	std::copy(source.data, source.data + source.m*source.n, data);
      }
      return *this;
    }
  
    // compute minor
    void compute_minor(const Matrix& mat, int d) {

      allocate(mat.m, mat.n);
    
      for (int i = 0; i < d; i++)
	(*this)(i,i) = 1.0;
      for (int i = d; i < mat.m; i++)
	for (int j = d; j < mat.n; j++)
	  (*this)(i,j) = mat(i,j);
    
    }

    // Matrix multiplication
    // c = a * b
    // c will be re-allocated here
    void mult(const Matrix& a, const Matrix& b) {

      if (a.n != b.m) {
	std::cerr << "Matrix multiplication not possible, sizes don't match !\n";
	return;
      }

      // reallocate ourself if necessary i.e. current Matrix has not valid sizes
      if (a.m != m or b.n != n)
	allocate(a.m, b.n);

      memset(data,0,m*n*sizeof(double));
    
      for (int i = 0; i < a.m; i++)
	for (int j = 0; j < b.n; j++)
	  for (int k = 0; k < a.n; k++)
	    (*this)(i,j) += a(i,k) * b(k,j);
    
    }

    void transpose() {
      for (int i = 0; i < m; i++) {
	for (int j = 0; j < i; j++) {
	  double t = (*this)(i,j);
	  (*this)(i,j) = (*this)(j,i);
	  (*this)(j,i) = t;
	}
      }
    }

    // take c-th column of m, put in v
    void extract_column(Vector& v, int c);  

    // memory allocation
    void allocate(int m_, int n_) {

      // if already allocated, memory is freed
      deallocate();
    
      // new sizes
      m = m_;
      n = n_;
    
      data = new double[m_*n_];
      memset(data,0,m_*n_*sizeof(double));

    } // allocate

    // memory free
    void deallocate() {

      if (data)
	delete[] data;

      data = nullptr;

    }    
  
    int m, n;
  
  private:
    double* data;
  
  }; // struct Matrix

  // Other useful functions
  void vector_show(const Vector&  m, std::string str);
  void vmadd(const Vector& a, const Vector& b, double s, Vector& c);
  void mult_matvec(const Matrix& a, const Vector& b, Vector &c);
  void compute_householder_factor(Matrix& mat, const Vector& v);
  void matrix_show(const Matrix&  m, std::string str);
  double matrix_compare(const Matrix& A, const Matrix& B);
  void householder(const Matrix &mat, Matrix& R, Matrix& Q);
  void solve_upper_triangular(const Matrix &r, const Vector &b, Vector &solution);
  void least_squares(const Matrix &A, const Vector &b, Vector &solution, bool verbose = false);
}

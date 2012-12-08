#ifndef SVM_H
#define SVM_H

#include <iostream>
#include <cstdio>
#include <exception>
#include <cmath>
#include "QuadProg++.hh"

class Kernel {
public:
  virtual double operator() (const Vector<double> x, const Vector<double> y) = 0;
};

class DotProd : public Kernel {
public:
  double operator() (const Vector<double> x, const Vector<double> y) {
    return dot_prod(x, y);
  };
};

class Polynomial : public Kernel {
public:
  Polynomial(double d):d(d){};
  double operator() (const Vector<double> x, const Vector<double> y) {
    return pow(1.0 + dot_prod(x, y), d); 
  };
private:
  double d;
};

class Gaussian : public Kernel {
public:
  Gaussian(double sigma):sigma(sigma){};
  double operator() (const Vector<double> x, const Vector<double> y) {
    return exp(-sum(pow(x - y, 2)) / 2.0 / sigma / sigma);
  };
private:
  double sigma;
};

class Hyperbolic : public Kernel {
public:
  Hyperbolic(double a, double b):a(a), b(b){};
  double operator() (const Vector<double> x, const Vector<double> y) {
    return tanh(a * dot_prod(x, y) + b);
  };
private:
  double a, b;
};

class SVM {
public:
  SVM(Matrix<double> x, Vector<double> y, Kernel* kernel = new DotProd()):kernel(kernel), x(x), y(y) {
    int r = x.nrows();
    int n = r, m = r, p = 1;
    Matrix<double> G(n, m), CE(n, p), CI(n, m);
    Vector<double> g0(n), ce0(p), ci0(m);
  
    for (int i = 0; i < n; i++)
      for (int j = 0; j < m; j++) {
        G[i][j] = (*kernel)(x.extractRow(i), x.extractRow(j)) * y[i] * y[j];
        // add an minute value to diagonal elements
        if (i == j) G[i][j] += 1.9e-9;
      }
    for (int i = 0; i < n; i++) g0[i] = -1.0;
    for (int i = 0; i < n; i++) CE[i][0] = y[i];
    ce0[0] = 0.0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < m; j++)
        CI[i][j] = i == j ? 1.0 : 0.0;
    for (int i = 0; i < m; i++) ci0[i] = 0.0;

    alpha.resize(n);

    try {
      solve_quadprog(G, g0, CE, ce0, CI, ci0, alpha);
    } catch (const std::exception& ex) {
      std::cerr << "\n!!! lerning failed !!!\n" << ex.what() << std::endl;
      throw;
    }

    theta = sum(x.extractRow(0)) - y[0];
  };

  void dump_alpha() {
    for (int i = 0; i < alpha.size(); i++)
      printf("alpha[%d] = %.6f\n", i, alpha[i]);
  };

  double discriminant(Vector<double> v) {
    return sum(v) - theta;
  };

private:
  Kernel* kernel;
  Vector<double> alpha;
  double theta;
  Matrix<double> x;
  Vector<double> y;

  double sum(Vector<double> v) {
    double ret = 0.0;
    for (int i = 0; i < x.ncols(); i++)
      ret += alpha[i] * y[i] * (*kernel)(v, x.extractRow(i));
    return ret;
  }
};

#endif

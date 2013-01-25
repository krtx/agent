#ifndef SVM_H
#define SVM_H

#include <iostream>
#include <algorithm>
#include <set>
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
  SVM(Matrix<double> x,
      Vector<double> y,
      Kernel* kernel = new DotProd(),
      double c = -1.0):kernel(kernel), x(x), y(y) {

    // 重複する列を削除する
    std::set<unsigned int> indexes;
    for (size_t i = 0; i < x.nrows(); i++) indexes.insert(i);
    for (size_t i = 0; i < x.nrows(); i++) {
      for (size_t j = i + 1; j < x.nrows(); j++) {
        bool same = true;
        for (size_t k = 0; k < x.ncols(); k++)
          if (x.extractRow(i)[k] != x.extractRow(j)[k]) {
            same = false; break;
          }
        if (same) { indexes.erase(i); break; }
      }
    }
    for (std::set<unsigned int>::iterator it = indexes.begin();
         it != indexes.end(); it++) {
      printf("%d ", *it);
    }
    x = x.extractRows(indexes);
    y = y.extract(indexes);
    
    puts("given values");
    for (size_t i = 0; i < x.nrows(); i++) {
      for (size_t j = 0; j < x.ncols(); j++) printf("%f ", x[i][j]);
      printf(":: %f\n", y[i]);
    }
    puts("");

    int r = x.nrows();
    int n = r;
    Matrix<double> G(n, n), CE(1.0 * n, 1), CI;
    Vector<double> g0(-1.0, n), ce0(0.0, 1), ci0;

    // y が全て同じ値のときは別処理にしてしまう
    for (size_t i = 0; i < y.size(); i++) printf("%f ", y[i]);
    puts("");
    bool same = true;
    for (size_t i = 0; i < y.size(); i++)
      if (y[i] != y[0]) { same = false; break; }
    if (same) {
      puts("same values");
      theta = 0.0;
      for (size_t i = 0; i < x.ncols(); i++) theta += x[0][i];
      for (size_t i = 0; i < x.nrows(); i++) {
        double tmp = 0.0;
        for (size_t j = 0; j < x.ncols(); j++) tmp += x[i][j];
        if (y[0] > 0) theta = std::min(theta, tmp);
        else theta = std::max(theta, tmp);
      }
    }
    else {
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
          G[i][j] = (*kernel)(x.extractRow(i), x.extractRow(j)) * y[i] * y[j];
          // add an minute value to diagonal elements
          if (i == j) G[i][j] += 1.0e-6;
        }
      for (int i = 0; i < n; i++) CE[i][0] = y[i];
    
      if (c < 1e-9) { // hard margin svm
        CI.resize(0.0, n, n);
        ci0.resize(0.0, n);
        for (int i = 0; i < n; i++) CI[i][i] = 1.0;
      }
      else { // soft margin svm
        CI.resize(0.0, n, 2 * n);
        ci0.resize(0.0, 2 * n);
        for (int i = 0; i < n; i++) {
          CI[i][i] = 1.0;
          CI[i][i + n] = -1.0;
        }
        for (int i = 0; i < n; i++) ci0[i + n] = c;
      }

      alpha.resize(n);

      try {
        solve_quadprog(G, g0, CE, ce0, CI, ci0, alpha);
      } catch (const std::exception& ex) {
        std::cerr << "\n!!! lerning failed !!!\n" << ex.what() << std::endl;
        throw;
      }

      puts("learning ended");

      for (size_t i = 0; i < x.nrows(); i++) {
        if (alpha[i] > 1e-6) {
          theta = -y[i];
          for (size_t j = 0; j < x.nrows(); j++)
            theta += x.extractRow(i)[j];
          //theta = sum(x.extractRow(i)) - y[i];
          break;
        }
      }
    }
  };

  void dump_alpha() {
    for (size_t i = 0; i < alpha.size(); i++)
      printf("alpha[%zu] = %.6f\n", i, alpha[i]);
  };

  double discriminant(Vector<double> v) {
    double ret = -theta;
    for (size_t i = 0; i < v.size(); i++) ret += v[i];
    return ret;
  };

private:
  Kernel* kernel;
  Vector<double> alpha;
  double theta;
  Matrix<double> x;
  Vector<double> y;

  double sum(Vector<double> v) {
    double ret = 0.0;
    for (size_t i = 0; i < x.nrows(); i++)
      ret += alpha[i] * y[i] * (*kernel)(x.extractRow(i), v);
    return ret;
  }
};

#endif

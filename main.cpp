#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <exception>
#include <string>
#include <cstdio>
#include "QuadProg++.hh"
#include "kernel.h"

double train_svm(Matrix<double> x,
                 Vector<double> y,
                 Vector<double>& alpha,
                 Kernel* kernel)
{
  int r = x.nrows();
  int n = r, m = r, p = 1;
  Matrix<double> G(n, m), CE(n, p), CI(n, m);
  Vector<double> g0(n), ce0(p), ci0(m);
  
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) {
      G[i][j] = (*kernel)(x.extractRow(i), x.extractRow(j)) * y[i] * y[j];
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

  return solve_quadprog(G, g0, CE, ce0, CI, ci0, alpha);
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
      std::cerr << "lerning failed: " << ex.what() << std::endl;
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

void read_input(std::istream& ifs, Matrix<double>& x, Vector<double>& y)
{
  std::vector<std::vector<double> > _x;
  std::vector<double> _y;

  std::string line;
  while (ifs && getline(ifs, line)) {
    std::istringstream is(line);
    std::vector<double> v; int val;
    
    while (is >> val) v.push_back(val);
    _y.push_back(v.back()); v.pop_back();

    // remove duplicates
    if (find(_x.begin(), _x.end(), v) == _x.end())
      _x.push_back(v);
    else
      _y.pop_back();
  }

  x.resize(_x.size(), _x[0].size());
  for (int i = 0; i < _x.size(); i++)
    for (int j = 0; j < _x[0].size(); j++)
      x[i][j] = _x[i][j];

  y.resize(_y.size());
  for (int i = 0; i < _y.size(); i++) y[i] = _y[i];
}

int main(int argc, char *const argv[])
{
  if (argc <= 1) puts("usage: ./main datafile");

  std::ifstream ifs(argv[1]);
  Matrix<double> x;
  Vector<double> y;
  read_input(ifs, x, y);

  //Gaussian *g = new Gaussian(10.0);
  SVM svm(x, y);

  svm.dump_alpha();

  return 0;
}


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <exception>
#include <string>
#include <cstdio>
#include "QuadProg++.hh"

double solve_qp(std::vector<Vector<double> > x,
                Vector<double> y,
                Vector<double>& alpha)
{
  int r = x.size();
  int n = r, m = r, p = 1;
  Matrix<double> G(n, m), CE(n, p), CI(n, m);
  Vector<double> g0(n), ce0(p), ci0(m);
  
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) {
      G[i][j] = dot_prod(x[i], x[j]) * y[i] * y[j];
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

void read_input(std::istream& ifs, std::vector<Vector<double> >& x, Vector<double>& y)
{
  std::vector<std::vector<double> > _x;
  std::vector<double> _y;

  std::string line;
  while (ifs && getline(ifs, line)) {
    std::istringstream is(line);
    std::vector<double> v; int val;
    
    while (is >> val) v.push_back(val);
    _y.push_back(v.back()); v.pop_back();
    _x.push_back(v);
  }

  for (int i = 0; i < _x.size(); i++) {
    Vector<double> t; t.resize(_x[i].size());
    for (int j = 0; j < _x[i].size(); j++) t[j] = _x[i][j];
    x.push_back(t);
  }

  y.resize(_y.size());
  for (int i = 0; i < _y.size(); i++) y[i] = _y[i];
}

int main (int argc, char *const argv[])
{
  if (argc <= 1) puts("usage: ./main datafile");

  std::ifstream ifs(argv[1]);
  std::vector<Vector<double> > x;
  Vector<double> y;
  read_input(ifs, x, y);

  Vector<double> alpha;

  solve_qp(x, y, alpha);

  Vector<double> weight(0.0, x[0].size());
  for(int i = 0; i < alpha.size(); i++)
    weight += alpha[i] * y[i] * x[i];

  double theta = dot_prod(weight, x[0]) - y[0];

  Vector<double> a(x[0].size());
  a[0] = 0.0; a[1] = 0.0;
  printf("%f\n", dot_prod(weight, a) - theta);
  a[0] = 20.0; a[1] = 20.0;
  printf("%f\n", dot_prod(weight, a) - theta);
  a[0] = 10.0; a[1] = 10.0;
  printf("%f\n", dot_prod(weight, a) - theta);

  return 0;
}


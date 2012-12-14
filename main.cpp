#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <exception>
#include <string>
#include <cstdio>
#include <boost/program_options.hpp>
#include "QuadProg++.hh"
#include "svm.h"

namespace po = boost::program_options;

void read_input(std::istream& ifs, Matrix<double>& x, Vector<double>& y)
{
  std::vector<std::vector<double> > _x;
  std::vector<double> _y;

  std::string line;
  while (ifs && getline(ifs, line)) {
    std::istringstream is(line);
    std::vector<double> v; double val;
    
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

// scaling x to [s,t]
Matrix<double> scaling(Matrix<double> x, double s, double t)
{
  double m0 = min(min(x)), m1 = max(max(x));
  return x * (t - s) / (m1 - m0);
}

double cross_validation(Matrix<double> x, Vector<double> y, Kernel *k, double c, int cross = -1)
{
  // 交差検定
  int group = cross;
  if (group <= 0) group = 1 + log2(x.nrows());
  int size = x.nrows() / group;
  double accuracy = 0.0;
  for (int i = 0; i < group; i++) {
    int si = i * size, sz = size + (i == group - 1 ? (x.nrows() % size) : 0);
    Matrix<double> samplex(x.nrows() - sz, x.ncols()), testx(sz, x.ncols());
    Vector<double> sampley(y.size() - sz), testy(sz);
    for (int j = 0; j < x.nrows(); j++) {
      if (j < si) {
        samplex.setRow(j, x.extractRow(j));
        sampley[j] = y[j];
      }
      else if (j < si + sz) {
        testx.setRow(j - si, x.extractRow(j));
        testy[j - si] = y[j];
      }
      else {
        samplex.setRow(j - sz, x.extractRow(j));
        sampley[j - sz] = y[j];
      }
    }
    SVM svm(samplex, sampley, k, c);
    int pass = 0;
    for (int j = 0; j < sz; j++)
      if (svm.discriminant(testx.extractRow(j)) * testy[j] > 0.0) pass++;
    double acc = pass / (double)sz * 100.0;
    printf("loop #%d :: %f\n", i, acc);
    accuracy += acc / group;
  }
  printf("total accuracy :: %f\n", accuracy);
  return accuracy;
}

int main(int argc, char *const argv[])
{
  po::positional_options_description p;
  po::options_description opt("options");

  p.add("input-file", -1);
  
  opt.add_options()
    ("help,h", "show help")
    ("input-file", po::value<std::string>(), "input file")
    ("dotprod,d", "use dotprod (default, no parameter)")
    ("polynomial,p",
     po::value<double>(),
     "use polynomial kernel (one parameter)")
    ("gaussian,g",
     po::value<double>(),
     "use gaussian kernel (one parameter)")
    ("hyperbolic,y",
     po::value<std::vector<double> >()->multitoken(),
     "use hyperbolic kernel (two parameters)")
    ("penalty,C",
     po::value<double>(),
     "soft margin svm parameter")
    ("dump-alpha",
     "dump alpha values")
    ("graph",
     "output graph data")
    ("validation",
     po::value<int>()->implicit_value(-1),
     "evaluate SVM by cross validation")
    ("scaling,s",
     po::value<std::vector<double> >()->multitoken(),
     "scaling input data")
    ("dump-scaled-data",
     "output scaled input data");

  po::variables_map argmap;
  try {
    po::store(po::command_line_parser(argc, argv)
              .options(opt).positional(p).run(), argmap);
  } catch (std::exception& ex) {
    std::cerr << "invalid option: " << ex.what() << std::endl;
    std::cerr << opt << std::endl;
    return 1;
  }
  po::notify(argmap);

  if (argmap.count("help") || !argmap.count("input-file")) {
    std::cerr << "usage: ./main filename options" << std::endl;
    std::cerr << opt << std::endl;
    return 1;
  }

  if (argmap.count("dotprod") + argmap.count("polynomial") + argmap.count("gaussian") + argmap.count("hyperbolic") > 1) {
    std::cerr << "too many kernels specified" << std::endl;
    std::cerr << opt << std::endl;
    return 1;
  }

  std::ifstream ifs(argmap["input-file"].as<std::string>().c_str());
  Matrix<double> x;
  Vector<double> y;
  read_input(ifs, x, y);

  if (argmap.count("scaling")) {
    std::vector<double> v = argmap["scaling"].as<std::vector<double> >();
    if (v.size() != 2) {
      std::cerr << "just 2 parameteres required for scaling" << std::endl << opt << std::endl;
      return 1;
    }
    x = scaling(x, std::min(v[0], v[1]), std::max(v[0], v[1]));
    if (argmap.count("dump-scaled-data")) {
      for (int i = 0; i < x.nrows(); i++) {
        for (int j = 0; j < x.ncols(); j++)
          printf("%f ", x.extractRow(i)[j]);
        printf("%f\n", y[i]);
      }
    }
  }

  Kernel *k;

  if (argmap.count("polynomial"))
    k = new Polynomial(argmap["polynomial"].as<double>());
  else if (argmap.count("gaussian"))
    k = new Gaussian(argmap["gaussian"].as<double>());
  else if (argmap.count("hyperbolic")) {
    std::vector<double> v = argmap["hyperbolic"].as<std::vector<double> >();
    if (v.size() != 2) {
      std::cerr << "just 2 parameteres are required for the hyperbolic kernel" << std::endl << opt << std::endl;
      return 1;
    }
    k = new Hyperbolic(v[0], v[1]);
  }
  else k = new DotProd();

  double c = -1.0;
  if (argmap.count("penalty")) c = argmap["penalty"].as<double>();

  if (argmap.count("dump-alpha")) {
    SVM svm(x, y, k, c);
    svm.dump_alpha();
  }

  if (argmap.count("graph")) {
    if (x.ncols() > 2) {
      std::cerr << "グラフは二次元データだけ" << std::endl;
      return 1;
    }
    SVM svm(x, y, k, c);
    double
      x0 = min(x.extractColumn(0)), x1 = max(x.extractColumn(0)),
      y0 = min(x.extractColumn(1)), y1 = max(x.extractColumn(1));
    double mgn = std::max(x1 - x0, y1 - y0) * 0.1;
    int total = 50;
    double delta = std::max(x1 - x0, y1 - y0) / total;
    for (double ay = y0 - mgn; ay <= y1 + mgn; ay += delta) {
      for (double ax = x0 - mgn; ax <= x1 + mgn; ax += delta) {
        Vector<double> v(2); v[0] = ax; v[1] = ay;
        printf("%f %f %f\n", ax, ay, svm.discriminant(v));
      }
      puts("");
    }
  }

  if (argmap.count("validation"))
    cross_validation(x, y, k, c, argmap["validation"].as<int>());
  
  return 0;
}


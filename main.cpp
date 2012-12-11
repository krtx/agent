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
  po::positional_options_description p;
  po::options_description opt("option");

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
     "soft margin svm parameter");

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

  Kernel *k;

  if (argmap.count("polynomial"))
    k = new Polynomial(argmap["polynomial"].as<double>());
  else if (argmap.count("gaussian"))
    k = new Gaussian(argmap["gaussian"].as<double>());
  else if (argmap.count("hyperbolic")) {
    std::vector<double> v = argmap["hyperbolic"].as<std::vector<double> >();
    k = new Hyperbolic(v[0], v[1]);
  }
  else k = new DotProd();

  double c = -1.0;
  if (argmap.count("penalty")) c = argmap["penalty"].as<double>();

  SVM svm(x, y, k, c);

  //svm.dump_alpha();

  double
    x0 = min(x.extractColumn(0)) - 5.0, x1 = max(x.extractColumn(0)) + 5.0,
    y0 = min(x.extractColumn(1)) - 5.0, y1 = max(x.extractColumn(1)) + 5.0;
  int total = 50;
  double delta = std::max((x1 - x0) / total, (y1 - y0) / total);
  for (double ay = y0; ay <= y1; ay += delta) {
    for (double ax = x0; ax <= x1; ax += delta) {
      Vector<double> v(2); v[0] = ax; v[1] = ay;
      printf("%f %f %f\n", ax, ay, svm.discriminant(v));
    }
    puts("");
  }
  
  return 0;
}


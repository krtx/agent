
#ifndef KERNEL_H
#define KERNEL_H

#include "QuadProg++.hh"
#include <cmath>

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

#endif

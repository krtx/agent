
#ifndef KERNEL_H
#define KERNEL_H

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

#endif

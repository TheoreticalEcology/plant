// -*-c++-*-
#ifndef TREE_STRATEGY_H_
#define TREE_STRATEGY_H_

#include <Rcpp.h>
#include <string>

#include "control.h"

#include "lookup.h"
#include "util.h"
#include "integration.h"
#include "interpolator.h"

namespace model {

class Strategy : public util::Lookup {
public:
  typedef util::PtrWrapper<Strategy> ptr;
  Strategy();
  Strategy(Rcpp::List x);

  // Parameters needs to be able to set our control properties:
  void set_control(Control x);
  // And R will want to query it:
  const Control& get_control() const;
  Control r_control() const;

  // Get the interpolator, where it exists.
  interpolator::Interpolator r_assimilation_fn() const;
  void r_set_assimilation_fn(interpolator::Interpolator x);

  Strategy r_copy() const;
  // Not sure about the need to allow this to be accessed, but needed
  // for a test.
  integration::QAG r_integrator() const {return integrator;}

  // All the rest of the class can be accessed only by Plant.
  friend class Plant;

private:
  // * Core traits
  double lma, rho, hmat, s, n_area;

  // * Deafult values for core traits
  double lma_0, rho_0, hmat_0, s_0, n_area_0;

  // * Individual allometry
  // Canopy shape parameters
  double eta, eta_c;
  // Leaf area per sapwood area
  double theta;
  // Empirical constants for scaling relationships
  double a1, B1, a3, k_l0, B4, k_s0, B5;
  // Bark area per sapwood area
  double b;

  // * Production
  // Respiration constants
  double c_Rs, c_Rb, c_Rr, c_Rl;
  // Yield = carbon fixed in tissue per carbon assimilated;
  double Y;
  // Conversion factor
  double c_bio;
  // Leaf, bark sapwood, and root turnover rates
  double k_l, k_b, k_s, k_r;
  // Leaf productivity parameters  - only used when no N reallocation
  double c_p1, c_p2;

  // * Seed production
  // Accessory cost of reproduction - multiplication factor
  double c_acc;
  // Scaling of seed accessory costs with seed mass
  double B7;
  // Proportion production alloctaed to reproduction
  double c_r1;
  // Size range across which individuals mature
  double c_r2;

  // * Mortality
  // Parameter for seedling mortality
  double c_s0;
  // Baseline structural mortality rate
  double c_d0;
  // Coeffcieint for wood density in mortality function
  double c_d1;
  // Coeffcieint for height in mortality function
  double B6;
  // Baseline for growth mortality rate
  double c_d2;
  // Coefficient for dry mass production in mortality function
  double c_d3;

  // Height of a (germinated) seed
  double height_0;

  Control control;

  // See issue #15's comments for commentary on this member:
  integration::QAG integrator;
  // This is optionally used
  interpolator::Interpolator assimilation_fn;

  double assimilation_fn_lookup(double h) const;

  // These things are really not to be used by anything, but are all
  // harmless (except for reset, actually).  They're used in
  // construction, and in mantaining the lookup table used to enable
  // {get,set}_parameters().
  void do_build_lookup();
  void reset();
  void set_parameters_post_hook();
  bool validate_parameters(Rcpp::List x) const;

  static integration::QAG integrator_from_control(const Control& control);
};

}

RCPP_EXPOSED_CLASS_NODECL(model::Strategy)

#endif
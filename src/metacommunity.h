// -*-c++-*-
#ifndef TREE_METACOMMUNITY_
#define TREE_METACOMMUNITY_

#include <Rcpp.h>
#include "patch.h"

namespace model {

class MetacommunityBase : public ode::OdeTarget {
public:
  virtual size_t size() const = 0;
  virtual double r_age() const = 0;
  virtual void step() = 0;
  virtual void step_deterministic() = 0;
  virtual void step_stochastic() = 0;
  virtual std::vector<int> births() = 0;
  virtual void deaths() = 0;
  virtual void add_seeds(std::vector<int> seeds) = 0;
  virtual Rcpp::List r_get_patches() const = 0;
  virtual void r_add_seedlings(Rcpp::IntegerMatrix seeds) = 0;
  virtual Rcpp::IntegerMatrix r_n_individuals() const = 0;
  virtual void r_clear() = 0;
  virtual void r_step() = 0;
  virtual void r_step_stochastic() = 0;
};

template <class Individual> 
class Metacommunity : public MetacommunityBase {
public:
  Metacommunity(Parameters p);
  Metacommunity(Parameters *p);
  
  // * Simple interrogation:
  size_t size() const;
  double r_age()  const;

  // * Main simulation control
  void step();
  void step_deterministic();
  void step_stochastic();

  // * Lower level parts of the main simulation
  std::vector<int> births();
  void deaths();
  void add_seeds(std::vector<int> seeds);

  // * ODE interface
  void derivs(double time, ode::iter_const y, ode::iter dydt);
  size_t ode_size() const;
  ode::iter_const ode_values_set(ode::iter_const it);
  ode::iter       ode_values(ode::iter it) const;
  ode::iter       ode_rates(ode::iter it)  const;

  // * R interface
  Patch<Individual> r_at(size_t idx) const;
  Rcpp::List r_get_patches() const;
  void r_add_seedlings(Rcpp::IntegerMatrix seeds);
  Rcpp::IntegerMatrix r_n_individuals() const;
  void r_clear();
  void r_step();
  void r_step_stochastic();
private:
  void initialise();
  size_t n_species() const {return parameters->size(); }
  
  Parameters::ptr parameters;
  std::vector< Patch<Individual> > patches;
  double age;
  ode::Solver< Metacommunity > ode_solver;

  typedef typename std::vector< Patch<Individual> >::iterator 
  patch_iterator;
  typedef typename std::vector< Patch<Individual> >::const_iterator
  patch_const_iterator;
};

template <class Individual>
Metacommunity<Individual>::Metacommunity(Parameters p)
  : parameters(p),
    age(0.0),
    ode_solver(this) {
  initialise();
}

template <class Individual>
Metacommunity<Individual>::Metacommunity(Parameters *p)
  : parameters(p),
    age(0.0),
    ode_solver(this) {
  initialise();
}

template <class Individual>
size_t Metacommunity<Individual>::size() const {
  return patches.size();
}

template <class Individual>
double Metacommunity<Individual>::r_age() const {
  return age;
}

template <class Individual>
void Metacommunity<Individual>::step() {
  step_deterministic();
  step_stochastic();
}

template <class Individual>
void Metacommunity<Individual>::step_deterministic() {
  std::vector<double> y(ode_size());
  ode_values(y.begin());
  ode_solver.set_state(y, age);
  ode_solver.step();
  age = ode_solver.get_time();
}

template <class Individual>
void Metacommunity<Individual>::step_stochastic() {
  deaths();
  add_seeds(births());
}

template <class Individual>
std::vector<int> Metacommunity<Individual>::births() {
  std::vector<int> n(size(), 0);
  for ( patch_iterator patch = patches.begin(); 
	patch != patches.end(); patch++ ) {
    n = util::sum(n, patch->births());
  }
  return n;
}

template <class Individual>
void Metacommunity<Individual>::deaths() {
  for ( patch_iterator patch = patches.begin(); 
	patch != patches.end(); patch++ )
    patch->deaths();
}

template <class Individual>
void Metacommunity<Individual>::add_seeds(std::vector<int> seeds) {
  size_t j = size();
  for ( patch_iterator patch = patches.begin();
  	patch != patches.end(); patch++ ) {
    std::vector<int> seeds_i(size());
    double p = 1.0/(double)j;
    for ( size_t i = 0; i < size(); i++, j-- ) {
      const int k = (int)R::rbinom(seeds[i], p);
      seeds[i] -= k;
      seeds_i[i] = k;
    }
    patch->add_seeds(seeds_i);
  }
}

// * ODE interace.
template <class Individual>
void Metacommunity<Individual>::derivs(double time, ode::iter_const y,
				       ode::iter dydt) {
  ode_values_set(y);
  ode_rates(dydt);
}

template <class Individual>
size_t Metacommunity<Individual>::ode_size() const {
  size_t ret = 0;
  for ( patch_const_iterator patch = patches.begin();
	patch != patches.end(); patch++ )
    ret += patch->ode_size();
  return ret;
}

template <class Individual>
ode::iter_const Metacommunity<Individual>::ode_values_set(ode::iter_const it) {
  for ( patch_iterator patch = patches.begin();
	patch != patches.end(); patch++ )
    it = patch->ode_values_set(it);
  return it;
}

template <class Individual>
ode::iter Metacommunity<Individual>::ode_values(ode::iter it) const {
  for ( patch_const_iterator patch = patches.begin(); 
	patch != patches.end(); patch++ )
    it = patch->ode_values(it);
  return it;
}

template <class Individual>
ode::iter Metacommunity<Individual>::ode_rates(ode::iter it) const {
  for ( patch_const_iterator patch = patches.begin(); 
	patch != patches.end(); patch++ )
    it = patch->ode_rates(it);
  return it;
}

template <class Individual>
Patch<Individual> Metacommunity<Individual>::r_at(size_t idx) const {
  return patches.at(util::check_bounds_r(idx, size()));
}

template <class Individual>
Rcpp::List Metacommunity<Individual>::r_get_patches() const {
  Rcpp::List ret;
  for ( patch_const_iterator it = patches.begin();
	it != patches.end(); it++ )
    ret.push_back(Rcpp::wrap(*it));
  return ret;
}

// Each column is a patch, each row a species.
template <class Individual>
void Metacommunity<Individual>::r_add_seedlings(Rcpp::IntegerMatrix seeds) {
  util::check_length((size_t)seeds.ncol(), size());
  util::check_length((size_t)seeds.nrow(), n_species());

  for ( size_t i = 0; i < size(); i++ ) {
    Rcpp::IntegerMatrix::Column seeds_col_i = seeds(Rcpp::_, i);
    std::vector<int> seeds_i(seeds_col_i.begin(), seeds_col_i.end());
    patches[i].add_seedlings(seeds_i);
  }
}

template <class Individual>
Rcpp::IntegerMatrix Metacommunity<Individual>::r_n_individuals() const {
  Rcpp::IntegerMatrix ret(n_species(), size());
  Rcpp::IntegerMatrix::const_iterator it = ret.begin();

  for ( patch_const_iterator p = patches.begin();
	p != patches.end(); p++ ) {
    std::vector<int> n = p->r_n_individuals();
    it = std::copy(n.begin(), n.end(), it);
  }

  return ret;
}

template <class Individual>
void Metacommunity<Individual>::r_clear() {
  age = 0.0;
  for ( patch_iterator it = patches.begin();
	it != patches.end(); it++ )
    it->r_clear();
  ode_solver.reset();
}

template <class Individual>
void Metacommunity<Individual>::r_step() {
  Rcpp::RNGScope scope;
  step();
}

template <class Individual>
void Metacommunity<Individual>::r_step_stochastic() {
  Rcpp::RNGScope scope;
  step_stochastic();
}

// Possibly can do 
//   patches.clear();
//   Patch p(parameters);
//   patches.resize(n_patches, p);
template <class Individual>
void Metacommunity<Individual>::initialise() {
  patches.clear();
  for ( int i = 0; i < parameters->n_patches; i++ ) {
    Patch<Individual> p(parameters.get());
    patches.push_back(p);
  }
}

}

#endif

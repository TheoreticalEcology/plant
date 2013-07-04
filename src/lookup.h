// -*-c++-*-
#ifndef TREE_LOOKUP_H_
#define TREE_LOOKUP_H_

#include <string>
#include <map>
#include <Rcpp.h>

// This is a weird class, partly because the way it is used is weird.
// 
// We have a class with an absolute ton of "double" members
// (`Strategy`); these represent model parameters and we want to be
// able to access them easily from within the `Plant` class (`Plant`
// has a member `Strategy *strategy` so we can do `strategy->n_area`
// to get concentration of leaf nitrogen by area).
// 
// However, we need to be able to get and set these parameters from R,
// but I don't want to write a thousand small interfaces -- even with
// Rcpp modules that's going to be a pain.  I want to be able to do
// get_parameters() and have a list of parameters (in this case, an R
// list) and similarly do set_parameters(x) for some list x and set a
// subset of parameters. 
// 
// That's what this class does.  Inherit from it privately and define
// the method `do_build_lookup()`.  This ends up with lines that look
// like this:
//   lookup_table["foo"] = &foo;
// where "foo" is the name of the element in the R list and &foo is
// the address of the member (which *must* be a double).
// 
// We do the lookup table building on get or set of parameters,
// because these are never time sensitive (wheras construction or copy
// might be).  It is also partly necessary to stop the abstracton
// becoming leaky, as the base constructor is called before the
// derived class (so we no nothing of the look up to be built).  
// 
// Lookup construction is still kept to a minimum by keeping a pointer
// to the address of the object in `addr`.  This is set to NULL on
// construction so that `addr != this` forcing a build of the table.
// Likewise, on copy, `addr != this`, because it points at the object
// that was the *source* object, forcing a rebuild (similarly for
// assignment).  These rebuilds are deferred until the lookup table is
// used (through get_parameters() and set_parameters()) so will be
// skipped for temporary copies created by pass-by-value, etc.
//
// In addition, derived classes can optionally provide a function
// set_parameters_post_hook(), which will be run after
// set_parameters().  This is used by `Strategy` to compute some
// constants.  At present, no information is available on which
// parameters changed, so if this is an expensive and frequent
// operation this might not be optimal.

namespace util {

class Lookup {
public:
  Lookup();
  Rcpp::List get_parameters();
  void set_parameters(Rcpp::List x);
  bool has_key(std::string key) const;
  virtual ~Lookup() {}

protected:
  typedef std::map<std::string, double*> lookup_type;
  lookup_type lookup_table;
  virtual void do_build_lookup() = 0;
  virtual void set_parameters_post_hook();

private:
  void build_lookup();
  double* lookup(std::string key) const;
  void check_keys(std::vector<std::string> keys) const;
  Lookup* addr; // used to detect copy and trigger lookup table rebuild
};

}

#endif

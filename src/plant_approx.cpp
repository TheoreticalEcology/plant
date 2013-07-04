#include "plant_approx.h"

namespace model {

PlantApprox::PlantApprox(Strategy s, PlantSpline ps)
  : Plant(s),
    plant_spline(ps) {
}

PlantApprox::PlantApprox(Strategy *s, PlantSpline *ps)
  : Plant(s),
    plant_spline(ps) {
}

void PlantApprox::compute_vars_phys(const Environment& environment) {
  if (large_plant_do_exact())
    Plant::compute_vars_phys(environment);
}

ode::iterator PlantApprox::ode_rates(ode::iterator it) const {
  if (large_plant_do_exact())
    return Plant::ode_rates(it);
  else
    return plant_spline->ode_rates(height(), it);
}

void PlantApprox::r_compute_vars_phys_spline(const Environment& environment) {
  plant_spline->compute_vars_phys(environment);
}

bool PlantApprox::large_plant_do_exact() const {
  return height() > plant_spline->height_max();
}

}

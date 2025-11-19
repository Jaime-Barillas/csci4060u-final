#pragma once

#include "sim_opts.h"
#include "particles.h"
#include "neighbours.h"
#include <libcommon/vec.h>

namespace particles {
  // Kernel Functions.
  struct PolyKernel      { using return_type = float; };
  struct SpikyGradKernel { using return_type = Vec3; };
  struct ViscLaplKernel  { using return_type = float; };

  template<typename T>
  concept Kernel = std::same_as<T, PolyKernel> ||
                   std::same_as<T, SpikyGradKernel> ||
                   std::same_as<T, ViscLaplKernel>;

  template<typename T>
  requires Kernel<T>
  typename T::return_type kernel(Vec3 &pos, Vec3 &particle);

  // Force Computation Functions.
  void calculate_density_pressure(Particles &ps, Neighbours &ns, const SimOpts &opts);
  void calculate_pressure_forces(Particles &ps, Neighbours &ns, const SimOpts &opts);
  void calculate_viscosity_forces(Particles &ps, Neighbours &ns, const SimOpts &opts);
  void calculate_external_forces(Particles &ps);
  void integrate(Particles &ps);
}

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
  typename T::return_type kernel(
    float point_x,
    float point_y,
    float point_z,
    float particle_x,
    float particle_y,
    float particle_z
  );

  // Force Computation Functions.
  void calculate_density_pressure(Particles &input, Particles &output, Neighbours &ns, const SimOpts &opts);
  void calculate_pressure_forces(Particles &input, Particles &output, Neighbours &ns, const SimOpts &opts);
  void calculate_viscosity_forces(Particles &input, Particles &output, Neighbours &ns, const SimOpts &opts);
  void calculate_external_forces(Particles &input, Particles &output);
  void integrate(Particles &output);
}

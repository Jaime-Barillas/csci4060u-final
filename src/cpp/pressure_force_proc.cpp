#include "pressure_force_proc.h"

#include "neighbour_proc.h"
#include "particles.h"
#include "util.h"
#include <libcommon/vec.h>

Vec3 PressureForceProc::spiky_kernel(Vec3 &point, Vec3 &particle) const {
  static constexpr float COEFFICIENT = -45.0f / (std::numbers::pi_v<float> * util::pow(SUPPORT, 6));

  Vec3 difference = particle - point;
  float dist = difference.length();
  float q = SUPPORT - dist;

  if (q < 0 || dist <= 0) {
    return { 0, 0, 0 };
  }

  q = q * q * COEFFICIENT;

  // Manually normalizing (div by dist) avoids an extra sqrt().
  difference *= q * (1.0f / dist);
  return difference;
}

void PressureForceProc::process(Particles &ps, Neighbours &ns, const SimOpts &opts) const {
  // FIXME: Something is wrong with the calculation.
  //        Particles tend to get 'sucked' into each other.
  //        Try smaller timesteps ?
  size_t particle_count = opts.particle_count;
  Vec3 pressure_kernel_temp;

  for (size_t i = 0; i < particle_count; i++) {
    Vec3 pressure_temp{ 0, 0, 0 };
    Particles *neighbours = ns.neighbours_near(ps.pos[i], opts);

    for (size_t j = 0; j < neighbours->size(); j++) {
      pressure_kernel_temp = spiky_kernel(ps.pos[i], neighbours->pos[j]);
      float pressure_factor = (ps.pressure[i] + neighbours->pressure[j]) / (2 * neighbours->density[j]);
      pressure_kernel_temp *= pressure_factor;
      pressure_temp += pressure_kernel_temp;
    }

    ps.pforce[i] = pressure_temp;
    ns.return_neighbour_list(neighbours);
  }
}

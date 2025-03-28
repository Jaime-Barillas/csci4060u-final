# CSCI4060U Final

1. Initialize particles
2. Sort into bins.
   + Bin size based on interaction radius.
3. Simulate:
   1. Apply "gravity".
   2. Apply viscosity impulses.
   3. Advance to predicted position.
   4. Update springs (Optional).
   5. Apply spring displacements (Optional).
   6. Apply Double Density Relaxation.
   7. Resolve collisions.
   8. Compute next velocity.

---

Project: Basic _parallelized_ particle based fluid simulator with OpenMP.
Parallelism in: particle calculations.
Plan:
* The are many for loops in the code. Use `parallel for`.
  * Potentially `parallel for collapse`.
* Try manually batching the particle computations.
  * With use of critical sections or other OpenMP features.
* Look into OpenMP SIMD directives.
* Compare timings for variants.

Stretch Goal:
* [Pony](https://www.ponylang.io/) implementation.
  * Actor model.
  * Type level "safety" annotations (reference capabilities)
  * Report on timings, ease of implementation, or benefit of using Pony.

NOTE: On fedora, install for SDL3:
+ libudev-devel (should be present outside docker container.)
+ dbus-devel (should be present outside docker container.)
+ pipewire-devel (should be present outside docker container.)
+ wayland-devel
+ libxkbcommon-devel
+ libglvnd-devel

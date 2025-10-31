#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <particles.h>

class RandomVec3 final : public Catch::Generators::IGenerator<particles::Vec3> {
  Catch::Generators::GeneratorWrapper<float> float_gen;
  particles::Vec3 pos;

  public:
    RandomVec3(float low, float high):float_gen{Catch::Generators::random(low, high)} {
        pos.x = float_gen.get();
        float_gen.next();
        pos.y = float_gen.get();
        float_gen.next();
        pos.z = float_gen.get();
        float_gen.next();
      }

      bool next() override {
        pos.x = float_gen.get();
        float_gen.next();
        pos.y = float_gen.get();
        float_gen.next();
        pos.z = float_gen.get();
        return float_gen.next();
      }

      particles::Vec3 const& get() const override {
        return pos;
      }
};

Catch::Generators::GeneratorWrapper<particles::Vec3> random_Vec3(float low, float high) {
  return Catch::Generators::GeneratorWrapper<particles::Vec3>(
    new RandomVec3(low, high)
  );
}

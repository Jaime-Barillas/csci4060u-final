#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <libcommon/vec.h>

class RandomVec3 final : public Catch::Generators::IGenerator<Vec3> {
  Catch::Generators::GeneratorWrapper<float> float_gen;
  Vec3 pos;

  public:
    RandomVec3(float low, float high)
    : float_gen{Catch::Generators::random(low, high)} {
        pos.data[0] = float_gen.get();
        float_gen.next();
        pos.data[1] = float_gen.get();
        float_gen.next();
        pos.data[2] = float_gen.get();
        float_gen.next();
      }

      bool next() override {
        pos.data[0] = float_gen.get();
        float_gen.next();
        pos.data[1] = float_gen.get();
        float_gen.next();
        pos.data[2] = float_gen.get();
        return float_gen.next();
      }

      Vec3 const& get() const override {
        return pos;
      }
};

Catch::Generators::GeneratorWrapper<Vec3> random_Vec3(float low, float high) {
  return Catch::Generators::GeneratorWrapper<Vec3>(
    new RandomVec3(low, high)
  );
}

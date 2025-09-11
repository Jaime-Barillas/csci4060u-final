use "collections"

actor PM
  let _main: Main
  let _particles: Array[Particle ref]

  new create(main': Main) =>
    _main = main'

    _particles = Array[Particle ref](10 * 10 * 10)
    for x in Range[I32](-5, 5) do
      for y in Range[I32](-5, 5) do
        for z in Range[I32](-5, 5) do
          _particles.push(
            recover ref Particle(
              where
              pos' = Vec3(x.f32() / 10.0, y.f32() / 10.0, z.f32() / 10.0),
              vel' = Vec3.zero(),
              density' = 0.0,
              pressure' = 0.0
            ) end
          )
        end
      end
    end

  be update() =>
    try
      for i in Range(0, 1000) do
        _particles(i)?.pos.x = _particles(i)?.pos.x + 0.02
      end
    end
    _main.draw(_particles.cpointer())

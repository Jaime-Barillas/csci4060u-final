struct Particle
  embed pos: Vec3
  embed vel: Vec3
  var density: F32
  var pressure: F32

  new create(pos': Vec3, vel': Vec3, density': F32, pressure': F32) =>
    pos = Vec3(pos'.x, pos'.y, pos'.z)
    vel = Vec3(pos'.x, pos'.y, pos'.z)
    density = density'
    pressure = pressure'

struct Vec3
  var x: F32
  var y: F32
  var z: F32

  new create(x': F32, y': F32, z': F32) =>
    x = x'
    y = y'
    z = z'

  new zero() =>
    x = 0
    y = 0
    z = 0

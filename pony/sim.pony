use "debug"
use "collections" // Range
use "runtime_info"
use "fork_join"

use @printf[None](s: Pointer[U8] tag, ...)

primitive SC
  fun particle_size(): I32 => 8
  fun particle_mass(): F32 => 2.5
  fun bound_damping(): F32 => -0.5
  fun support_radius(): F32 => 16.0
  fun gas_constant(): F32 => 2000.0
  fun near_gas_constant(): F32 => 30000.0
  fun rest_density(): F32 => 300.0
  fun viscosity(): F32 => 200.0
  fun poly6(): F32 => 2.96449182726e-10
  // fun poly6(): F32 => 2.96449182726
  fun spiky(): F32 => 0.00000303563963112
  fun visc(): F32 => 0.0000121425585245

primitive Util
  fun iso_copy(arr: Array[Particle] box): Array[Particle] iso^ =>
    let iso_arr: Array[Particle] iso = Array[Particle](arr.size())
    for p in arr.values() do
      iso_arr.push(Particle.copy(p))
    end
    consume iso_arr
    // recover iso
    //   let iso_arr = Array[Particle](arr.size())
    //   for p in arr.values() do
    //     iso_arr.push(p.copy())
    //   end
    //   iso_arr
    // end

type ParticleBatch is (Array[Particle] iso, Array[Particle] val)
type UpdatedParticles is Array[Particle] iso

class val Particle
  var x: F32 = 0
  var y: F32 = 0
  var vx: F32 = 0
  var vy: F32 = 0
  var density: F32 = 0
  var pressure: F32 = 0

  new val create(x': F32, y': F32, vx': F32, vy': F32, d': F32, p': F32) =>
    x = x'
    y = y'
    vx = vx'
    vy = vy'
    density = d'
    pressure = p'

  new iso copy(other: Particle val) =>
    x = other.x
    y = other.y
    vx = other.vx
    vy = other.vy
    density = other.density
    pressure = other.pressure

  fun box distance_to(other: Particle box): F32 =>
    let dx = other.x - x
    let dy = other.y - y
    ((dx * dx) + (dy * dy)).sqrt()

actor Sim
  let _main: Main
  let _bound_x: F32
  let _bound_y: F32
  var _pcount: USize = 400
  var _time_step: F32 = 60.0
  var _sim_steps: I32 = 2
  var _gravity_y: F32 = -9.8

  let _sched_auth: SchedulerInfoAuth
  var _old_ps: Array[Particle] val
  var _ps: Array[Particle] iso

  new create(main: Main,
             bound_x: F32,
             bound_y: F32,
             auth: SchedulerInfoAuth) =>
    _main = main
    _bound_x = bound_x
    _bound_y = bound_y
    _sched_auth = auth
    _old_ps = Array[Particle]
    _ps = Array[Particle]

  be set_pcount(value: USize) => _pcount = value
  be set_time_step(value: F32) => _time_step = value
  be set_sim_steps(value: I32) => _sim_steps = value
  be set_gravity_y(value: F32) => _gravity_y = value

  // Calculate densities and pressures
  be simulate(old_ps: Array[Particle] val) =>
    // We can't modify val arrays so we make an iso copy to use with the
    // fork_join library.
    _old_ps = old_ps
    let new_ps = Util.iso_copy(old_ps)

    let job = Job[ParticleBatch, UpdatedParticles](
      ParticleWorkerBuilder(1.0 / _time_step),
      ParticleBatcher((consume new_ps, old_ps)),
      ArrayCollector(this, old_ps.size()),
      _sched_auth
    )

    job.start()

  // Calculate forces and integrate
  be continue_sim(ps_with_densities: UpdatedParticles) =>
    _old_ps = consume ps_with_densities
    let new_ps = Util.iso_copy(_old_ps)

    let job = Job[ParticleBatch, UpdatedParticles](
      ParticleWorkerBuilder2(1.0 / _time_step),
      ParticleBatcher((consume new_ps, _old_ps)),
      ArrayCollector2(this, _old_ps.size()),
      _sched_auth
    )

    job.start()

  be finish_sim(new_ps: UpdatedParticles) => _main.draw(consume new_ps)

class ParticleWorker is Worker[ParticleBatch, UpdatedParticles]
  let _dt: F32
  var _batch: Array[Particle] iso = Array[Particle]
  var _neighbours: Array[Particle] val = Array[Particle]

  var debug: Bool = true

  new iso create(dt: F32) =>
    _dt = dt

  fun ref receive(work_set: ParticleBatch) =>
    (_batch, _neighbours) = consume work_set

  fun ref process(runner: WorkerRunner[ParticleBatch, UpdatedParticles] ref) =>
    let sr = SC.support_radius()
    let mass = SC.particle_mass()
    let poly6 = SC.poly6()
    let gas_const = SC.gas_constant()
    let rest_density = SC.rest_density()

    for i in Range(0, _batch.size()) do
      try
        var density: F32 = 0
        var pressure: F32 = 0
        let p = _batch(i)?

        for j in Range(0, _neighbours.size()) do
          let d = p.distance_to(_neighbours(j)?)
          if (d < sr) then
            let r2 = sr * sr
            let d2 = d * d
            density = density + (mass * (poly6) * (r2 - d2).powi(3))
          end
        end

        pressure = gas_const * (density - rest_density)
        let newp = Particle(p.x, p.y, p.vx, p.vy, density, pressure)
        _batch(i)? = newp
      end
    end

    runner.deliver(_batch = Array[Particle])

class ParticleWorkerBuilder is WorkerBuilder[ParticleBatch, UpdatedParticles]
  let _dt: F32

  new create(dt: F32) => _dt = dt
  fun ref apply(): ParticleWorker iso^ => ParticleWorker(_dt)

class iso ArrayCollector is Collector[ParticleBatch, UpdatedParticles]
  let _sim: Sim
  var _ps: Array[Particle]

  new iso create(sim: Sim, total_size: USize) =>
    _sim = sim
    _ps = Array[Particle](total_size)

  fun ref collect(runner: CollectorRunner[ParticleBatch, UpdatedParticles] ref, result: UpdatedParticles) =>
    // TODO: Try unchop()?
    let amount = result.size()
    (consume result).copy_to(_ps, 0, _ps.size(), amount)

  fun ref finish() =>
    let ps: Array[Particle] iso = Util.iso_copy(_ps)
    _sim.continue_sim(consume ps)

class ParticleBatcher is Generator[ParticleBatch]
  var _ps: Array[Particle] iso
  var _neighbours: Array[Particle] val
  var _batch_sizes: Array[USize] = Array[USize]

  new iso create(ps: ParticleBatch) =>
    (_ps, _neighbours) = consume ps

  fun ref init(num_workers: USize) =>
    _batch_sizes = EvenlySplitDataElements(_ps.size(), num_workers)

  fun ref apply(): ParticleBatch^ ? =>
    if _ps.size() == 0 then
      error
    end

    let batch_size = _batch_sizes.shift()?
    (let batch, _ps) = (consume _ps).chop(batch_size)
    (consume batch, _neighbours)

class ParticleWorker2 is Worker[ParticleBatch, UpdatedParticles]
  let _dt: F32
  var _batch: Array[Particle] iso = Array[Particle]
  var _neighbours: Array[Particle] val = Array[Particle]

  var debug: Bool = true

  new iso create(dt: F32) =>
    _dt = dt

  fun ref receive(work_set: ParticleBatch) =>
    (_batch, _neighbours) = consume work_set

  fun ref process(runner: WorkerRunner[ParticleBatch, UpdatedParticles] ref) =>
    let sr = SC.support_radius()
    let mass = SC.particle_mass()
    let viscosity = SC.viscosity()
    let visc = SC.visc()
    let spiky = SC.spiky()

    for i in Range(0, _batch.size()) do
      try
        var pforce_x: F32 = 0
        var pforce_y: F32 = 0
        var vforce_x: F32 = 0
        var vforce_y: F32 = 0
        var gforce_y: F32 = 0
        let p = _batch(i)?

        for j in Range(0, _neighbours.size()) do
          if i == j then continue end

          let pj = _neighbours(j)?
          let d = p.distance_to(pj)
          if (d < sr) then
            var r = sr - d

            // Viscosity
            let rel_visc = ((pj.vx - p.vx) / pj.density)
            let vmult = (mass * visc * viscosity * r)
            vforce_x = vforce_x + (rel_visc * vmult)
            vforce_y = vforce_y + (rel_visc * vmult)

            // Pressures
            r = r.powi(3)
            let rel_p = ((pj.pressure + p.pressure) / (2 * pj.density))
            let pmult = (0.05 * mass * spiky * r)
            pforce_x = pforce_x + (rel_p * pmult)
            pforce_y = pforce_y + (rel_p * pmult)
          end
        end

        gforce_y = gforce_y + ((0.1 * 9.8 * mass) / p.density)
        let force_x = pforce_x + vforce_x
        let force_y = pforce_y + vforce_y + gforce_y

        let accel_x = (force_x / p.density)
        let accel_y = (force_y / p.density)
        var vel_x = p.vx + (accel_x * _dt)
        var vel_y = p.vy + (accel_y * _dt)
        var pos_x = p.x + (vel_x * _dt)
        var pos_y = p.y + (vel_y * _dt)

        if ((pos_x < 0) or (pos_x > 1152)) then
          pos_x = pos_x.min(1152).max(0)
          vel_x = vel_x * SC.bound_damping()
        end
        if ((pos_y < 0) or (pos_y > 720)) then
          pos_y = pos_y.min(1152).max(0)
          vel_y = vel_y * SC.bound_damping()
        end

        let newp = Particle(pos_x, pos_y, vel_x, vel_y, p.density, p.pressure)
        _batch(i)? = newp
      end
    end
    runner.deliver(_batch = Array[Particle])

class ParticleWorkerBuilder2 is WorkerBuilder[ParticleBatch, UpdatedParticles]
  let _dt: F32

  new create(dt: F32) => _dt = dt
  fun ref apply(): ParticleWorker2 iso^ => ParticleWorker2(_dt)

class iso ArrayCollector2 is Collector[ParticleBatch, UpdatedParticles]
  let _sim: Sim
  var _ps: Array[Particle]

  new iso create(sim: Sim, total_size: USize) =>
    _sim = sim
    _ps = Array[Particle](total_size)

  fun ref collect(runner: CollectorRunner[ParticleBatch, UpdatedParticles] ref, result: UpdatedParticles) =>
    // TODO: Try unchop()?
    let amount = result.size()
    (consume result).copy_to(_ps, 0, _ps.size(), amount)

  fun ref finish() =>
    let ps: Array[Particle] iso = Util.iso_copy(_ps)
    _sim.finish_sim(consume ps)

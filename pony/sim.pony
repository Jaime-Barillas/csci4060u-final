use "runtime_info"
use "fork_join"

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

  new val create(x': F32, y': F32) =>
    x = x'
    y = y'

  new iso copy(other: Particle val) =>
    x = other.x
    y = other.y

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

  be simulate(old_ps: Array[Particle] val) =>
    // We can't modify val arrays so we make an iso copy to use with the
    // fork_join library.
    let new_ps = Util.iso_copy(old_ps)

    let job = Job[ParticleBatch, UpdatedParticles](
      ParticleWorkerBuilder,
      ParticleBatcher((consume new_ps, old_ps)),
      ArrayCollector(this, old_ps.size()),
      _sched_auth
    )

    job.start()

  be continue_sim(ps_with_densities: UpdatedParticles) =>
    let new_ps = consume ps_with_densities
    _main.draw(consume new_ps)

class ParticleWorker is Worker[ParticleBatch, UpdatedParticles]
  var _batch: Array[Particle] iso = Array[Particle]
  var _neighbours: Array[Particle] val = Array[Particle]

  fun ref receive(work_set: ParticleBatch) =>
    (_batch, _neighbours) = consume work_set

  fun ref process(runner: WorkerRunner[ParticleBatch, UpdatedParticles] ref) =>
    runner.deliver(_batch = Array[Particle])

class ParticleWorkerBuilder is WorkerBuilder[ParticleBatch, UpdatedParticles]
  fun ref apply(): ParticleWorker iso^ => ParticleWorker

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
    (consume batch, _neighbours = Array[Particle])


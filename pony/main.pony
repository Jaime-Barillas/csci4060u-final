use "debug"
use "actor_pinning"
use "runtime_info"
use "time"

primitive C
  fun height(): I32 => 720
  fun width(): I32 => (C.height() * 16) / 10
  fun renderer_name(): Pointer[U8] tag =>
    ifdef "USE_VULKAN_RENDERER" then
      "vulkan,software".cstring()
    else
      Pointer[U8]
    end

actor Main
  let _env: Env
  let _pin_auth: PinUnpinActorAuth
  let _sim: Sim
  let _ev: SdlEvent = SdlEvent
  let _ui: Ui = Ui
  var _w: SdlWindow = SdlWindow
  var _r: SdlRenderer = SdlRenderer
  var _should_quit: Bool = false
  var _old_time: U64

  new create(env: Env) =>
    let sched_auth = SchedulerInfoAuth(env.root)
    _env = env
    _pin_auth = PinUnpinActorAuth(env.root)
    _sim = Sim(this, C.width().f32(), C.height().f32(), sched_auth)

    // Print the number of _maximum_ scheduler threads.
    let num_schedulers = Scheduler.schedulers(sched_auth)
    _env.out.print("Parallelism Level: " + num_schedulers.string())

    ActorPinning.request_pin(_pin_auth)
    await_pinned_then_init()

    _old_time = Time.micros()

  be await_pinned_then_init() =>
    if not ActorPinning.is_successfully_pinned(_pin_auth) then
      await_pinned_then_init()
      return
    end

    if init_SDL() then
      iterate(reset_particles(_ui.pcount().usize()))
    else
      _env.exitcode(1)
      cleanup()
    end

  be iterate(ps: Array[Particle] val) =>
    if _should_quit then
      cleanup()
      return
    end

    // FIXME: Currently dt includes drawing. It shouldn't.
    let new_time = Time.micros()
    let dt = new_time - _old_time
    _old_time = new_time
    _ui.set_frame_time_sim((dt.f32() / 10e6)) // TODO: Double check

    while @SDL_PollEvent(_ev) != 0 do
      @SDL_ConvertEventToRenderCoordinates(_r, _ev)
      match _ev.event_type()
      | let _: SdlEventQuit => _should_quit = true
      | let _: SdlEventMouseMotion => _ui.update_mouse_pos(_ev.x(), _ev.y())
      | let _: SdlEventMouseButtonDown => _ui.update_mouse_down(_ev.x(), _ev.y())
      | let _: SdlEventMouseButtonUp => _ui.update_mouse_up(_ev.x(), _ev.y())
      end
    end

    _sim.simulate(ps)

  be draw(ps: Array[Particle] val) =>
    let rect = SdlFRect
    let half_psize = SC.particle_size().f32() / 2.0
    rect.w = SC.particle_size().f32()
    rect.h = SC.particle_size().f32()

    @SDL_SetRenderDrawColor(_r, 12, 12, 64, 255)
    @SDL_RenderClear(_r)
    _ui.draw(_r)
    for p in ps.values() do
      rect.x = p.x - half_psize
      rect.y = p.y - half_psize
      @SDL_RenderFillRect(_r, rect)
    end
    @SDL_RenderPresent(_r)

    iterate(ps)

  fun reset_particles(count: USize): Array[Particle] val =>
    let next_multiple = (count.f32() / (16.0 * 10.0)).sqrt().ceil().usize()
    let cols = next_multiple * 16
    let rows = next_multiple * 10
    let pos_step = SC.particle_size().f32() * 1.25

    let ps: Array[Particle] trn = Array[Particle](count)
    let half_x = C.width().f32()
    let half_y = C.height().f32()
    var i: USize = 0
    while i < count do
      let x = ((half_x / 2.0) + ((i % cols).f32() * pos_step)) - ((cols / 2).f32() * pos_step)
      let y = ((half_y / 2.0) + ((i / cols).f32() * pos_step)) - ((rows / 2).f32() * pos_step)
      ps.push(Particle(x, y, 0, 0, 0, 0))

      i = i + 1
    end
    consume ps

  fun ref init_SDL(): Bool =>
    if @SDL_Init(SdlInitVideo()) == 0 then
      let err = String.copy_cstring(@SDL_GetError())
      _env.out.print("Failed to initialize SDL: " + err)
      return false
    end

    let rect = SdlRect
    let display = @SDL_GetPrimaryDisplay()
    @SDL_GetDisplayUsableBounds(display, rect)
    // W/ integer truncation: Get largest multiple of height that fits display.
    let height = (rect.h / C.height()) * C.height()
    let width = (height * 16) / 10  // 16 by 10 best aspect ratio

    _w = @SDL_CreateWindow("Final - CSCI4060U - Pony".cstring(), width, height, 0)
    if _w.is_null() then
      let err = String.copy_cstring(@SDL_GetError())
      _env.out.print("Failed to create window: " + err)
      return false
    end

    _r = @SDL_CreateRenderer(_w, C.renderer_name())
    if _r.is_null() then
      let err = String.copy_cstring(@SDL_GetError())
      _env.out.print("Failed to create renderer: " + err)
      return false
    end

    // Bonus renderer setup. Assume these all succeed...
    // LogicalPresentation -> Scaling
    // VSync -> Quiter operation when not under load.
    @SDL_SetRenderDrawBlendMode(_r, SdlBlendmodeBlend())
    @SDL_SetRenderLogicalPresentation(_r, C.width(), C.height(), SdlLogicalPresentationIntegerScale())
    @SDL_SetRenderVSync(_r, 1)

    true

  fun ref cleanup() =>
    if not _w.is_null() then
      @SDL_DestroyWindow(_w)
    end

    if not _r.is_null() then
      @SDL_DestroyRenderer(_r)
    end

    @SDL_Quit()


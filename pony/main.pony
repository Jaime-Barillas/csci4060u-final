use "actor_pinning"
use "runtime_info"

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
  let _ev: SdlEvent = SdlEvent
  var _w: SdlWindow = SdlWindow
  var _r: SdlRenderer = SdlRenderer
  var should_quit: Bool = false

  new create(env: Env) =>
    _env = env
    _pin_auth = PinUnpinActorAuth(env.root)

    // Print the number of _maximum_ scheduler threads.
    let num_schedulers = Scheduler.schedulers(SchedulerInfoAuth(_env.root))
    _env.out.print("Parallelism Level: " + num_schedulers.string())

    ActorPinning.request_pin(_pin_auth)
    await_pinned_then_init()

  be await_pinned_then_init() =>
    if not ActorPinning.is_successfully_pinned(_pin_auth) then
      await_pinned_then_init()
      return
    end

    if init_SDL() then
      // init_ui()
      // init_sim()
      iterate()
    else
      _env.exitcode(1)
      cleanup()
    end

  be iterate() =>
    if should_quit then
      cleanup()
      return
    end

    while @SDL_PollEvent(_ev) != 0 do
      @SDL_ConvertEventToRenderCoordinates(_r, _ev)
      match _ev.event_type()
      | let _: SdlEventQuit => should_quit = true
      end
    end

    @SDL_SetRenderDrawColor(_r, 12, 12, 64, 255)
    @SDL_RenderClear(_r)
    @SDL_RenderPresent(_r)

    iterate()

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


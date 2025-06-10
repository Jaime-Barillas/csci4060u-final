use "actor_pinning"
use "debug"
use "log"
use "runtime_info"
use "sdl3"

actor Main
  let _log: Logger
  let _pin_auth: PinUnpinActorAuth
  var _w: Window
  var _d: GPUDevice

  new create(env: Env) =>
    _w = Window
    _d = GPUDevice

    let log_target = ConsoleTarget(env.out)
    _log = Logger("Main", log_target)
    _pin_auth = PinUnpinActorAuth(env.root)

    if not SDL.was_init() then
      _log.info(SDL.get_error())
      return
    end

    let thread_count = Scheduler.schedulers(SchedulerInfoAuth(env.root))
    _log.info("Scheduler Threads: " + thread_count.string())

    ActorPinning.request_pin(_pin_auth)
    wait_for_pin()

  be wait_for_pin() =>
    if ActorPinning.is_successfully_pinned(_pin_auth) then
      _log.info("Pinned Main actor")
      setup_and_run()
    else
      wait_for_pin()
    end

  // TODO: Clean up. Perhaps these SDL functions should be partial?
  be setup_and_run() =>
    try
      let display_id = SDL.get_primary_display()
      if display_id == 0 then error end

      let display_bounds: Rect = Rect
      if not SDL.get_display_bounds(display_id, display_bounds) then error end
      _log.info("Display Size: "
                + display_bounds.w.string()
                + ", "
                + display_bounds.h.string())

      let width = (display_bounds.w.i32() * 2) / 3
      let height = (display_bounds.h.i32() * 2) / 3
      _w = SDL.create_window("fluid-sim-sph", width, height, 0)
      if _w.is_null() then error end
      _log.info("Window Size: "
                + width.string()
                + ", "
                + height.string())

      _d = SDL.create_gpu_device(GPUShaderFormat.spirv(), true)
      if _d.is_null() then error end

      if not SDL.claim_window_for_gpu_device(_d, _w) then error end
    else
      _log.err(SDL.get_error())
      return
    end

    run()

  be run() =>
    var running = true
    var event: Event = Event

    while SDL.poll_event(event) do
      match event.kind
      | EventKind.quit() =>
        running = false
        break
      end
    end

    let cmd_buf = SDL.acquire_gpu_command_buffer(_d)
    if cmd_buf.is_null() then
      _log.err(SDL.get_error())
    else
      (let swapchain_texture, let ok) = SDL.wait_and_acquire_gpu_swapchain_texture(cmd_buf, _w)
      if not ok then
        _log.err(SDL.get_error())
        SDL.cancel_gpu_command_buffer(cmd_buf)
      else
        let color_target: GPUColorTargetInfo = GPUColorTargetInfo
        color_target.texture = swapchain_texture
        color_target.clear_color.g = 0.75
        color_target.clear_color.b = 0.5
        color_target.load_op = GPULoadOp.clear()
        color_target.store_op = GPUStoreOp.store()

        let render_pass = SDL.begin_gpu_render_pass(
          cmd_buf,
          color_target,
          1,
          Pointer[None]
        )
        SDL.end_gpu_render_pass(render_pass)
        SDL.submit_gpu_command_buffer(cmd_buf)
      end
    end

    if running then
      run()
    else
      cleanup()
    end

  be cleanup() =>
      SDL.release_window_from_gpu_device(_d, _w)
      SDL.destroy_gpu_device(_d)
      SDL.destroy_window(_w)
      ActorPinning.request_unpin(_pin_auth)

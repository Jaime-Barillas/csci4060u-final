use "lib:SDL3" if not windows // Needed by the wrapper library
use "lib:libSDL3" if windows // TODO: Test on Windows
use "lib:ui" if not windows
use "lib:libui" if windows

use "actor_pinning"
use "files"
use "log"
use "runtime_info"

use @create_ui[U8](exe_path: Pointer[U8] tag)
use @destroy_ui[None]()
use @update_ui[U8]()
use @render_ui[None](particles: Pointer[Particle ref] tag)

actor Main
  let _exe_path: String
  let _log: Logger
  let _pin_auth: PinUnpinActorAuth
  let _pm: PM

  new create(env: Env) =>
    _pm = PM(this)

    // The C code will need to know the path to the exe to load shaders in the
    // sibling "shader" folder.
    _exe_path = try
      Path.dir(env.args(0)?)
    else "."
    end

    let log_target = ConsoleTarget(env.out)
    _log = Logger("Main", log_target)
    _pin_auth = PinUnpinActorAuth(env.root)

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

  be setup_and_run() =>
    if @create_ui(_exe_path.cstring()) == 1 then
      update()
    end

  be update() =>
    let running = @update_ui()

    if running == 1 then
      _pm.update()
    else
      cleanup()
    end

  be draw(particles: Pointer[Particle ref] tag) =>
    @render_ui(particles)
    update()

  be cleanup() =>
    @destroy_ui()
    ActorPinning.request_unpin(_pin_auth)

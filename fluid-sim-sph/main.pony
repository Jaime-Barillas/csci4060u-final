use "actor_pinning"
use "log"
use "runtime_info"
use "sdl3"

actor Main
  let _log: Logger
  let _pin_auth: PinUnpinActorAuth

  new create(env: Env) =>
    let log_target = ConsoleTarget(env.out)
    _log = Logger("Main", log_target)
    _pin_auth = PinUnpinActorAuth(env.root)

    if not Sdl3.was_initialized() then
      _log.info(Sdl3.get_error())
      return
    end

    let thread_count = Scheduler.schedulers(SchedulerInfoAuth(env.root))
    _log.info("Scheduler Threads: " + thread_count.string())

    ActorPinning.request_pin(_pin_auth)
    wait_for_pin()

  be wait_for_pin() =>
    if ActorPinning.is_successfully_pinned(_pin_auth) then
      _log.info("Pinned!")
      run()
    else
      wait_for_pin()
    end

  be run() =>
    let display = Display
    _log.info(
      "Display Size: "
      + display.width().string()
      + ", "
      + display.height().string()
    )

    with window = Make.sdl_window("Title", display.width() / 2, display.height() / 2) do
      _log.info("Window up")
      var keep_running = true
      var ev: SdlEvent = SdlEvent
      while keep_running do
        Events.poll(ev)
        match ev.kind
        | EventType.quit() => keep_running = false
        end

        window.render()
      end
    end
    _log.info("Window destroyed")
    ActorPinning.request_unpin(_pin_auth)

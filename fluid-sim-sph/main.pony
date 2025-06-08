use "log"
use "runtime_info"
use "sdl3"
use "term"

actor Main
  new create(env: Env) =>
    let thread_count = Scheduler.schedulers(SchedulerInfoAuth(env.root))
    env.out.print("Threads: " + thread_count.string())

    if Sdl3.was_initialized() then
      env.out.print("SDL3 Initialized")
    else
      env.err.print(Sdl3.get_error())
    end

    let target = ConsoleTarget(env.out)
    let logger = Logger("console", target, Warn)
    logger.err("Test Error")
    logger.warn("Test Warn")
    logger.info("Test Info")

    let display = Display
    env.out.print(
      "Display Size: "
      + display.width().string()
      + ", "
      + display.height().string()
    )

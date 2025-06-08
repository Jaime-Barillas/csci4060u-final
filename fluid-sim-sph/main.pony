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

    let display = Display
    env.out.print(
      "Display Size: "
      + display.width().string()
      + ", "
      + display.height().string()
    )

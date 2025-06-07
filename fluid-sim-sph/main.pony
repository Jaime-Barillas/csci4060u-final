use "runtime_info"

actor Main
  new create(env: Env) =>
    let thread_count = Scheduler.schedulers(SchedulerInfoAuth(env.root))
    env.out.print("Threads: " + thread_count.string())

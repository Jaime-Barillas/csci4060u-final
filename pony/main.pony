actor Main
  let _renderer_name: Pointer[U8] tag

  new create(env: Env) =>
    ifdef "USE_VULKAN_RENDERER" then
      _renderer_name = "vulkan,software".cstring()
    else
      _renderer_name = Pointer[U8]
    end

    if @SDL_Init(SdlInitVideo()) == 0 then
      env.out.print("Fail")
      env.exitcode(1)
    end

    let n = @SDL_GetPrimaryDisplay()
    env.out.print("Hello, World!" + n.string())

    let w = @SDL_CreateWindow("Final - CSCI4060U - Pony".cstring(), 768, 512, 0)
    if w.is_null() then
      env.out.print("Window")
      env.exitcode(1)
    end
    let r = @SDL_CreateRenderer(w, Pointer[U8])
    if r.is_null() then
      env.out.print("Renderer")
      env.exitcode(1)
    end

    var should_quit = false
    var ev = SdlEvent
    while not should_quit do
      while (@SDL_PollEvent(ev) != 0) do
        match ev.event_type()
        | let _: SdlEventQuit => should_quit = true
          env.out.print("Quitting...")
        end
      end

      @SDL_SetRenderDrawColor(r, 0, 12, 56, 255)
      @SDL_RenderClear(r)
      @SDL_RenderPresent(r)
    end

    @SDL_DestroyRenderer(r)
    @SDL_DestroyWindow(r)

    @SDL_Quit()


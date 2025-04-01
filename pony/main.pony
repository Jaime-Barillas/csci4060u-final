use "libui"

actor Main
  let _renderer_name: Pointer[U8] tag

  new create(env: Env) =>
    ifdef "USE_VULKAN_RENDERER" then
      _renderer_name = "vulkan,software".cstring()
    else
      _renderer_name = Pointer[U8]
    end

    SDL.init(0)
    env.out.print("Hello, World!")
    SDL.quit()

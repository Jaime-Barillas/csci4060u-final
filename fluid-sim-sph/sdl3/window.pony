class SdlWindow
  let _w: _Window
  let _d: _GpuDevice

  new _null() =>
    _w = _Window
    _d = _GpuDevice

  new create(title: String, width: I64, height: I64)? =>
    _w = @SDL_CreateWindow(title.cstring(), width.i32(), height.i32(), 0)
    if _w.is_null() then
      error
    end

    _d = @SDL_CreateGPUDevice(GpuShaderFormat.spirv(), 0, Pointer[U8])
    if _d.is_null() then
      error
    end

    if @SDL_ClaimWindowForGPUDevice(_d, _w) == 0 then
      error
    end

  fun _is_null(): Bool =>
    _w.is_null() or _d.is_null()

  fun render() =>
    let cmd_buf = @SDL_AcquireGPUCommandBuffer(_d)
    var swap_tex: GpuTexture tag = GpuTexture
    if @SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, _w, addressof swap_tex, Pointer[U32], Pointer[U32]) == 0 then
      @SDL_CancelGPUCommandBuffer(cmd_buf)
    else
      let colorti: SdlGpuColorTargetInfo = SdlGpuColorTargetInfo

      colorti.texture = swap_tex
      // FIXME: Always black.
      colorti.clear_color.r = 1.0
      colorti.clear_color.g = 1.0
      colorti.clear_color.b = 1.0
      colorti.clear_color.a = 1.0
      colorti.load_op = SdlGpuLoadOp.clear()
      colorti.store_op = SdlGpuStoreOp.store()

      let render_pass = @SDL_BeginGPURenderPass(cmd_buf, colorti, 1, Pointer[None])
      @SDL_EndGPURenderPass(render_pass)
      @SDL_SubmitGPUCommandBuffer(cmd_buf)
    end

  fun dispose() =>
    @SDL_ReleaseWindowFromGPUDevice(_d, _w)
    @SDL_DestroyGPUDevice(_d)
    @SDL_DestroyWindow(_w)

primitive Make
  fun sdl_window(title: String, width: I64, height: I64): SdlWindow =>
    try
      SdlWindow(title, width, height)?
    else
      SdlWindow._null()
    end

primitive Events
  fun poll(event: SdlEvent): Bool =>
    @SDL_PollEvent(event) == 1

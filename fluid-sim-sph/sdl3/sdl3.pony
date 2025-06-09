"""
SDL3 bindings to the few functions used.
"""

use "lib:SDL3"
use "lib:wrapper" if not windows
use "lib:libwrapper" if windows

/* Init */
use @SDL_Init[U8](flags: U32)
use @SDL_Quit[None]()
use @SDL_WasInit[U32](flags: U32)

/* Error */
use @SDL_GetError[Pointer[U8] ref]()

/* Video */
use @SDL_GetPrimaryDisplay[U32]()
use @SDL_GetDisplayBounds[U8](display_id: U32, rect: SdlRect)
use @SDL_CreateWindow[_Window](title: Pointer[U8] tag, w: I32, h: I32, flags: U64)
use @SDL_DestroyWindow[None](window: _Window)

use @SDL_CreateGPUDevice[_GpuDevice](format_flags: U32, debug_mode: U8, name: Pointer[U8] tag)
use @SDL_DestroyGPUDevice[None](device: _GpuDevice)
use @SDL_ClaimWindowForGPUDevice[U8](device: _GpuDevice, window: _Window)
use @SDL_ReleaseWindowFromGPUDevice[None](device: _GpuDevice, window: _Window)
use @SDL_AcquireGPUCommandBuffer[_GpuCommandBuffer](device: _GpuDevice)
use @SDL_CancelGPUCommandBuffer[U8](command_buffer: _GpuCommandBuffer)
// FIXME: Use non blocking variant? See warning in remarks of docs for
//        SDL_AcquireGPUCommandBuffer.
use @SDL_WaitAndAcquireGPUSwapchainTexture[U8](
  command_buffer: _GpuCommandBuffer,
  window: _Window,
  swapchain_texture: Pointer[GpuTexture] tag,
  swapchain_texture_width: Pointer[U32] tag,
  swapchain_texture_height: Pointer[U32] tag
)
use @SDL_BeginGPURenderPass[_GpuRenderPass](
  command_buffer: _GpuCommandBuffer,
  color_target_infos: SdlGpuColorTargetInfo,
  num_color_targets: U32,
  depth_stencil_target_info: Pointer[None] tag // Not used
)
use @SDL_EndGPURenderPass[None](render_pass: _GpuRenderPass)
use @SDL_SubmitGPUCommandBuffer[U8](command_buffer: _GpuCommandBuffer)

/* Events */
use @SDL_PollEvent[U8](event: SdlEvent)
use @PSDL_ConvertEvent[SdlEvent ref](event: SdlEvent ref)

struct SdlRect
  var x: I32 = 0
  var y: I32 = 0
  var w: I32 = 0
  var h: I32 = 0

primitive _WindowP
type _Window is Pointer[_WindowP] tag

primitive _GpuDeviceP
type _GpuDevice is Pointer[_GpuDeviceP] tag

primitive GpuShaderFormat
  fun spirv(): U32 => 2

primitive _GpuCommandBufferP
type _GpuCommandBuffer is Pointer[_GpuCommandBufferP] tag

primitive _GpuTextureP
type GpuTexture is Pointer[_GpuTextureP] tag

primitive SdlGpuLoadOp
  fun clear(): U32 => 1

primitive SdlGpuStoreOp
  fun store(): U32 => 0

struct SdlFColor
  var r: F32 = 0
  var g: F32 = 0
  var b: F32 = 0
  var a: F32 = 1

struct SdlGpuColorTargetInfo
  var texture: GpuTexture         = GpuTexture
  var mip_level: U32              = 0
  var layer_or_depth_plane: U32   = 0
  embed clear_color: SdlFColor    = SdlFColor
  var load_op: U32                = 0
  var store_op: U32               = 0
  var resolve_texture: GpuTexture = GpuTexture
  var resolve_mip_level: U32      = 0
  var resolve_layer: U32          = 0
  var cycle: U8                   = 0
  var cycle_resolve_texture: U8   = 0
  var padding1: U8                = 0
  var padding2: U8                = 0

primitive _GpuRenderPassP
type _GpuRenderPass is Pointer[_GpuRenderPassP] tag

primitive EventType
  fun quit(): U32 => 0x100
  fun mouse_motion(): U32 => 0x400
  fun mouse_button_down(): U32 => 0x401
  fun mouse_button_up(): U32 => 0x402

// NOTE: SDL_Event is padded to a size of 128 bytes.
struct SdlEvent
  var kind:  U32 = 0
  var pad00: U32 = 0
  var pad01: U64 = 0
  var pad02: U64 = 0
  var pad03: U64 = 0
  var pad04: U64 = 0
  var pad05: U64 = 0
  var pad06: U64 = 0
  var pad07: U64 = 0
  var pad08: U64 = 0
  var pad09: U64 = 0
  var pad10: U64 = 0
  var pad11: U64 = 0
  var pad12: U64 = 0
  var pad13: U64 = 0
  var pad14: U64 = 0
  var pad15: U64 = 0

  fun ref as_mouse_motion_event(): SdlMouseMotionEvent =>
    @PSDL_ConvertEvent[SdlMouseMotionEvent](this)

  fun ref as_mouse_button_event(): SdlMouseButtonEvent =>
    @PSDL_ConvertEvent[SdlMouseButtonEvent](this)

struct SdlMouseMotionEvent
  var kind:  U32 = 0
  var pad00: U32 = 0 // reserved
  var pad01: U64 = 0 // timestamp
  var pad02: U64 = 0 // windowID + which (mouse)
  var pad03: U32 = 0 // state (mouse buttons)
  var x:     F32 = 0
  var y:     F32 = 0
  var xrel:  F32 = 0
  var yrel:  F32 = 0
  var pad04: U32 = 0
  var pad05: U64 = 0
  var pad06: U64 = 0
  var pad07: U64 = 0
  var pad08: U64 = 0
  var pad09: U64 = 0
  var pad10: U64 = 0
  var pad11: U64 = 0
  var pad12: U64 = 0
  var pad13: U64 = 0
  var pad14: U64 = 0

struct SdlMouseButtonEvent
  var kind:  U32 = 0
  var pad00: U32 = 0 // reserved
  var pad01: U64 = 0 // timestamp
  var pad02: U64 = 0 // windowID + which (mouse)
  var button: U8 = 0
  var down:   U8 = 0
  var clicks: U8 = 0
  var pad03:  U8 = 0
  var x:     F32 = 0
  var y:     F32 = 0
  var pad04: U32 = 0
  var pad05: U64 = 0
  var pad06: U64 = 0
  var pad07: U64 = 0
  var pad08: U64 = 0
  var pad09: U64 = 0
  var pad10: U64 = 0
  var pad11: U64 = 0
  var pad12: U64 = 0
  var pad13: U64 = 0
  var pad14: U64 = 0
  var pad15: U64 = 0

primitive Sdl3
  fun _init() =>
    if @SDL_Init(0x00000020) == 0 then // Init video
      @SDL_Quit()
    end

  fun _final() => @SDL_Quit()

  fun was_initialized(): Bool =>
    (@SDL_WasInit(0) and 0x00000020) != 0

  fun get_error(): String =>
    recover val String.copy_cstring(@SDL_GetError()) end

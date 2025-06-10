"""
SDL3 bindings to the few functions used.
"""

use "lib:SDL3"
use "lib:wrapper" if not windows
use "lib:libwrapper" if windows

/*=====*/
/* FFI */
/*=====*/

/* Init */
use @SDL_Init[U8](flags: U32)
use @SDL_Quit[None]()
use @SDL_WasInit[U32](flags: U32)

/* Error */
use @SDL_GetError[Pointer[U8] ref]()

/* Video */
use @SDL_GetPrimaryDisplay[U32]()
use @SDL_GetDisplayBounds[U8](display_id: U32, rect: Rect)
use @SDL_CreateWindow[Window](
  title: Pointer[U8] tag,
  w: I32,
  h: I32,
  flags: U64
)
use @SDL_DestroyWindow[None](window: Window)

use @SDL_CreateGPUDevice[GPUDevice](
  format_flags: U32,
  debug_mode: U8,
  name: Pointer[U8] tag
)
use @SDL_DestroyGPUDevice[None](device: GPUDevice)
use @SDL_ClaimWindowForGPUDevice[U8](device: GPUDevice, window: Window)
use @SDL_ReleaseWindowFromGPUDevice[None](device: GPUDevice, window: Window)
use @SDL_AcquireGPUCommandBuffer[GPUCommandBuffer](device: GPUDevice)
use @SDL_CancelGPUCommandBuffer[U8](command_buffer: GPUCommandBuffer)
// FIXME: Use non blocking variant? See warning in remarks of docs for
//        SDL_AcquireGPUCommandBuffer.
use @SDL_WaitAndAcquireGPUSwapchainTexture[U8](
  command_buffer: GPUCommandBuffer,
  window: Window,
  swapchain_texture: Pointer[GPUTexture] tag,
  swapchain_texture_width: Pointer[U32] tag,
  swapchain_texture_height: Pointer[U32] tag
)
use @SDL_BeginGPURenderPass[GPURenderPass](
  command_buffer: GPUCommandBuffer,
  color_target_infos: GPUColorTargetInfo,
  num_color_targets: U32,
  depth_stencil_target_info: Pointer[None] tag // Not used
)
use @SDL_EndGPURenderPass[None](render_pass: GPURenderPass)
use @SDL_SubmitGPUCommandBuffer[U8](command_buffer: GPUCommandBuffer)

/* Events */
use @SDL_PollEvent[U8](event: Event)
use @PSDL_ConvertEvent[Event ref](event: Event ref)

/*=====*/
/* API */
/*=====*/

type Fn is SDL // Make nicer to use with package aliases
primitive SDL
  fun _init() =>
    if @SDL_Init(InitVideo()) == 0 then // Init video
      @SDL_Quit()
    end

  fun _final() => @SDL_Quit()

  fun was_init(): Bool =>
    (@SDL_WasInit(0) and InitVideo()) != 0

  fun get_error(): String =>
    recover val String.copy_cstring(@SDL_GetError()) end

  fun get_primary_display(): DisplayID => @SDL_GetPrimaryDisplay()

  fun get_display_bounds(display_id: DisplayID, rect: Rect): Bool =>
    @SDL_GetDisplayBounds(display_id, rect) != 0

  fun create_window(title: String, w: I32, h: I32, flags: U64): Window =>
    @SDL_CreateWindow(title.cstring(), w, h, flags)

  fun destroy_window(window: Window) => @SDL_DestroyWindow(window)

  fun create_gpu_device(
    format_flags: U32,
    debug_mode: Bool,
    name: (String | None) = None
  ): GPUDevice =>
    @SDL_CreateGPUDevice(
      format_flags,
      if debug_mode then 1 else 0 end,
      match name
      | let _: None => Pointer[U8]
      | let s: String => s.cstring()
      end
    )

  fun destroy_gpu_device(device: GPUDevice) => @SDL_DestroyGPUDevice(device)

  fun claim_window_for_gpu_device(device: GPUDevice, window: Window): Bool =>
    @SDL_ClaimWindowForGPUDevice(device, window) != 0

  fun release_window_from_gpu_device(device: GPUDevice, window: Window) =>
    @SDL_ReleaseWindowFromGPUDevice(device, window)

  fun acquire_gpu_command_buffer(device: GPUDevice): GPUCommandBuffer =>
    @SDL_AcquireGPUCommandBuffer(device)

  fun cancel_gpu_command_buffer(command_buffer: GPUCommandBuffer): Bool =>
    @SDL_CancelGPUCommandBuffer(command_buffer) != 0

  fun wait_and_acquire_gpu_swapchain_texture(
    command_buffer: GPUCommandBuffer,
    window: Window,
    swapchain_texture_width: Pointer[U32] tag = Pointer[U32],
    swapchain_texture_height: Pointer[U32] tag = Pointer[U32]
  ): (GPUTexture, Bool) =>
    var tex: GPUTexture = GPUTexture
    let res = @SDL_WaitAndAcquireGPUSwapchainTexture(
      command_buffer,
      window,
      addressof tex,
      swapchain_texture_width,
      swapchain_texture_height
    ) != 0
    (tex, res)

  fun begin_gpu_render_pass(
    command_buffer: GPUCommandBuffer,
    color_target_infos: GPUColorTargetInfo,
    num_color_targets: U32,
    depth_stencil_target_info: Pointer[None] tag
  ): GPURenderPass =>
    @SDL_BeginGPURenderPass(
      command_buffer,
      color_target_infos,
      num_color_targets,
      depth_stencil_target_info
    )

  fun end_gpu_render_pass(render_pass: GPURenderPass) =>
    @SDL_EndGPURenderPass(render_pass)

  fun submit_gpu_command_buffer(command_buffer: GPUCommandBuffer): Bool =>
    @SDL_SubmitGPUCommandBuffer(command_buffer) != 0

  fun poll_event(event: Event): Bool =>
    @SDL_PollEvent(event) != 0

/*=======*/
/* Types */
/*=======*/

type DisplayID is U32

primitive InitVideo
  fun apply(): U32 => 0x00000020

struct Rect
  var x: I32 = 0
  var y: I32 = 0
  var w: I32 = 0
  var h: I32 = 0

primitive _Window
type Window is Pointer[_Window] tag

primitive _GPUDevice
type GPUDevice is Pointer[_GPUDevice] tag

primitive GPUShaderFormat
  fun spirv(): U32 => 2

primitive _GPUCommandBuffer
type GPUCommandBuffer is Pointer[_GPUCommandBuffer] tag

primitive _GPUTexture
type GPUTexture is Pointer[_GPUTexture] tag

primitive GPULoadOp
  fun clear(): U32 => 1

primitive GPUStoreOp
  fun store(): U32 => 0

struct FColor
  var r: F32 = 0
  var g: F32 = 0
  var b: F32 = 0
  var a: F32 = 1

struct GPUColorTargetInfo
  var texture: GPUTexture         = GPUTexture
  var mip_level: U32              = 0
  var layer_or_depth_plane: U32   = 0
  embed clear_color: FColor       = FColor
  var load_op: U32                = 0
  var store_op: U32               = 0
  var resolve_texture: GPUTexture = GPUTexture
  var resolve_mip_level: U32      = 0
  var resolve_layer: U32          = 0
  var cycle: U8                   = 0
  var cycle_resolve_texture: U8   = 0
  var padding1: U8                = 0
  var padding2: U8                = 0

primitive _GPURenderPass
type GPURenderPass is Pointer[_GPURenderPass] tag

primitive EventKind
  fun quit(): U32 => 0x100
  fun mouse_motion(): U32 => 0x400
  fun mouse_button_down(): U32 => 0x401
  fun mouse_button_up(): U32 => 0x402

// NOTE: SDL_Event is padded to a size of 128 bytes.
struct Event
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

  fun ref as_mouse_motion_event(): MouseMotionEvent =>
    @PSDL_ConvertEvent[MouseMotionEvent](this)

  fun ref as_mouse_button_event(): MouseButtonEvent =>
    @PSDL_ConvertEvent[MouseButtonEvent](this)

struct MouseMotionEvent
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

struct MouseButtonEvent
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

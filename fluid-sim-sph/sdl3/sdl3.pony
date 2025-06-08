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
use @SDL_CreateWindow[Window](title: Pointer[U8] tag, w: I32, h: I32, flags: U64)
use @SDL_DestroyWindow[None](window: Window)

/* Events */
use @SDL_PollEvent[U8](event: Event)
use @PSDL_ConvertEvent[Event ref](event: Event ref)

struct SdlRect
  var x: I32 = 0
  var y: I32 = 0
  var w: I32 = 0
  var h: I32 = 0

primitive _Window
type Window is Pointer[_Window] tag

primitive EventType
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

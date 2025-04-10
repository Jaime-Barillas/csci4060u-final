use "lib:ui"             if not windows
use "lib:libui"          if windows

use "lib:udev"           if not windows
use "lib:dbus-1"         if not windows
use "lib:pipewire-0.3"   if not windows
use "lib:wayland-client" if not windows
use "lib:wayland-egl"    if not windows
use "lib:wayland-cursor" if not windows
use "lib:xkbcommon"      if not windows

use "lib:version"        if windows
use "lib:Imm32"          if windows
use "lib:SetupAPI"       if windows
use "lib:Winmm"          if windows
use "lib:user32"         if windows
use "lib:gdi32"          if windows
use "lib:winspool"       if windows
use "lib:shell32"        if windows
use "lib:ole32"          if windows
use "lib:oleaut32"       if windows
use "lib:uuid"           if windows
use "lib:comdlg32"       if windows

use @SDL_Init[CBool](flags: U32)
use @SDL_Quit[None]()
use @SDL_GetError[Pointer[U8] box]()
use @SDL_GetPrimaryDisplay[SdlDisplayID]()
use @SDL_GetDisplayUsableBounds[CBool](id: SdlDisplayID, rect: SdlRect tag)
use @SDL_CreateWindow[SdlWindow](title: Pointer[U8] tag, width: I32, height: I32, flags: U64)
use @SDL_DestroyWindow[None](window: SdlWindow)
use @SDL_CreateRenderer[SdlRenderer](window: SdlWindow, renderer_name: Pointer[U8] tag)
use @SDL_DestroyRenderer[None](renderer: SdlRenderer)
use @SDL_SetRenderDrawBlendMode[CBool](renderer: SdlRenderer, mode: U32)
use @SDL_SetRenderLogicalPresentation[CBool](renderer: SdlRenderer, width: I32, height: I32, mode: U32 /* C Enum */)
use @SDL_SetRenderVSync[CBool](renderer: SdlRenderer, vsync: I32)
use @SDL_PollEvent[CBool](event: SdlEvent tag)
use @SDL_ConvertEventToRenderCoordinates[CBool](renderer: SdlRenderer, event: SdlEvent tag)
use @SDL_SetRenderDrawColor[CBool](renderer: SdlRenderer, r: U8, g: U8, b: U8, a: U8)
use @SDL_RenderClear[CBool](renderer: SdlRenderer)
use @SDL_RenderPresent[CBool](renderer: SdlRenderer)

type CBool is U32

primitive SdlInitVideo
  fun apply(): U32 => 0x20
type SdlInitFlags is SdlInitVideo

primitive SdlBlendmodeBlend
  fun apply(): U32 => 1
type SdlBlendmode is SdlBlendmodeBlend

primitive SdlLogicalPresentationIntegerScale
  fun apply(): U32 => 4
type SdlRenderLogicalPresentation is SdlLogicalPresentationIntegerScale

type SdlDisplayID is U32

struct SdlRect
  var x: I32 = 0
  var y: I32 = 0
  var w: I32 = 0
  var h: I32 = 0

primitive _SdlWindow
type SdlWindow is Pointer[_SdlWindow] tag

primitive _SdlRenderer
type SdlRenderer is Pointer[_SdlWindow] tag

// SDL_Event is a union of various structs.
// Size: 128 bytes.
// First 32 bytes contains the event type.
primitive SdlEventQuit
  fun apply(): U32 => 0x100
primitive SdlEventMouseMotion
  fun apply(): U32 => 0x400
primitive SdlEventMouseButtonDown
  fun apply(): U32 => 1025
primitive SdlEventMouseButtonUp
  fun apply(): U32 => 1026
primitive SdlEventUnknown
  fun apply(): U32 => -1
// Only SDL_EVENT_QUIT, SDL_EVENT_MOUSE_MOTION, and SDL_EVENT_MOUSE_BUTTON_{DOWN,UP} are used.
struct SdlEvent
  var _event_type: U32 = 0
  var _pad1:       U32 = 0  // reserved
  var _pad2:       U64 = 0  // timestamp
  var _pad3:       U64 = 0  // SDL_WindowID, SDL_MouseID
  var _pad4:       U32 = 0  // SDL_MouseButtonFlags
  var _x:          F32 = 0
  var _y:          F32 = 0
  var _xrel:       F32 = 0  // (Mouse motion only)
  var _yrel:       F32 = 0  // (Mouse motion only)
  var _pad9:       U32 = 0
  var _pad10:      U64 = 0
  var _pad11:      U64 = 0
  var _pad12:      U64 = 0
  var _pad13:      U64 = 0
  var _pad14:      U64 = 0
  var _pad15:      U64 = 0
  var _pad16:      U64 = 0
  var _pad17:      U64 = 0
  var _pad18:      U64 = 0
  var _pad19:      U64 = 0

  fun event_type(): ( SdlEventQuit
                    | SdlEventMouseMotion
                    | SdlEventMouseButtonDown
                    | SdlEventMouseButtonUp
                    | SdlEventUnknown ) =>
    match _event_type
    | SdlEventQuit()            => SdlEventQuit
    | SdlEventMouseMotion()     => SdlEventMouseMotion
    | SdlEventMouseButtonDown() => SdlEventMouseButtonDown
    | SdlEventMouseButtonUp()   => SdlEventMouseButtonUp
    else
      SdlEventUnknown
    end

  fun x(): F32 => _x
  fun y(): F32 => _y
  fun xrel(): F32 => _xrel
  fun yrel(): F32 => _yrel


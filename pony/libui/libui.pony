// libui packages SDL3, microui, and the microui based UI.
use "lib:ui" if not windows
use "lib:libui" if windows

// It would be easier to link against shared libraries for SDL3. Since we are
// linking against the static libraries we also have to link against its
// dependencies:
// TODO: This list is for Fedora with wayland, test under Ubuntu.
//       Should we only support X11 and depend on XWayland?
// libudev.so libdbus-1.so libpipewire-0.3.so libwayland-client.so
// libwayland-egl.so libwayland-cursor.so libxkbcommon.so
use "lib:udev" if not windows
use "lib:dbus-1" if not windows
use "lib:pipewire-0.3" if not windows
use "lib:wayland-client" if not windows
use "lib:wayland-egl" if not windows
use "lib:wayland-cursor" if not windows
use "lib:xkbcommon" if not windows

// Windows SDL3 deps.
use "lib:version" if windows
use "lib:Imm32" if windows
use "lib:SetupAPI" if windows
use "lib:Winmm" if windows

// SDL3 FFI declarations.
use @SDL_Init[Bool](flags: U32)
use @SDL_Quit[None]()

primitive SDL
  fun init(flags: U32): Bool => @SDL_Init(flags)
  fun quit() => @SDL_Quit()

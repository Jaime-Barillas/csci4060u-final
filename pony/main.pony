use "lib:SDL3_shared" if not windows

use "actor_pinning"

// ANCHOR: SDL_FFI
use @SDL_Init[Bool](flags: U32)
use @SDL_Quit[None]()
use @SDL_CreateWindowAndRenderer[Bool](title: Pointer[U8] tag, width: U32, height: U32, window_flags: U64, window: SdlWindow, renderer: SdlRenderer)
use @SDL_DestroyRenderer[None](renderer: SdlRenderer)
use @SDL_DestroyWindow[None](window: SdlWindow)
use @SDL_PollEvent[U32](event: SdlEvent)
use @SDL_RenderClear[Bool](renderer: SdlRenderer)
use @SDL_RenderPresent[Bool](renderer: SdlRenderer)
// ANCHOR_END: SDL_FFI

type SdlWindow is Pointer[None] tag
type SdlRenderer is Pointer[None] tag
type SdlEvent is Pointer[U8] tag // bleh!

primitive SdlInit
  fun video(): U32 => 0x00000020

actor Main
  let _env: Env
  let _pin_auth: PinUnpinActorAuth
  var _window: SdlWindow
  var _renderer: SdlRenderer

  new create(env: Env) =>
    _env = env
    _pin_auth = PinUnpinActorAuth(env.root)
    _window = SdlWindow
    _renderer = SdlRenderer
    env.out.print("Requesting Main actor to be pinned...")
    ActorPinning.request_pin(_pin_auth)
    await_pinning()

  // ANCHOR: pinning
  be await_pinning() =>
    if ActorPinning.is_successfully_pinned(_pin_auth) then
      run()
    else
      await_pinning()
    end
  // ANCHOR_END: pinning

  be run() =>
    _env.out.print("Successfully Pinned!")
    @SDL_Init(SdlInit.video())

    @SDL_CreateWindowAndRenderer("Pony SDL3".cstring(), 640, 480, 0, addressof _window, addressof _renderer)
    var should_quit = false
    let ev = Array[U8](128)
    ev.reserve(128)
    _env.out.print("Event pointer: " + ev.cpointer().usize().string())
    repeat
      // while @SDL_PollEvent(ev.cpointer()) do
      //   _env.out.print("Polling...")
      //   if try ev.read_u32(0)? == 0x100 /*SDL_EVENT_QUIT*/ else false end then
      //     should_quit = true
      //   end
      // end
      var b = @SDL_PollEvent(ev.cpointer())
      _env.out.print(b.string())
      b = @SDL_PollEvent(ev.cpointer())
      _env.out.print(b.string())
      b = @SDL_PollEvent(ev.cpointer())
      _env.out.print(b.string())
      b = @SDL_PollEvent(ev.cpointer())
      _env.out.print(b.string())
      b = @SDL_PollEvent(ev.cpointer())
      _env.out.print(b.string())
      _env.out.print((try ev.read_u32(0)? else 6969 end).string())
      should_quit = true
      @SDL_RenderClear(_renderer)
      @SDL_RenderPresent(_renderer)
    until should_quit end

    @SDL_DestroyRenderer(_renderer)
    @SDL_DestroyWindow(_window)
    @SDL_Quit()
    ActorPinning.request_unpin(_pin_auth)

use "debug"
use "term"

class Display
  let _display_id: U32
  var _width: I64
  var _height: I64

  new create() =>
    _display_id = @SDL_GetPrimaryDisplay()
    _width = 0
    _height = 0
    if _display_id == 0 then
      Debug.err(
        ANSI.bright_yellow() + "[SDL3] "
        + ANSI.bright_red() + "Error: "
        + ANSI.reset() + Sdl3.get_error()
      )
    else
      let rect: SdlRect = SdlRect
      if @SDL_GetDisplayBounds(_display_id, rect) == 0 then
        Debug.err(
          ANSI.bright_yellow() + "[SDL3] "
          + ANSI.bright_red() + "Error: "
          + ANSI.reset() + Sdl3.get_error()
        )
      else
        // It is possible for the primary monitor to not be located at (0, 0).
        // In particular for multi-monitor setup.
        _width = (rect.w - rect.x).i64()
        _height = (rect.h - rect.y).i64()
      end
    end

  fun width(): I64 => _width
  fun height(): I64 => _height

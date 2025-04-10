use "lib:ui" if not windows
use "lib:libui" if windows

use @UI_create_ui[_Ui]()
use @UI_destroy_ui[None](ui: _Ui)
use @UI_get_pcount[F32](ui: _Ui)
use @UI_get_time_step[F32](ui: _Ui)
use @UI_get_sim_steps[F32](ui: _Ui)
use @UI_get_gravity_y[F32](ui: _Ui)
use @UI_set_frame_time_sim[None](ui: _Ui, frame_time_sim: F32)
use @UI_set_frame_time_step[None](ui: _Ui, frame_time_step: F32)
use @UI_update_mouse_pos[None](ui: _Ui, x: I32, y: I32)
use @UI_update_mouse_down[None](ui: _Ui, x: I32, y: I32)
use @UI_update_mouse_up[None](ui: _Ui, x: I32, y: I32)
use @UI_draw[None](ui: _Ui, renderer: SdlRenderer)

type _Ui is Pointer[None] tag

class Ui
  let _ui: _Ui

  new create() =>
    _ui = @UI_create_ui()

  fun ref destroy() => @UI_destroy_ui(_ui)

  fun pcount(): F32 => @UI_get_pcount(_ui)
  fun time_step(): F32 => @UI_get_time_step(_ui)
  fun sim_steps(): F32 => @UI_get_sim_steps(_ui)
  fun gravity_y(): F32 => @UI_get_gravity_y(_ui)
  fun ref set_frame_time_sim(value: F32) => @UI_set_frame_time_sim(_ui, value)
  fun ref set_frame_time_step(value: F32) => @UI_set_frame_time_step(_ui, value)
  fun ref update_mouse_pos(x: F32, y: F32) => @UI_update_mouse_pos(_ui, x.i32(), y.i32())
  fun ref update_mouse_down(x: F32, y: F32) => @UI_update_mouse_down(_ui, x.i32(), y.i32())
  fun ref update_mouse_up(x: F32, y: F32) => @UI_update_mouse_up(_ui, x.i32(), y.i32())
  fun ref draw(renderer: SdlRenderer) => @UI_draw(_ui, renderer)

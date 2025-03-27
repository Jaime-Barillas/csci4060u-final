// microui provides a C only header. We need to wrap any #include of it with
// extern "C" { ... } to ensure it works with C++.
extern "C" {
  #include <microui.h>
}

#import "../../../../../typst/templates/paper.typ" : paper
#import "code.typ"

#show: paper.with(
  title: [Robobots Battle Royale],
  abstract: [
    lorem freaking ipsum. #lorem(90)
  ],
  authors: ((name: "Jaime Barillas", organization: "OntarioTech University",),),
  code-font: "Fantasque Sans Mono"
)

#set raw(syntaxes: "pony-min.sublime-syntax")

= Introduction

#code.insert("../pony/main.pony", anchor: "SDL_FFI", lang: "pony", caption: [Function wrappers...])
#code.insert("../pony/main.pony", anchor: "pinning", lang: "pony", caption: [To run on the main thread...])

#lorem(130)
```pony
// A comment

type Color is _U16

actor Main
  """
    Doc strings!
  """
  new create(env: Env iso)? =>
    /*
       block comments!
     */
    (1 + 2 - 3) / 2 *3.24 % 4 %% 5 xor "a" < 2 > 2 <= 2 >= 2
    if true then
      env.out.print("Hello, World!")
      env.out.print('\n')
      env.out.print('A')
    end
    if env is env then
      env.out.write("Goodbye, World!")
    end
    match x
    | None => 2
    | let h: String => "2"
    end
}
```

== Part 2
#lorem(160)

#lorem(160)
#figure(
raw(
"int main(void) {
  return 1 + 2 * 3 / 4;
}",lang: "c"
),
caption: "testing"
)

== Part 3
#lorem(160)
#lorem(160)

#import "paper.typ" : paper
#import "code.typ"

#show: paper.with(
  title: [Robobots Battle Royale],
  authors: ((name: "Jaime Barillas", organization: "Ontario Tech University",),),
  code-font: "Fantasque Sans Mono",
  abstract: [
    lorem freaking ipsum. #lorem(90)
  ]
)

#set raw(syntaxes: "pony-min.sublime-syntax")

= Introduction

= Fluid Simulation With Smoothed Particle Hydrodynamics

#code.insert(
  "../pony/main.pony",
  "SDL_FFI",
  lang: "pony",
  caption: [Function wrappers...]
)
#code.insert(
  "../pony/main.pony",
  "pinning",
  lang: "pony",
  caption: [To run on the main thread...],
)

// See @pony:main.pony:pinning for cool code.

Test biblio @schuermann2016sph @schuermann2017sph2d @MullerMatthias2003Pfsf @Clavet-2005-PVFS

#bibliography("bibliography.bib")


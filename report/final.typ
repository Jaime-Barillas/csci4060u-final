#import "paper.typ" : paper
#import "code.typ"

#show: paper.with(
  title: [2D Fluid Sim With Pony],
  authors: ((name: "Jaime Barillas", organization: "Ontario Tech University",),),
  code-font: "Fantasque Sans Mono",
  abstract: [
    This report presents the results of implementing a simple 2D fluid
    simulator using Pony, an object oriented, actor model programming language
    for concurrent programs. It also provides an introduction to Pony in the
    context of building the fluid simulator. Focus is placed on the discussion
    of difficulties arising from Pony's implementation of the actor model and
    its use of _reference capabilities_ to ensure _data-race_ and _lock-free_
    programs.
  ]
)

#set raw(syntaxes: "pony-min.sublime-syntax")

= Preliminaries - Particle Based Fluid Simulation

There are two main approaches to simulating fluids: A grid based method,
which focuses on the motion of fluid through grid cells, and a particle
based method, which focuses on the motion of individual particles. The
implementation accompanying this report takes the particle based approach,
in particular,
#link(
  "https://en.wikipedia.org/wiki/Smoothed-particle_hydrodynamics",
  [Smoothed Particle Hydrodynamics (SPH)]
)
techniques are used to approximate the properties of particles needed to
calculate acceleration, velocity, and ultimately, position. This report does
not cover SPH in any detail, only the broad. See @MullerMatthias2003Pfsf for
good overview of the subject.

\

The main take-away from SPH is that many properties of fluids at specific
point $r$ can be calculated as a weighted sum of that same property of the
neighbouring particles multiplied with a smoothing function $W$:
$
  A(r) = limits(Sigma)_j m_j A_j/rho_j W(r - r_j, h)
$
Here $rho_j$ refers to the density of neighbouring particle $j$, $r_j$ is
the position of particle $j$, $m_j$ is the mass of the particle, and $h$ is
a "support radius" (the maximum distance neighbouring particles can be to
contribute to $A(r)$). This formula is used to calculate density @density-sph,
pressure forces @pressure-sph, and viscosity forces @viscosity-sph, all of
which are needed to create a working and convincing simulation.
$
  A_rho(r) = limits(Sigma)_j m_j W(r - r_j, h)
$<density-sph>
$
  A_("pressure")(r) = limits(Sigma)_j m_j A_("pressure",j)/rho_j nabla W(r - r_j, h)
$<pressure-sph>
$
  A_("viscosity")(r) = limits(Sigma)_j m_j A_("viscosity",j)/rho_j nabla^2 W(r - r_j, h)
$<viscosity-sph>
For @pressure-sph we require the gradient of the smoothing function $W$ and
for @viscosity-sph we need to calculate the laplacian. It should be noted
that each $W$ in the above equations can be chosen seperately. The purpose
of the smoothing functions to determine how much the neighbour particle
contributes to the property at position $r$, further away neighbours
contribute less. The exact
smoothing functions chosen are those used in @Sol11b.
Note also how replacing $A_rho$ with density ($rho$) causes it to cancel out in
the right-hand side of equation @density-sph.

\

The fluid simulator comes in two implementations: A C++ version using OpenMP
to parallelize computations, and a Pony version using the fork_join library
to try and match the way OpenMP distributes work across available threads.

= The Pony Programming Language

We start with a quick introduction to the basics of Pony before moving on to
discuss the implementation.
Pony is an actor model based language that claims to be _data-race free_ and
_deadlock free_@ponyclaims. It uses its type system to statically ensure that
a Pony program cannot have data races. Variables and their aliases can be
annotated with different _reference capabilities_ that determine how the data
can be shared among actors. For example, the ```pony val``` capability allows
data annotated with it to be shared amongst actors, whereas, the
```pony iso``` capability does not. So, how does Pony use actors and reference
capabilities to model a concurrent program?

== Actors & Classes

On top of being based on the actor model, Pony is also object oriented. Actors
and classes represent two "modes" of concurrency in Pony. Classes provide soley
synchronous execution of code (i.e. no concurrency) while actors allow for
asynchronous execution. Given the bellow Pony program, all execution of the
code in both classes will run sequentially.

#figure(
  caption: [
    This pony program will _always_ print `Hello A!` followed by `Hello B!`.
  ],
  [
    ```pony
    class A
      fun say_hello(env: Env) => env.out.print("Hello A!")

    class B
      fun say_hello(env: Env) => env.out.print("Hello B!")

    actor Main
      new create(env: Env) =>
        let a = A
        let b = B
        a.say_hello(env)
        b.say_hello(env)
    ```
  ]
)<pony-classes>

Take the following program instead. Classes ```pony A``` and ```pony B``` are
replaced with actors. This program has the potential to run the
```pony say_hello()``` behaviours in any order.

#figure(
  caption: [
    This pony program will could print `Hello A!` and `Hello B!` in any order.
  ],
  [
    ```pony
    actor A
      be say_hello(env: Env) => env.out.print("Hello A!")

    actor B
      be say_hello(env: Env) => env.out.print("Hello B!")

    actor Main
      new create(env: Env) =>
        let a = A
        let b = B
        a.say_hello(env)
        b.say_hello(env)
    ```
  ]
)<pony-actors>

This shows the main difference between actors and classes: Actors have
_behaviours_ that can execute asynchronously while classes do not. It is
worth noting that code defined within an actor runs sequentially. It is
never the case that two different behaviours of the same actor (or the same
behaviour called twice) will run concurrently.

== Reference Capabilities

Pony's approach to concurrency is to represent concurrent parts of a program
with actors and their associated behaviours. State can be passed around
actors but requires knowledge of reference capabilities. The "Why Pony?"
section of their website@whypony provides simple reasons, summarised here, as
to why reference capabilities were added to Pony.
- _Mutable state is hard_. - In traditional concurrent programs, two or more
  threads of execution could potentially update a shared variable at the same
  time. Sharing mutable state between threads of execution can be hard to get
  right.
- _Immutable data is easy_. - If the state consists of data that cannot be
  changed, it is safe to share between threads of execution.
- _Isolated data is safe_. - If data is isolated (there is only one variable
  reference to it in existence), then it is safe to update. No other thread
  could potentially read or write to it.

\

There are a total of six reference capabilities in Pony. The table from the
"Reference Capabilities > Reference Capabilities Matrix" section of the Pony
tutorial@ponytut is recreated below:
#show table: set par(justify: false)
#figure(
  caption: [Reference Capabilities],
  table(
    columns: (auto, auto, auto, auto),
    table.header(
      [],
      [Deny global read/write aliases],
      [Deny global write aliases],
      [Don’t deny any global aliases]
    ),
    [*Deny local read/write aliases*],[*iso*],[],     [],
    [*Deny local write aliases*],     [trn],  [*val*],[],
    [*Don’t deny any local aliases*], [ref],  [box],  [*tag*],
    [],[Mutable],[Immutable],[Opaque],
  )
)

The reference capabilities in bold are the _only_ ones that allow sharing data
across actors:

#figure(
  caption: [Sendable reference capabilities.],
```pony
actor A
  // Keep local `tag` alias to msg. `tag` can alias all of iso, val,
  // and tag.
  var msg: String tag = String
  be set_msg_iso(s: String iso) => msg = s
  be set_msg_val(s: String val) => msg = s
  be set_msg_tag(s: String tag) => msg = s

actor Main
  new create(env: Env) => None
```
)

The above code shows the three _sendable_ reference capabilities in action.
Each of the behaviours for ```pony A``` can be sent data that is either
annotated with the corresponding referrence capability. There is also a sort
of heirarchy between reference capabilities in that data tagged with
```pony iso``` can be sent as ```pony tag```,
```pony ref``` and ```pony val``` can be sent as ```pony box```, and all
capabilities can be sent as ```pony tag```. Take the bellow code. It will not
compile due to the use of ```pony ref``` as the reference capability attached
to the ```pony s``` parameter:

#figure(
  caption: [Sendable reference capabilities.],
```pony
actor A
  var msg: String tag = String
  be set_msg_ref(s: String ref) => msg = s

actor Main
  new create(env: Env) => None
```
)

Also, if we want to share ```pony iso``` data, but keep it as ```pony iso```
or _convert it to a non-```pony tag``` reference capability_, we can do that
by "consuming" the alias as we pass it along. This is necessary to maintain
the guarantees of ```pony iso``` as the _only existing reference_ to that
piece of data:

#figure(
  caption: [
    Passing around ```pony iso``` data. Only one alias to ```pony iso```
    data is allowed to exist so we have to ```pony consume``` aliases we
    no longer need. The ```pony recover <refcap> ... end``` syntax tells
    pony to give us data with the `<refcap` capability instead of
    whatever the default is.
  ],
```pony
actor A
  // Note that we now keep a local `String iso` field by consuming
  // the `s` parameter.
  var msg: String iso = String
  be set_msg_iso(s: String iso) => msg = consume s

actor Main
  new create(env: Env) =>
    let a = A
    // Strings are `ref` by default.
    // `recover iso ... end` tells Pony we want an `iso` string.
    let s: String iso = recover iso String end
    a.set_msg_iso(consume s)
```
)

= Implementations and Results

As mentioned before, there are two implementations. The C++ version uses
OpenMP and its `parallel for` constructs to parallelize the fluid simulator.
The Pony version makes use of actors for parallelization. We begin by taking
a look the basic design.

#pagebreak()

== The Common Plan

The overall approach to simulating particle based fluids is as follows:
+ Initialize the particles once at the beginning.
+ Iterate through each particle and:
  + For every other particle:
    + Test if its distance is within the support radius $h$.
    + If it is, use equation @density-sph to calculate its density
      contribution.
  + Calculate pressure using the accumulated density value.
+ Iterate through each particle a second time:
  + For every other particle:
    + Test if its distance is within the support radius $h$.
    + If it is, use equations @pressure-sph and @viscosity-sph to
      calculate both its pressure force and viscosity force
      contribution.
+ Perform one last iteration of each particle:
  + Use the accumulated forces to calculate the particle's acceleration,
    velocity, and ultimately, position.
+ Draw each particle on screen.


-----

  + message passing
+ Scheduler threads
  + Work stealing
+ Reference Capabilities
  + iso, val, ref, tag
  + box, trn
  + aliasing (e.g. safe to alias iso as tag (iso!))

/*
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
*/

= ♪I Got 99 Problems and a Pony Ain't One...♪ It's Three.

*!! The Pony version consumes memory like crazy! (All those copies made I
guess) I don't have time to fix it. !!*

Aaaaaand we hit the first problem virtually immediately! Pony does not have a
way to directly exit the program.
+ Want to quit if SDL initialization fails.
+ No exit() without C FFI.
+ Talk about quiescence.

The second issue we run into is that many SDL3 functions expect to be run from
the main thread. This is an issue for Pony as actors are not guaranteed to
always run on the same thread.
+ Scheduler threads.
+ Message Queue.
+ Work stealing.
+ actor_pinning package.
+ We could just hog a thread with a while loop.
  - What about single core machines?
  - Lose the ability to use _all_ cores on a machine.

// See @pony:main.pony:pinning for cool code.


#bibliography("bibliography.bib")


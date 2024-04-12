
# Calm Display

This is the latest attempt by ITGaz (geefr) to rework the rendering code.

Please don't use it outside ITGMania, specifically the 'clarity' branch of https://github.com/geefr/ITGMania

At least until it's done cooking, the contents of this folder is licenced as GPL3

# Status

Very early prototype. Currently CalmDisplayOpenGL can render a single clear colour to the window, and that's it.

# Concepts

While my previous attempt at an OpenGL 4.x RageDisplay directly implemented RageDisplay, it had
to map the engine's draw path (built on OpenGL 1.x) over to OpenGL 4.x. Essenitally emulating the
ancient GL functions using modern tech.

This worked to a degree, but meant that I was trying to beat NVidia at their own game - I got good
performance, but not better than throwing the old OpenGL calls at the driver. Similar results on
Intel and AMD, with their respective quirks.

Calm is a fresh take, including modifications to Rage / the Actor's draw path.
New capability is kept somewhat distinct to allow new concepts to be used, with the concept of
Drawables / Draw commands used for the transation - The Actor's draw path should provide
a set of persistent Drawables, which act as the scene graph for CalmDisplay to render.

Initially I'm only implementing an OpenGL path, and probably with quite limited performance e.g.
not including instanced rendering or complex batching mechanisms, just mapping the Actors over
to a set of drawables, and handling the challenges of depth slice / z management.

On the stepmania / rage side of things, the Calm display is exposed as `DISPLAY2`. Typical logic
for an actor is then to create drawables `if(DISPLAY2)`, and otherwise execute the current
draw path against `DISPLAY`.


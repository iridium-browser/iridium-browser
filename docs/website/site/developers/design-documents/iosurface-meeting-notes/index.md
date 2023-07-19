---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: iosurface-meeting-notes
title: iosurface-meeting-notes
---

How Chrome uses IOSurfaces:

- Three processes involved in rendering: browser process (owns window), renderer
process (sandboxed), GPU process (executes OpenGL)

- GPU process has to provide rendering results to browser process

- Upon window resize, GPU process allocates an IOSurface, uses IOSurfaceGetID to
get global identifier

- Sends ID to browser process, which uses IOSurfaceLookup to turn it into an
IOSurfaceRef

- Both processes use CGLTexImageIOSurface2D to bind IOSurface to an OpenGL
texture

- GPU process binds texture to an FBO, renders into it using OpenGL

- Browser process texture maps that IOSurface onto a triangle pair

- (When window is resized, both GPU and browser processes use CFRelease against
IOSurfaceRef to unreference it)

Note:

- We found early on in Chrome's development that it didn't appear necessary to
double-buffer our IOSurfaces

- For this reason, currently allocate one IOSurface per tab. Didn't want to
double VRAM requirement unnecessarily.

Q. What are the buffering semantics of IOSurfaces? Double-buffered?
Single-buffered? Is there locking going on internally to make them appear
double-buffered?

A. Semantically single-buffered.

Basically, rendering correctness is determined just by how the command buffers
are serialized to the GPU.

This means if the browser process manages to squeeze in a command buffer which
draws the IOSurface to the screen in between the GPU process's command buffers
filling the IOSurface, we could possibly see an incomplete rendered frame.

Q. Is it possible to change these semantics with dictionary properties during
IOSurfaceCreate?

A. No.

Q. Should Chrome double-buffer its IOSurfaces (i.e., avoid rendering into one
while concurrently sampling from it in another process)? Would doing so avoid
any driver instability (window server lockups, kernel panics) we have seen on
various OS X hardware and software configurations? I note that both Firefox and
the Qt WebKit port are double-buffering their IOSurfaces.

A. Double-buffering probably would only improve the "race" between rendering to
and from the IOSurface.

Q. When are IOSurfaces released? If two processes both release their
IOSurfaceRefs, is the IOSurface reclaimed immediately? In a deferred fashion?

A. Should go away immediately. Subject to outstanding live texture objects that
reference that IOSurface.

It would be possible to re-look up the IOSurface in this case. (\* Maybe this is
the reason Vangelis was able to do this in his testing. \*)

You have to call glDeleteTexture, or delete the OpenGL context, to ensure the
IOSurface bound to the texture is gone.

Can run: ioreg -n IOSurfaceRoot -w 0

Has very verbose output, but it could be parsed.

A good indicator that we aren't leaking IOSurfaces is that the IOSurface IDs
don't increase monotonically. If they are increasing, it's an indication
something is leaking somewhere.

Q. At what point does the IOSurface allocation fail? We had a leak at one point,
and at some point IOSurface allocation would fail.

A. You could exhaust the kernel's address space. Otherwise, there's currently no
hard-coded limit.

When you create an IOSurface, it doesn't allocate anything except system memory.
You're not really using those pages right off the bat.

Until you actually create a texture object and use it with a particular GPU, it
doesn't allocate VRAM, either.

You need to worry about texture lifetimes across processes; the IOSurface can
get read back into system memory if the texture is deleted but the IOSurface is
stil alive!

Whenever the last texture reference is torn down, you're probably paying the
cost of paging the VRAM back to system memory before the IOSurface is
deallocated.

There is a way to tell IOSurface to auto-purge if you're willing to track a
system-wide use count. Any time the use count goes to 0, purge the VRAM and
system memory. Requires more cross-process lifetime management.

Q. Is fragmentation something we should worry about?

A. Not really. Nothing you can do at the user level about it.

In general, the working set of the GPU will be much larger than the few
IOSurfaces you allocate. But: Chrome is allocating up to 8 IOSurfaces per
window...

Q. In 10.7, there appear to be new APIs that indicate whether the IOSurface is
in use.

A. Each process referencing an IOSurface can increment and decrement its use
count. When the process's use count goes from 0 to 1, traps into the kernel to
reference the IOSurface at the system level.

IOSurfaceIsInUse() is the only call that should be used to determine usage --
according to IOSurfaceIncrementUseCount / IOSurfaceDecrementUseCount. You can't
query how many system-level references exist for a given IOSurface.

In a system that's quiescent, (i.e., running just the Finder, and in particular
not Safari), should be able to make some guarantees that IOSurfaces are all
released.

Could potentially use ioreg in conjunction with "wc -l" on a system with a known
configuration (i.e., a GPU bot) to assert that there are no leaked IOSurfaces.

One thing you do have to worry about: IOSurface doesn't let you get past any of
the limitations of the GPU. For example, render size limits. Can only render to
a surface so big. There is the fragmentation issue. The IOSurface does have to
be allocated contiguously.

Q. We're not using IOSurfaceAlignProperty.

A. Not necessary. Implementation will pick an aligned value for e.g. the row
bytes if the value is not specified.

Q. What kind of user errors can lead to IOSurface leaks? For example, if you
release the IOSurface but still have an OpenGL texture linked to the IOSurface,
and don't delete the OpenGL texture, the IOSurface still appears to be valid.

A. Already answered above.

Q. Are IOSurfaces more expensive resources to allocate than OpenGL textures
(accessible only to one process)? If so, how much more expensive?

A. No. They might not be as performant -- due to how the GPU memory is allocated
-- but they're basically as cheap as textures.

Q. During CGLTexImageIOSurface2D call: you can change how the memory as accessed
-- but can you change how much of the IOSurface you use?

A. Theoretically you could pick a smaller region of the upper-left-hand corner
-- but you can't pick an arbitrary sub-rectangle.

Q. Chrome currently renders IOSurfaces to the screen using OpenGL, which is
difficult to make seamlessly interoperate with Cocoa. Currently Chrome has to
generate a "mask" in Cocoa and render the OpenGL as an underlay in order for
Cocoa to correctly occlude web page rendering when necessary. In the past,
attempted to use a layer-backed view for the root view, but had occlusion
problems such as the find bar. Are there better ways, or ways more compatible
with CG, to draw IOSurfaces to the screen? Alternative code paths like IOSurface
-&gt; CIImage? Also, we switch back and forth between software-rendered NSViews
and those into which we render an IOSurface using OpenGL.

A. Not really. There's also a way to back a CALayer to an IOSurfaceRef nowadays
-- but this wouldn't really save a blit. It might save \*Chrome\* a blit, but CA
would be doing it internally.

Could consider rendering to a single-buffered NSOpenGLContext in the browser
process!

Q. Is the sharing of Core Animation layer trees across processes (which Safari
does via SPI, apparently) more efficient than using IOSurfaces?

A. Yes -- it is probably more efficient -- but we can't talk about what Safari
might be doing internally.

Q. Texture corruption during paging

A. Should re-verify existing Radars (MapsGL) against 10.8.2 seed on current
hardware.

Q. Is there something Chrome might be doing obviously wrong in its rendering
pipeline which is provoking window server hangs and kernel panics more often
than other OpenGL applications?

A. Watch out for lifetime management of stuff. Leaks are bad. Will provoke
paging that otherwise wouldn't have to do.

Check cross-process GL texture lifetime.

Aside from that, nothing obviously architecturally wrong.

Q. Are window server hangs like http://crbug.com/140175 definitely Apple bugs,
or is it expected that applications might be able to provoke this behavior?

A. Window server hangs are Apple bugs. Please file them. The more Radars the
better.

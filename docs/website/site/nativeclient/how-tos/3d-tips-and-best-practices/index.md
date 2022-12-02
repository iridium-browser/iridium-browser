---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: 3d-tips-and-best-practices
title: 3D Tips and Best Practices
---

# Pepper 3D on Chrome provides a secure implementation of OpenGL ES 2.0. Here are some tips for getting maximum performance.

## Don’t update indices

For security all indices must be validated. If you change them we have to
validate them again. Therefore structure your code so indices are not updated
often.

## Don’t use client side buffers

In OpenGL ES 2.0 you can use client side data with glVertexAttribPointer and
glDrawElements. It’s REALLY SLOW! Don’t use them. Instead, whenever possible use
VBOs (Vertex Buffer Objects). Side-note: Client side buffers have been removed
from OpenGL 4.0.

## Don’t mix vertex data with index data.

Actually this is off by default. In real OpenGL ES 2.0 you can create a single
buffer and bind it to both GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER. In
Pepper 3D, by default, you can only bind buffers to 1 bind point. There is the
option to enable binding buffers to both points. Doing so requires expensive
work so don’t do it.

## For dynamic textures (ie, video) or dynamic vertex data (skinning / particles) consider using CHROMIUM_map_sub

<http://src.chromium.org/viewvc/chrome/trunk/src/gpu/GLES2/extensions/CHROMIUM/CHROMIUM_map_sub.txt>

## Don’t call glGetXXX or glCheckXXX during rendering.

Calling either of those stalls our multi-process pipeline. This is normal advice
for OpenGL programs but is particularly important for 3D on Chrome. This
includes glGetError – avoid calling it in release builds.

## Make sure to enable Attrib 0.

In OpenGL you MUST enable Attrib 0. In OpenGL ES 2.0 you don’t have to enable
Attrib 0. What that means is that in order to emulate OpenGL ES 2.0 on top of
OpenGL we have to do some expensive work.
In practice most programs don’t have an issue here but just in case, the most
obvious way this might bite you is if you bind your own locations and don’t
start with 0. Example: Imagine you have a vertex shader with 2 attributes
“positions” and “normals”

> glBindAttribLocation(program, “positions”, 1);

> glBindAttribLocation(program, “normals”, 2);

Those 2 functions would make make your shader NOT use attrib 0 in which case
we’d have to do some expensive work internally

## Minimize the use of glFlush and avoid using glFinish

It is generally good practice to minimize explicit calls to glFlush and avoid
using glFinish. Particularly so on Native Client where they incur additional
overhead.

## Avoid reading back output from the GPU to the client

In other words, don't call glReadPixels. This is slow.

**When benchmarking, avoid comparing results where one system is limited by
vsync and another is not.**

## Don’t use GL_FIXED

It’s not supported in OpenGL and so emulation for OpenGL ES 2.0 is slow. By
default GL_FIXED support is turned off Pepper 3D. There is the option to turn it
on. Don’t do it.

## Use a smaller plugin and let CSS scale it.

The size your plugin renders and the size it displays in the page are set
separately. CSS controls the size your plugin displays where as the width and
height attribute of your &lt;embed&gt; element control the size your plugin
renders.

## Use HTML where approriate

If you’re used to making native games you’re probably used to rendering
everything yourself. The browser though can already render text and UI very well
and it will composite that HTML with your plugin using all the standard HTML5
and CSS methods available.

**Avoid updating a small portion of a large buffer**

This is especially an issue in Windows where we emulate OpenGL ES 2.0 on top of
DirectX. In the current implementation, updating a portion of a buffer requires
re-processing the entire buffer. In other words if you make a buffer
(glBufferData) of 10000 bytes and then later call glSubBufferData to update just
3 of those bytes, all 10000 bytes will have to be re-converted. (Yea, I know,
lame)

2 suggestions:

1.  Separate static vertex data from dynamic. In other words, put your
            static data in 1 buffer and dynamic data in a different buffer. That
            way your static data won't have to be re-converted.
2.  Volunteer to fix the perf issues
            <http://angleproject.googlecode.com>

# General OpenGL advice

## Interleaved data is faster to render than non-interleaved

3 buffers of \[position,position,position\], \[normal,normal,normal\],
\[texcoord,texcoord,texcoord\] is slower than 1 buffer of
\[position,normal,texcoord,position,normal,texcoord,position,normal,texcoord\].

## Separate dynamic data from static

Assume you have positions, normals and texcoords. Further assume you update
positions every frame. It would be best to put positions in 1 buffer and normals
+ texcoords in a separate buffer. That way, you can call glBufferData or
glBufferSubData on a smaller range of data.

## glBindBuffer is expensive

Consider putting multiple meshes in a single buffer and using offsets (as long
as the buffers are static, see above)

## Check your extensions and max GL features

Not every GPU supports every extension nor has the same amount of textures
units, vertex attributes, etc. Make sure you check for the features you need.
For example, if you are using non power of 2 texture with mips make sure
GL_OES_texture_npot exists. If you are using floating point textures make sure
GL_OES_texture_float exists. If you are using DXT1, DXT3 or DXT5 texture make
sure GL_ETC_texture_compression_dxt1, GL_CHROMIUM_texture_compression_dxt3 and
GL_CHROMIUM_texture_compression_dxt5 exist.
If you are using textures in vertex shaders make sure
glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, …) returns a value greater than
0.
If you are using more than 8 textures in a single shader make sure
glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, …) returns a value greater than or
equal to the number of simulatious textures you need.
etc...

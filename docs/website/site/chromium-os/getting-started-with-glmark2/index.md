---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: getting-started-with-glmark2
title: Getting started with glmark2
---

[TOC]

## Introduction

This guide describes how to download, modify, and build the glmark2 suite.
glmark2 is an OpenGL (ES) 2.0 benchmark maintained by the Linaro Graphics
Working group. Their site is: <https://launchpad.net/glmark2>
This guide is intended for anyone who wishes to use glmark2 in Chromium OS, as
well as developers who wish to improve it.

## What is OpenGL ES?

OpenGL ES is a subset of the OpenGL API that is designed for embedded systems.
It lacks many of the functions of OpenGL. glmark2 supports both regular OpenGL
and OpenGL ES.
Read more about it:
<http://www.khronos.org/opengles>
<http://en.wikipedia.org/wiki/OpenGL_ES>

## How to build and run glmark2 on your Linux machine

Before running it on Chromium OS, you might find it useful to run glmark2 on
your Linux development workstation. You can build glmark2 on a Linux workstation
using the following steps:
In the terminal, go to a local folder and clone the upstream repo:

```none
git clone git://git.linaro.org/people/afrantzis/glmark2.git
cd glmark2
```

The instructions to build and install are in the INSTALL file. You can reference
that or follow these instructions:

```none
sudo -i                        # install as root
./waf configure --enable-gl    # setup for installing glmark2 with regular OpenGL
./waf                          # build it!
./waf install                  # installs files into /usr/local/bin and /usr/local/share/glmark2
```

Once installed, run glmark2 using the command:

```none
glmark2
```

## How to build and run glmark2 in Chromium OS

glmark2 has been incorporated into Chromium OS as a third party git repository,
so it can be built using emerge.
glmark2 depends on mesa. First, you need to build mesa with the following
command:

```none
USE="egl gbm gles1 gles2 shared-glapi" emerge-<board> mesa
```

\*\*Temporary workaround step: Save the glmark2 ebuild file that's attached to
this page (see below) into third_party/chromiumos-overlay/media-libs/glmark2.
Then build glmark2 using:

```none
USE="glesv2 drm" emerge-<board> glmark2
```

When this done, install it on your Chromium OS target machine. From the target
machine, run:

```none
gmerge -n mesa
gmerge -n glmark2
```

Once it has been installed, run glmark2 with one of the following commands:

```none
glmark2                    # with OpenGL
glmark2-es2                # with OpenGL ES v2
glmark2-es2-drm            # with OpenGL ES v2 and without X
```

## Developing for glmark2

In your Chromium OS development chroot, go to `~/trunk/src/third_party/glmark2`.
You can modify the source code here. To change build options, update
third_party/chromiumos-overlay/media-libs/glmark2/glmark2-9999.ebuild.
Once your changes are up and running, you can send them upstream.

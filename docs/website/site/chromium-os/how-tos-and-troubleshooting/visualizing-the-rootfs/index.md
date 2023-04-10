---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: visualizing-the-rootfs
title: Visualizing the Rootfs
---

An important aspect in working on an embedded OS is keeping the size of our
updates down which usually entails keeping the size of the OS image down. If
left unchecked, the OS image will increase steadily. In order to help developers
track down where space is going we created a tool cros_tree_map that helps
developers figure out where our precious space is going.

In order to use, you need to have:

1.  an OS image (the .bin file).
2.  at least a mini-layout checkout of the code.

To use:

1.  mount the image onto a directory -- you can do this easily with
            [mount_gpt_image.sh](/chromium-os/how-tos-and-troubleshooting/helper-scripts#TOC-mount_gpt_image.sh)
            (in `~/src/scripts`). You should use the `--safe` option to ensure
            you do not make accidental changes to the Root FS. By default it
            will mount the rootfs onto `/tmp/m`. You can use the `--from=<path
            to .bin file>`
2.  Run du: `sudo du -xB 1 /tmp/m > du.out` (`du.out` is your output
            file).
3.  Use cros_tree_map to create a json report:
            `~/chromite/contrib/cros_tree_map du --du-output du.out
            --strip-prefix /tmp/m > image_size.json`

At this point you will have a json file with all the directory information. To
visualize this info, you can use webtree map here:
<https://github.com/martine/webtreemap>. You can then view the html file from
your trusty web browser.

Here's an example snapshot of a Chrome OS image (note: yours will be interactive
so you can dig in to different areas).

![](/chromium-os/how-tos-and-troubleshooting/visualizing-the-rootfs/Screenshot%20from%202013-04-08%2013_46_48.png)

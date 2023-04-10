---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: order-file-development-guide
title: Order file development guide
---

1. Build chromeos-chrome with profile instrumentation

```none
export USE="$USE -reorder"
export EXTRA_BUILD_ARGS="order_profiling=1 order_text_section=''"
```

2. Add `--no-sandbox` to chrome start arguments (in the buttom of
/sbin/session_manager_setup.sh).

3. As you prepare, you may have run chrome collecting logs (which you don't want
yet). Remove them and reboot.

```none
rm /var/log/chrome/cyglog.*; sudo reboot
```

4. After you browse several sites (google.com, maps.google.com, youtube.com),
copy logs to your host machine to some directory.

5. Sanitize logs.

```none
head -1 cyglog.*
```

If some log does not contain header (containing /opt/google/chrome/chrome),
remove it. This log left from before you removed all cyglogs last boot.

6. Merge logs to a single file. This may take some minutes to run.

```none
$CHROME_SRC/tools/cygprofile/mergetraces.py `ls cyglog.* -Sr` > merged_cyglog
```

7. Symbolize the log using your instrumented image (from p.1).

```none
$CHROME_SRC/tools/cygprofile/symbolize.py -t orderfile merged_cyglog /build/x86-alex/opt/google/chrome/chrome > unpatched_orderfile
```

8. Rebuild chromeos-chrome with new orderfile.

```none
export USE="$USE -reorder"
export EXTRA_BUILD_ARGS="order_profiling=0 order_text_section='/home/$USER/trunk/src/scripts/cyglog/unpatched_orderfile'"
```

9. Patch the orderfile to

```none
$CHROME_SRC/tools/cygprofile/patch_orderfile.py unpatched_orderfile /build/x86-alex/opt/google/chrome/chrome > orderfile
```

9. If you want to visually check that the resulting image is reordered, use

```none
nm -Sn /build/x86-alex/opt/google/chrome/chrome | less
```

And note, that ChromeMain symbol is not at the beginning of symbols.

In the image built without reordering, ChromeMain immediately follows main and
is about 10th symbol in the image.

10. Update the repository containing orderfiles.

The repo, containing orderfiles is here:
http://git.chromium.org/gitweb/?p=chromiumos/profile/chromium.git

Intructions on how to update the repo:
http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/git-server-side-information

11. Update
src/third_party/chromiumos-overlay/chromeos-base/chromeos-chrome/chromeos-chrome-9999.ebuild
to grab the latest version of the orderfile (that you updated in p.10).

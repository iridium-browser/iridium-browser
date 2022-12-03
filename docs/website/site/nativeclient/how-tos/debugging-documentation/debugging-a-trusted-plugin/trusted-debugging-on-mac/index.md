---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
- - /nativeclient/how-tos/debugging-documentation
  - Debugging Documentation
- - /nativeclient/how-tos/debugging-documentation/debugging-a-trusted-plugin
  - Debugging a Trusted Plugin
page_name: trusted-debugging-on-mac
title: Debugging a Trusted Plugin on Mac
---

<table>
<tr>

1.  <td>Make an new Xcode project, use the GUI to make a Bundle project
            that uses Core Foundation.</td>
2.  <td>Add existing NaCl sources</td>
3.  <td>DEPS in ppapi from chromium, add the C++ sources - create a
            Group and add the ppapi files directly. Adding ppapi as a folder
            reference doesn't work.</td>
4.  <td>Set "Header search paths" to point to the chromium ppapi headers
            (|SDK_root|/third_party), NOT the built-in NaCl headers.</td>
5.  <td>Add the SDK root to "Header Search Paths"</td>
6.  <td>Build the plugin.</td>
7.  <td>Edit hello_world/hello_world.html so that the embed tag has
            type="application/x-hello-world" and no nacl= attribute.</td>
8.  <td>Instead of handling a onload event in the EMBED tag, you have to
            call moduleDidLoad() directly after the EMBED tag.</td>
9.  <td>Make sure to uncheck "Load Symbols Lazily" in the Debugging
            panel of Xcode preferences.</td>
10. <td>To debug, you have to use Chromium - best is to get a waterfall
            build from http://build.chromium.org/f/chromium/snapshots/Mac/. This
            style of debugging is not supported with Google Chrome Dev
            Channel</td>
    1.  <td>In Xcode, ctrl-click on "Executables" and select "Add Custom
                Executable…".</td>
    2.  <td>Call the new custom exec, say, "Chromium Dev"</td>
    3.  <td>Point it at the .app wrapper for Chromium that you got from
                the waterfall, e.g. ~/Downloads/chrome-mac/Chromium.app.</td>
    4.  <td>Add these arguments in the Arguments tab:</td>
        1.  <td>--user-data-dir=/tmp/nacl-debug-profile</td>
        2.  <td>--register-pepper-plugins="$HOME/Source/nacl-sdk/src/examples/hello_world/HelloWorld/build/Debug/HelloWorld.bundle;application/x-hello-world"</td>
        3.  <td>--single-process</td>
        4.  <td>file://$HOME/Source/nacl-sdk/src/examples/hello_world/hello_world.html</td>
11. <td>It is possible to debug a plugin using Chrome Dev channel, but
            it's a little more raw:</td>
    1.  <td>In a shell, run Chrome like this: Google\\
                Chrome.app/Contents/MacOS/Google\\ Chrome
                --user-data-dir=/tmp/nacl-debug-profile
                --register-pepper-plugins="$HOME/Source/nacl-sdk/src/examples/hello_world/HelloWorld/build/Debug/HelloWorld.bundle;application/x-hello-world"
                file://$HOME/Source/nacl-sdk/src/examples/hello_world/hello_world.html</td>
    2.  <td>In Chrome, create a new tab and visit about:memory, this
                will list the PID of the plugin tab.</td>
    3.  <td>In Xcode, Run -&gt; Attach To Process, then pick the
                appropriate PID.</td>
        1.  <td>Note: if you get various errors about formatting, just
                    click "Continue"</td>

</tr>
</table>

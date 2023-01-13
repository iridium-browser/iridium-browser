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
page_name: debugging-a-trusted-plugin-on-linux
title: Debugging a Trusted Plugin on Linux
---

1.  Get a Chromium check-out. You can usually use the revision from the
            nacl-sdk DEPS file.
2.  You don’t need to build all of chromium, just pepper.
    1.  Go to the src/ directory in your check-out
    2.  type: ‘make ppapi_tests’ to build the pepper tests as well as
                any libraries they depend on. You’ll end up with libppapi_cpp.a
                and libppapi_cpp_object.a.
3.  Build trusted version of the code
    1.  For a C plugin you don’t have to link against anything. ppapi C
                is all headers.
    2.  For C++, you need libppapi_cpp.a and libppapi_cpp_objects.a.
                Link against ppapi_cpp_objects and ppapi_cpp from chrome.
        1.  link order matters: ppapi_cpp_objects has to come before
                    ppapi_cpp
    3.  Be sure to specify -fno-rtti and -fPIC since chrome does not
                build run-time type information by default, and you’ll be
                generating a shared library as your plugin.
    4.  For the linking step, be sure to specify -shared
4.  To load your plugin, your application will need an embed tag. For
            trusted plugins this should not have a “nacl” but a regular “type”
            attribute, for example: type="application/x-hello-world"
5.  Unlike with an untrusted plugin, instead of handling a onload event
            in the EMBED tag, you have to call moduleDidLoad() directly after
            the EMBED tag.
6.  To debug, you have to use Chromium - best is to get a waterfall for
            your platform build from
            http://build.chromium.org/f/chromium/snapshots/ You can also finish
            the chromium build you checked out but be prepared to wait a while.
            This style of debugging is not supported with Google Chrome Dev
            Channel
7.  In a shell, launch chrome with the following arguments:
    1.  --user-data-dir=/tmp/nacl_debugging_chrome_profile
    2.  --register-pepper-plugins="location/of/your/plugin.so;application/x-hello-world"
    3.  --single-process
    4.  file://location/of/your/web_page.html
8.  In Chrome, create a new tab and visit about:memory, this will list
            the pid of the plugin tab.
9.  You can now use gdb to attach to the pid and debug your plugin

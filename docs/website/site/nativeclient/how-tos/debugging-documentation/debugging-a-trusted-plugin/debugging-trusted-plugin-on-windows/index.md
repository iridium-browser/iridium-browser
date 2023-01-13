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
page_name: debugging-trusted-plugin-on-windows
title: Debugging a Trusted Plugin on Windows
---

1.  Get VS2008
2.  Set up the project/Solution using the wizard: make a Win32 Console
            app, and then click through the wizard to modify the project to be a
            Win32 DLL, that exports symbols. Visit here for more info:
            <http://msdn.microsoft.com/en-us/library/ms235636(v=vs.80).aspx>
3.  In the project properties, turn off precompiled headers. Do this so
            you can use the same #include stack in your sources to build both
            untrusted and trusted.
4.  Note: I ran into problems using the ppapi headers that come with
            NaCl:
    1.  1&gt;d:\\native_client_sdk\\toolchain\\win_x86\\nacl64\\include\\machine\\_types.h(19)
                : fatal error C1083: Cannot open include file:
                'native_client/src/include/portability.h': No such file or
                directory
    2.  I tried -D__native_client__, but this produced even more errors.
    3.  To resolve this, I copied toolchain/win_x86/nacl64/include/ppapi
                to sit at the same level as examples
5.  In the project properties, add &lt;native_client_sdk&gt; to the
            header search paths.
6.  After copying the ppapi headers, the project still won't build
            because I need to copy in the ppapi C++ sources and build them too.
            I DEPSed in the ppapi sources from nacl/ppapi and added all the
            files in ppapi/cpp/\*.cc to the VS project.
7.  I had to change some source files. When you build under nacl-gcc,
            things like inttypes.h (and stdint.h) are automagically part of the
            #include chain. This is not so when building trusted. In this case,
            I added #include &lt;ppapi/c/pp_stdtype.h&gt; to the files that
            needed it. (Note that on Windows, there does not seem to be a
            &lt;stdint.h&gt; or &lt;inttypes.h&gt;).
8.  Edit hello_world/hello_world.html so that the embed tag has
            type="application/x-hello-world" and no nacl= attribute.
9.  Instead of handling a onload event in the EMBED tag, you have to
            call moduleDidLoad() directly after the EMBED tag.
10. Run the local HTTP server in examples.
11. Once the DLL is built, run chrome
            --register-pepper-plugins="d:\\native_client_sdk\\examples\\NaClExamples\\HelloWorld\\Debug\\HelloWorld.dll;application/x-hello-world"
            --user-data-dir=d:\\trusted-debug-profile
            --wait-for-debugger-children
12. Visit localhost:5103 and run the hello_world example.
13. To debug the DLL you have to attach to the right process. It isn't
            clear how you find this, other than by guessing. I had limited
            success in pulling up the Debug -&gt; Attach... panel, then
            launching chrome and visiting
            localhost:5103/hello_world/hello_world.html, then hitting Refresh on
            the Attach panel and attaching the the new "chrome.exe" process. I
            have not been able to figure out how to debug startup issues. There
            are some extra debugging hints here:
            <http://www.chromium.org/developers/how-tos/debugging>

Implications:

1.  The SDK will have to DEPS in and bundle src/ppapi from the chromium
            project.
2.  When building a trusted plugin, you have to use the chromium ppapi
            headers and build the .cc files, then switch over to different
            headers and link with libppapi_cpp.a to build a .nexe. We could
            automate some of this by adding a build step in the SDK to build
            libppapi_cpp.a and bundling that library.
3.  **Potential show-stopper**: on Windows, there is no built-in
            pthreads library. This means the pi_generator example will not
            build, nor will any other app that uses pthread, unless we can find
            a pthreads lib that we can re-distribute.
4.  **Potential show-stopper**: None of this work allows for .dso's,
            which are ELF. These will not load on Windows. Not sure if it's
            possible to make .dso's into DLLs and mimic the dynamic loading
            process. dlopen() will not work for the same reason that pthreads
            don't work (no Windows support).

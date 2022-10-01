---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/serviceworker
  - Service workers
page_name: service-worker-faq
title: Service Worker Debugging
---

**Q: How do I debug?**

A: From a page on the same origin, go to Developer Tools &gt; Application &gt;
Service Workers.

You can also use chrome://inspect/#service-workers to find all running service
workers.

To poke around at the internals (usually only Chromium developers should need
this), visit chrome://serviceworker-internals .

**Q: When I have Developer Tools open, requests go straight to the network; the
Service Worker does not get a fetch event.**

A: On the Developer Tools' Network tab, if Disable cache is checked, requests
will go to the network instead of the Service Worker. Uncheck that.

**Q: I get an error message about "Only secure origins are allowed". Why?**

A: Service workers are only available to "secure origins" (HTTPS sites,
basically) in line with a policy to [prefer secure origins for powerful new
features](/Home/chromium-security/prefer-secure-origins-for-powerful-new-features).
However http://localhost is also considered a secure origin, so if you can,
developing on localhost is an easy way to avoid this error.

You can also use the --unsafely-treat-insecure-origin-as-secure command-line
flag to explicitly list a HTTP Origin. For example:

**$ ./chrome
--unsafely-treat-insecure-origin-as-secure=http://your.insecure.site:8080**

([Prior to Chrome
62](https://chromium.googlesource.com/chromium/src/+/55dab613843031ab360193116f5d80d0d23308fb),
you must also include the --user-data-dir=/test/only/profile/dir to create a
fresh testing profile for the unsafely-treat-insecure-origin-as-secure flag to
work.)

If you want to test on https://localhost with a self-signed certificate, do:

**$ ./chrome --allow-insecure-localhost https://localhost**

You might also find the --ignore-certificate-errors flag useful.

**Q: I made a change to my service worker. How do I reload?**

A: From Developer Tools &gt; Application &gt; Service Workers, check "Update on
reload" and reload the page.

**Q: I have a different question.**

A: Reach us out via
[service-worker-discuss](https://groups.google.com/a/chromium.org/forum/#!forum/service-worker-discuss)
or Stack Overflow with
"[service-worker](http://stackoverflow.com/questions/tagged/service-worker)"
hash tag.

## Experimental Service Worker Debugging functionality

### Step-wise SW debugging

Chrome 44 required, but no experiments needed. Debugging message flow using
conventional breakpoint debugging on both ends (page and the sw). You can kill
the worker to debug its installation, you can debug the way it serves resources,
etc.

### Active SW in Sources

*   Navigate to a page w/ service worker, opens DevTools
*   Worker is displayed in the Threads list
*   Service Workers tab lists Active Running service workers this page
            (its iframes) belong to

<img alt="image"
src="https://lh6.googleusercontent.com/e4cfdjN0GVnRqgeVr85oZ4FajaI5Lc8U-pkfTWoJ8HxyQ1Tigg40nzfpDDdRlEUIPu7M68wGM0qmsvJz-wu6lDvwInvNmfW-d8wP3gKpXCOomU-VTNVtH_VTWgmuZFTXJ8vU24A">

### Console execution

*   Console displays contexts for frames as well as workers
*   One can execute expressions in the SW context and filter by context

<img alt="image"
src="https://lh6.googleusercontent.com/HIP54ymxanAUqevqNfK4FzzgK3BsQSxoUJEM70K7i3oiJYImMvW9igdCXMwsMSLv1Uys6w6TObJdjCMklr0Eq_qW3BEpkHCyiDT7scgVz0ytNX7ma7bY9HUrAPdD6s-rPMEfpqc">

<img alt="image"
src="https://lh5.googleusercontent.com/UtOGH7s6zefhEmr9-uYHqKwhOPT6Iup5qM8Gy2FcA59FLiL2l-lw2zhEAiTK_vuiHpJjHSaptlvyXEINS6s2NyGBC_eVBECStddOdLQqJtxjOpCkJ_gkA_om7JtaC0JrruOzl9s">

### Console errors

*   Should there be any installation errors (syntax errors in main
            script), they are available in the page console
*   Clicking on the error message reveals the erroneous script

<img alt="image"
src="https://lh6.googleusercontent.com/lYsWZUJ21KTbr23up6cV6gKkYQOrQ0VwORZ6Z_zPzrXhAaKzqFTVeEbRK3w6srjfS_gcb_aLvHrSEdEIe5KvC1Hm7oVHqac4RwUeVlhe2cpYJF_8fCZSzz9CcBLPVVymV_kXlvo">

### Breakpoints

*   User can set breakpoints in the service worker scripts
*   Should user set the breakpoint, Stop the service worker and reload
            the page, service worker will stop on that breakpoint

<img alt="image"
src="https://lh5.googleusercontent.com/F9kDEzN4Bsec-TL5HOcUdRh_JvYrw7C0IjWQ0Kz8Usq-v6ZwlLlO8Ve-DBeTYrP0ETY5jjrOE2Ms2Df-LgYRktMBfJuSaCz-t4lpENLYivqgUqY9wH7qFoYGudOt0Xg0jQ2FxC8">

## SW debugging panel in Resources

1.  Turn on DevTools experiments (in `about:flags`)
2.  In DevTools, Go to Settings -&gt; Experiments, hit Shift 6 times
3.  Check "Service worker inspection", close and reopen DevTools
4.  Now you have this experiment enabled.

This experiment unlocks a view in Resources just like you asked.
[<img alt="image"
src="/blink/serviceworker/service-worker-faq/resources-sw.png">](/blink/serviceworker/service-worker-faq/resources-sw.png)
Currently, to view network activity of the worker, click the "inspect" from
Resources to launch a dedicated devtools window for the worker itself.

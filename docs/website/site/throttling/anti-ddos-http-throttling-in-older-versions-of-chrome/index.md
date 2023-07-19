---
breadcrumbs:
- - /throttling
  - Anti-DDoS HTTP Throttling of Extension-Originated Requests
page_name: anti-ddos-http-throttling-in-older-versions-of-chrome
title: Anti-DDoS HTTP Throttling in Older Versions of Chrome
---

Chrome 12 through Chrome 19 had anti-DDoS HTTP throttling not only for requests
originating from extensions, but for normal web page requests as well. To
disable throttling in versions of Chrome up to 20.0.1115.2: Type
**chrome://net-internals/#httpThrottling** into the address bar and uncheck the
checkbox labeled "Throttle HTTP requests if the server has been overloaded or
encountered an error."

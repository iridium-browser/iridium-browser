---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: flakiness-dashboard
title: Flakiness Dashboard HOWTO
---

## What was it?

The flakiness dashboard was a web app that showed test history information in a
grid with different colors for passes, failures, and flakes.

Test history is available directly in the build UI, e.g.
[this view](https://ci.chromium.org/ui/test/chromium%3Aci/ninja%3A%2F%2F%3Ablink_web_tests%2Ffast%2Fforms%2Fplaintext-mode-2.html)

To browse top flake clusters and see their impact in various contexts see
[LUCI Analysis](https://luci-analysis.appspot.com/p/chromium/clusters).

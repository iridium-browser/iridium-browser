---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: isolated-testing
title: Isolated Testing
---

[TOC]

## What's happening

The Chrome team is in the process of retro-fitting the ability of running tests
outside a checkout to scale out testing so the tests results are returned
faster.

## Design documents

The whole LUCI/Swarming/Isolate project lives in its own project at
[github.com/luci](https://github.com/luci). The [design docs on the
wiki](https://github.com/luci/luci-py/wiki) are a recommended reading since it
gives background about the rationale why is it done in the first place.

## User guide

[I want to convert my test or use the
infrastructure](/developers/testing/isolated-testing/for-swes) &lt;- most likely
this one.

## End goal

[Deduplicate test execution via deterministic
builds](/developers/testing/isolated-testing/deterministic-builds)

---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/telemetry
  - 'Telemetry: Introduction'
page_name: add_a_measurement
title: 'Telemetry: Add a Measurement'
---

[TOC]

## Are you sure you want to add a measurement?

Writing a good measurement is really hard. We already have a bunch of
measurements in
[tools/perf/measurements](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/measurements/)
for you to use. If you are looking to write a new benchmark, you probably just
need to add or modify an existing story set and you might need to add new
metrics to an existing measurement. Think twice before you write another
measurement.

## What is a measurement?

A measurement is a python class that obtains a set of Numbers about a Page.
Measurements must work for all pages, not just one specific page.

For example, let’s say my users often go to a specific page, click a button, and
scroll a div that shows up. I could measure any number of things:

    How long it takes to go to that URL in a fresh tab until the div shows up

    How smoothly I can scroll a div

    How much time I spend in key WebKit systems: javascript, style, layout,
    paint, compositing

    How much the memory footprint varies as I scroll the div

    How much time I spend decoding images and painting the page

These are all examples of measurements.

## How about Timeline-Based Measurement?

Timeline-Based Measurement is a new and preferred way for computing Telemetry
metrics from traces in a unified manner. The code is contained in
[tools/telemetry/telemetry/web_perf](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/web_perf/).

For details about this approach, see the following documents: [Why choose
TimelineBasedMeasurement over other
measurements?](https://docs.google.com/document/d/10G0PbePQOwJao57Mu6Xr7Fx0o2ng8fwZ33wUqpnsyog)

[How does TimelineBasedMeasurement
work?](https://docs.google.com/document/d/1cx_yplQQUVtIka5DD846kcfheIdIoTHfCL0dH93ezA8/edit?usp=sharing)

## I think I need to write a new measurement, now what?

Please email [telemetry@chromium.org](mailto:telemetry@chromium.org) with
details about what you are trying to measure. We can help you get started and
ensure that there are not existing measurements that you could use.

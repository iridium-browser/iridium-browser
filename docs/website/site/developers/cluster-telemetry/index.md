---
breadcrumbs:
- - /developers
  - For Developers
page_name: cluster-telemetry
title: Cluster Telemetry
---

[Run on Cluster Telemetry!](http://ct.skia.org/) (Google only, sorry)

**Cluster Telemetry 101**

[Telemetry](https://github.com/catapult-project/catapult/tree/master/telemetry)
is Chrome's performance testing framework, using it you can perform arbitrary
actions on a set of web pages and report metrics about it.

**Cluster Telemetry** allows you to run telemetry's benchmarks using multiple
repository patches through Alexa's top 10k and top 100k web pages.

Developers can use the framework to:

*   Measure the performance of multi-repo patches against the top subset
            of the internet on Android devices, bare-metal linux machines, and
            windows GCE instances using the [perf
            page](https://ct.skia.org/chromium_perf/).
*   Gather metrics for analysis or for reports against 100k web pages on
            CT's VM farm (linux and windows) using the [analysis
            page](https://ct.skia.org/chromium_analysis/).
*   Compute metrics on an analysis run using the [metrics analysis
            page](https://ct.skia.org/metrics_analysis/).

**Should I use CT?**

If you would like to know how your patch impacts Chrome's performance or would
like to do analysis against a large repository of real world web pages then you
should try out Cluster Telemetry.

CT has been used to gather analysis data for the following projects:

*   Web vitals performance
*   Ad tagging accuracy
*   UserCounter analysis
*   Real-world leak detector
*   Gather data for a language detector
*   TTI variability study
*   Analysis of 5M SVGs
*   Stat rare usages

CT has been used to gather perf data for the following projects:

*   Slimming paint
*   Clustering top web sites for Chrome graphics
*   Performance data for layer squashing and compositing overlap map
*   SkPaint in Graphics Context
*   Culling
*   New paint dictionary

**Which telemetry benchmarks does CT support?**

CT supports all benchmarks listed in
[tools/perf/contrib/cluster_telemetry](https://source.chromium.org/chromium/chromium/src/+/HEAD:tools/perf/contrib/cluster_telemetry/;l=1?q=contrib%2Fcluster&sq=&ss=chromium&originalUrl=https:%2F%2Fcs.chromium.org%2F).
Many of these benchmarks exist outside of CT, but these are the corresponding CT
compatible versions.

CT also allows you to run against unlanded / modified benchmarks using the
multi-repo patches supported in the UI.

Can try out your benchmark's CT version locally with:

`python tools/perf/run_benchmark ${YOUR_BENCHMARK} --also-run-disabled-tests
--user-agent=desktop
--urls-list=`[`http://www.google.com`](http://www.google.com/)`
--archive-data-file=/tmp/something --output-dir=/tmp/output/
--browser-executable=/usr/bin/google-chrome --browser=exact --pageset-repeat=1
--output-format=csv --use-live-sites`

**How accurate are CT's results?**

For an empty patch repaint run on Desktop, these are the results:

<img alt="consistent-results.png"
src="https://lh4.googleusercontent.com/WrHyPEmSptfgO28zEsnN5e1tM-SFXAtlsQoPnKEghkgSZye19OSoiER6CRoQ9D00dSLWT6Kwxhra3YY3aFx743Tl6rNRrkcmITmv23wQkiCouvLNBDwcoUPly9WWRADcraqWfSE"
height=157px; width=624px;>

The overall results from Cluster Telemetry runs are accurate within a percentage
point.

The per webpage results (visible when you click on a field) do have some
variance, but this has been greatly improved due to efforts detailed
[here](https://docs.google.com/a/chromium.org/document/d/1GhqosQcwsy6F-eBAmFn_ITDF7_Iv_rY9FhCKwAnk9qQ/edit#heading=h.lgvqzgu7bc4d).

**Framework Code and Documentation**

Cluster Telemetry is primarily written in Go with a few Python scripts. The
frontend is written using Polymer. The framework lives in
[main/ct](https://skia.googlesource.com/buildbot/+/main/ct).

Here is more detailed documentation for the different pages:

*   [Perf
            page](https://docs.google.com/document/d/1GhqosQcwsy6F-eBAmFn_ITDF7_Iv_rY9FhCKwAnk9qQ/edit)
*   [Analysis
            page](https://docs.google.com/document/d/1ziof4lNwDFXyerVbEocdF3_DdUHVnD3FKYB9rShztuE/edit#)
*   [Metrics Analysis
            Page](https://docs.google.com/document/d/1MY95ULhEuKFznBQpF60_uhdhRco5bWzUVfVTp0M2hDw/)

**Contact Us**

If you have questions, please email rmistry@ or
[cluster-telemetry@google.com](mailto:cluster-telemetry@google.com).

---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: rappor
title: Rappor (Randomized Aggregatable Privacy Preserving Ordinal Responses)
---

RAPPOR reports consist of randomly generated data that is biased based on data
collected from the user. Data from many users can be aggregated to learn
information about the population, but little or nothing can be concluded about
individual users from their reports.

Descriptions of individual metrics should be found in
[tools/metrics/rappor/rappor.xml](https://chromium.googlesource.com/chromium/src/+/HEAD/tools/metrics/rappor/rappor.xml)

For full technical details of the algorithm, see [the RAPPOR
paper](http://static.googleusercontent.com/media/research.google.com/en/us/pubs/archive/42852.pdf).

## Algorithm

The first time the RapporService is started, a client will generate and save a
random 128-byte secret key, which won't change and is never transmitted to the
server. It will also assign itself to a random cohort.

For each metric we collect, we store a [Bloom
filter](http://en.wikipedia.org/wiki/Bloom_filter), represented as an array of m
bits. Each cohort uses a different set of hash functions for the bloom filter.
When the RapporService is passed a sample for recording, it sets bits in the
Bloom filter for that metric. For example, with the "Settings.HomePage2" metric,
which is collected only for users who opt-in to UMA, our Bloom filter will be an
array of 128 bits, and one or two of those bits will be set based on the eTLD+1
of the user's homepage.

Once we have collected samples, and are ready to generate a report, we take the
array of bits we've gathered for the metric and introduce two levels of noise by
taking the following steps.

1.  Add deterministic noise.
    1.  Create a deterministic psuedo-random function by passing the
                client's secret key, the metric name, and the bloom filter value
                into an HMAC_DRBG function.
    2.  For each bit use this deterministic function to flip two
                weighted coins.
        1.  If the first is heads, replace the bit with the result of
                    the second coin.
        2.  Otherwise, leave the bit as is.
2.  Add fresh random noise.
    1.  Using fresh randomness, flip two more coins, with different
                weights.
        1.  If the bit is true, report the result of the first coin
                    flip.
        2.  Otherwise, report the result of the second.

The cohort that the client that belongs to and the results from the above
process are sent to the server.

The large amount of randomness means that we can't draw meaningful conclusions
from a small number of reports. Even if we aggregate many reports from the same
user, they include the same pseudo-random noise in all of their reports of the
same value, so we are effectively limited to one report for each distinct value.

Indeed, even with infinite amounts of data on a RAPPOR statistic, there are
strict bounds on how much information can be learned, as outlined in more detail
at <http://arxiv.org/abs/1407.6981>. In particular, the data collected from any
given user or client contains such significant uncertainty, and guarantees such
strong deniability, as to prevent observers from drawing conclusions with any
certainty.

**Adding metrics**

To add a new rappor metric, you need to add a bit of code to collect your
sample, and add your metric to
[tools/metrics/rappor/rappor.xml](https://chromium.googlesource.com/chromium/src/+/HEAD/tools/metrics/rappor/rappor.xml).
Samples should be recorded on the UI thread of the browser process. For most use
cases, you will want to use one of the helper methods in
[components/rappor/rappor_utils.h](https://chromium.googlesource.com/chromium/src/+/HEAD/components/rappor/rappor_utils.h)
(formerly
[chrome/browser/metrics/rappor/sampling.h](https://chromium.googlesource.com/chromium/src/+/HEAD/chrome/browser/metrics/rappor/sampling.h))
e.g.

```none
#include "components/rappor/rappor_utils.h"
```

```none
```

```none
rappor::SampleDomainAndRegistryFromGURL(g_browser_process->rappor_service(),
```

```none
                                        "Settings.HomePage2", GURL(homepage_url));
```

If you need to do something more specific you may need to call
RapporService::RecordSample directly, e.g.

```none
#include "components/rappor/rappor_service.h"
```

```none
<table>
```
```none&#10;<tr>&#10;```
```none&#10;<td>g_browser_process->rappor_service()->RecordSample( </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> "MyCustomMetric", </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> rappor::ETLD_PLUS_ONE_RAPPOR_TYPE, </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> my_custom_metric_sample);</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>);</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;</table>&#10;```

If you collect multiple samples for the same metric in one reporting interval
(currently 30 minutes), a single sample will be randomly selected for generating
the randomized report.

Remember to add documentation for your metric to
[tools/metrics/rappor/rappor.xml](https://chromium.googlesource.com/chromium/src/+/HEAD/tools/metrics/rappor/rappor.xml)

```none
<table>
```
```none&#10;<tr>&#10;```
```none&#10;<td><rappor-metric name="Settings.HomePage2" type="ETLD_PLUS_ONE"> <owner>holte@chromium.org</owner> <summary> The eTLD+1 of the prefs::kHomePage setting. Recorded when a profile is loaded if the URL is valid and prefs::kHomePageIsNewTabPage is false. </summary> </rappor-metric></td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;</table>&#10;```

## Code overview

[CL/49753002](https://codereview.chromium.org/49753002) introduces a
`RapporService` object which is instantiated by the browser process object. The
service allows collection of RAPPOR metrics and periodically uploads reports to
our servers.

### Sample Collection

In order to collect samples, we call `RapporService::RecordSample` to record
samples of it. The first call to `RecordSample` after the browser starts or a
report is generated will instantiate a new `RapporMetric` object to hold the
data for that metric. If subsequent calls to `RecordSample` are made for the
same metric during the same reporting interval, one sample is randomly selected
for generating the report. Calls to `RecordSample` should take place on the
browser UI thread.

Currently, samples may only be collected in the browser process, but
`RapporMetric`s could be serialized to IPC calls to enable collection from other
processes.

#### Report generation and uploading

The other function of `RapporService` is generating and uploading reports of the
collected data. First `RapporService::RegisterPrefs()` should be called to
register `prefs::kRapporSecret` and `prefs::kRapporCohort`, and then
`RapporService::Start()` may be called to begin generating reports.

The `RapporService` generates a report shortly after it is started at fixed time
intervals afterwards. If any new `RapporMetrics`s have been created, randomized
data is generated for them by calling `RapporMetric::GetReport()` and recorded
into a `RapporReports` proto, and the `RapporMetric` is deleted.

```none
<table>
```
```none&#10;<tr>&#10;```
```none&#10;<td>message RapporReports {</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // Which cohort these reports belong to. The RAPPOR participants are </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // partioned into cohorts in different ways, to allow better statistics and </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // increased coverage. In particular, the cohort will serve to choose the </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // hash functions used for Bloom-filter-based reports. </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> optional int32 cohort = 2; </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> message Report { </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // The name of the metric, hashed. </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> optional fixed64 name_hash = 1; </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // The sequence of bits produced by random coin flips in </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // RapporMetric::GetReport(). For a complete description of RAPPOR </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // metrics, refer to the design document at: </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> // http://www.chromium.org/developers/design-documents/rappor </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> optional bytes bits = 2; </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> } </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td> repeated Report report = 3; </td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>}</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;</table>&#10;```

The proto is passed to the `LogUploader`. It stores all of the logs it is passed
in a queue, and sends them to the server. When uploads fail, it retries with
exponential backoff. For now, if chrome exits before the logs are uploaded, they
are lost. We may implement caching unsent logs in prefs similar to UMA in the
future.
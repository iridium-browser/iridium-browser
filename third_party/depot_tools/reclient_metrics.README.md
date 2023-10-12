# Reclient metric collection

[TOC]

## Overview

When chromium developers enable download_remoteexec_cfg and run their build with use_remoteexec enabled,

e.g.

`.gclient`
```
solutions = [
  {
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {
 ...
      "download_remoteexec_cfg": True,
 ...
    },
    ...
  },
]
```

```
$ gclient runhooks
$ gn gen -C out/Release --args="use_remoteexec=true"
$ autoninja -C out/Release chrome
```

reproxy uploads reclient's build metrics. The download_remoteexec_cfg gclient flag is only available for Google employees.

Before uploading metrics, reproxy will show a message 10 times to warn users that we will collect build metrics.

Users can opt in by running the following command.
$ python3 reclient_metrics.py opt-in

Users can opt out by running the following command.
$ python3 reclient_metrics.py opt-out

## What type of data are collected?

We upload the contents of <ninja-out>/.reproxy_tmp/logs/rbe_metrics.txt.
This contains
* Flags passed to reproxy
  * Auth related flags are filtered out by reproxy
* Start and end time of build tasks
* Aggregated durations and counts of events during remote build actions
* OS (e.g. Win, Mac or Linux)
* Number of cpu cores and the amount of RAM of the building machine


We collect googler hostnames to help diagnose long tail issues.
We don't collect any other personally identifiable information
(e.g. username, ip address).

## Why are reproxy metrics collected? / How are the metrics collected?

We (Chrome build team/Reclient team) collect build metrics to find slow build tasks that harm developer's productivity. Based on collected stats, we find the place/build tasks where we need to focus on. Also we use collected stats to track Chrome build performance on developer's machine. We'll use these stats to measure how much we can/can't improve build performance on developer machines.

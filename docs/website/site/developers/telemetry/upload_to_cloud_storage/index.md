---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/telemetry
  - 'Telemetry: Introduction'
page_name: upload_to_cloud_storage
title: 'Telemetry: Cloud Storage'
---

[TOC]

In order to reduce the size of the Chromium source checkout, Telemetry stores
binaries in Cloud Storage. This includes:

*   *Recordings of webpages*, known as "page set archives" or
            "[WPR](https://github.com/chromium/web-page-replay) archives."
*   *Support binaries*, such as device/host_forwarder, ipfw,
            minidump_stackwalk, crash_service.
*   *credentials.json*, which is in Cloud Storage to provide access
            rights.

Many benchmarks require these files to run, and will fail without them.

When recording performance tests, we generally don't call this script directly.
Instead, we use [update_wpr](https://source.chromium.org/chromium/chromium/src/+/main:tools/perf/recording_benchmarks.md).


## Set Up Cloud Storage

### Install depot_tools

Follow [these instructions](/developers/how-tos/install-depot-tools) to install
depot_tools.

### Request Access (for Google partners)

Many of the page set archives are in the `chrome-partner-telemetry` bucket,
which is not accessible publicly. Google partners can request access by emailing
`telemetry@chromium.org` or submitting a ticket [`here`](https://bugs.chromium.org/p/chromium/issues/entry?template=Chromium+Perf+Test+Data%2C+Resource+or+Access+Request).



### Authenticate into Cloud Storage

Some files in Cloud Storage include data internal to Google or its partners. To
run benchmarks that rely on this data, you need to authenticate. Run the command
below and follow the instructions for authentication with your **corporate
account**.

```none
$ python depot_tools/gsutil.py config
```

When prompted with “`What is your project-id?`”, just enter `0`. Note that this
command is in `depot_tools`, not part of the Google Cloud SDK.

## Buckets

Telemetry has three Cloud Storage buckets you can put binary data in.

*   `cloud_storage.PUBLIC_BUCKET == 'chromium-telemetry'`
*   `cloud_storage.PARTNER_BUCKET == 'chrome-partner-telemetry'`
*   `cloud_storage.INTERNAL_BUCKET == 'chrome-telemetry'`

BSD-compatible support binaries should all go into `PUBLIC_BUCKET` so that
everyone can use them.

Google wants to avoid legal issues with distributing third-party content, so to
be safe, **most recordings of websites on the public web go in
`PARTNER_BUCKET`**, which is accessible by Googlers and certain Google partners.
Recordings of Google-properties on the public web can go in `PUBLIC_BUCKET`, and
recordings of unreleased or internal Google websites go in `INTERNAL_BUCKET`.

## Upload to Cloud Storage

### Upload your files into the bucket “chromium-telemetry”

Put the target file in the directory you want it to be when downloaded from
Cloud Storage, say `path/to/target`. Use this command to upload:

```none
$ python depot_tools/upload_to_google_storage.py --bucket chromium-telemetry path/to/target
```

A SHA1 file `path/to/target.sha1` will be generated for each uploaded file.

### Check the .sha1 files into the repository

```none
$ git add path/to/target.sha1
```

## Download from Cloud Storage

### Download the file in Python

```none
from telemetry.util import cloud_storagecloud_storage.GetIfChanged('path/to/target', cloud_storage.PUBLIC_BUCKET)
```

### Download the file manually

```none
$ python depot_tools/download_from_google_storage.py -s [target.sha1] -b chromium-telemetry
```

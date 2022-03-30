---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: cbuildbot-metadata-json-tutorial
title: 'cbuildbot: Tutorial on adding data to metadata.json'
---

### Note: Until [crbug.com/356930](http://crbug.com/356930) is resolved, this applies only data added by stages that run AFTER the sync stage and subsequent re-execution of cbuildbot.

### Why would you want to do this?

If you want to keep track of something that is happening on a builder and
process it in a separate application, you’ll want to add it to metadata.json.
This will allow you to export the necessary data in JSON format so that another
program can pull it in and process it.

How to add data to metadata.json

    During a run, metadata is stored in the builder run metadata object,
    accessible from most stages via self._run.attrs.metadata. Any stage which
    runs after the final re-execution of cbuildbot can write data to this
    metadata object for eventual inclusion in the uploaded metadata file. To do
    this, use the metadata object's UpdateWithDict method.

    The final metadata.json file is built and uploaded during the ReportStage.
    During the run, snapshots of the metadata may be uploaded to
    partial-metadata.json, for instance during the ReportBuildStartStage.

    Much of the metadata is written by
    cbuildbot_metadata.CBuildbotMetadata.GetReportMetadataDict, which is called
    by ReportStage at the end of the build. However, it is generally preferrable
    to write metadata within the stage the first knows or produces this data,
    rather than waiting until the end of the build.
---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/crash-reports
  - Crash Reports
page_name: handle-file-upload-scheme
title: Handle file upload scheme
---

Some client application need to handle the actual upload of the crash reports,
specially on Mac or iOS where using NSUrlSession is mandatory on some
conditions.. It can be done by adding support of the file:// scheme for upload
URL.

Three main modifications have to be done to breakpad to support this feature.

    Write the HTTP request to file.

    Breakpad thread.

    Separate the request creation and the response handler.

As Breakpad is used by multiple applications on multiple platforms, the standard
behavior has to be preserved if the scheme is not file://.

## Write the HTTP request to file

It is possible to set the uploading URL in the config NSDictionary\* to a
temporary file://. This requires an API to get the configuration dictionary.
This URL can then be checked in the HTTPMultipartUpload send method, and in case
of a file:// scheme, the request is written in a file.

## Breakpad thread.

Every call in Breakpad must be executed in the breakpad thread. This implies to
use the \[BreakpadController withBreakpadRef:\] method that will ensure that
operations are executed in the correct thread.

## Separate the request creation and the response handler.

The biggest change in the project is the separation of the request creation and
the response handler. It is possible for the client to wait a long time before
sending the report. The two phases send the report and interpret the response
have to be splitted.

Breakpad renames the minidump from the minidumpID to an ID contained in the HTTP
response. At the moment the response is received, it is necessary to have both
IDs. An API has to be provided to the client to store the configuration
NSDictionary, and to reconstruct the upload objects using it.

**Proposed new API :**

### HTTPMultipartUpload:

- \[HTTPMultipartUpload send\] : (modified)

This method will test if the url scheme is file://. In that case, the request
body is written to the pointed file and method returns nil.

### Uploader

- \[Uploader report\] (modified)

Split this method into two method :

    - \[Uploader report\] that will initiate the upload

    - \[handleResponse:(NSData\*)response withError:(NSError\*)error\] that will
    handle the response.

If the URL is not file://, - \[Uploader report\] will directly call -
\[handleResponse:(NSData\*)response withError:(NSError\*)error\].

- \[handleResponse:(NSData\*)response withError:(NSError\*)error\] (added)

that will handle the response.

NSDictionary \*readConfigurationData(const char \*configFile); (added)

The configuration has to be saved to be able to recreate the uploader object
when the network response is received. This method has to be public.

### Breakpad

NSDictionary\* BreakpadGetNextReportConfiguration(BreakpadRef ref); (added)

This method is needed to retrieve the configuration of the next report to be
uploaded.

void BreakpadUploadReportWithParametersAndConfiguration(BreakpadRef ref,

NSDictionary \*server_parameters,

NSDictionary \*configuration); (added)

void BreakpadHandleNetworkResponse(BreakpadRef ref,

NSDictionary\* configuration,

NSData\* data,

NSError\* error); (added)

### BreakpadController :

- (void)threadUnsafeSendReportWithConfiguration:(NSDictionary\*)configuration

withBreakpadRef:ref; (added)

This method calls BreakpadUploadReportWithParametersAndConfiguration in the
current thread. It is needed to keep the sequentiality of the operations. As the
name of the method suggests, it is threadUnsafe and should only be used from the
- (void)withBreakpadRef:(void(^)(BreakpadRef))callback

callback.

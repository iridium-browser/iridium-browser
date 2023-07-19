---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: google-cloud-print-proxy-design
title: Chromium Print Proxy
---

[TOC]

## **Abstract**

A cloud print proxy enables cloud printing support for legacy (non-cloud-aware) printers. The proxy acts as a [protocol bridge](http://code.google.com/apis/cloudprint/docs/proxyinterfaces.html) between Google Cloud Print and the native print driver stack on existing PCs. The cloud print proxy runs on Windows (XP SP2 and higher), Mac OS X (10.5 and higher on Intel), and Linux desktops and laptops. The proxy code is built as a part of the Google Chrome browser and is an end-user opt-in feature. It should be noted that in parallel to developing the cloud print proxy for legacy printer support, Google will be working with third parties to help create solutions (based on open/documented standards) for allowing printers to connect directly to Google Cloud Print without the need for a dedicated PC or cloud print proxy on that PC.

## **Goals**

*   Provide cloud printing support for legacy printers.
*   Serve as a reference implementation for the [service
            interfaces](http://code.google.com/apis/cloudprint/docs/proxyinterfaces.html)
            provided by Google Cloud Print.

## **Audience**

The cloud print proxy code acts as a reference implementation for implementing
the [service
interfaces](http://code.google.com/apis/cloudprint/docs/proxyinterfaces.html)
provided by Google Cloud Print. This document is intended for Google Chrome
browser developers as well as future implementers of the [service
interfaces](http://code.google.com/apis/cloudprint/docs/proxyinterfaces.html)
(for example, printer vendors).

## **Design**

The Google Chrome browser exposes the cloud print proxy as an end-user opt-in
feature in the Options dialog. When the user chooses to enable this feature, the
proxy code asks for the user's Google account credentials (if this has not
already been entered for another cloud-related feature such as Bookmarks Sync).
Upon successful authentication, the client proxy code runs a background thread
that performs the following steps:

*   It enumerates all the local and network printers connected to the
            client machine. It then requests Google Cloud Print for a list of
            printers already registered to this specific proxy for this specific
            user.
*   The cloud print proxy then registers any local and network printers
            that have not already been registered with the user's Google
            account. Printer registration includes publishing the capabilities
            as well as printer default settings for each printer in an
            extensible format. Printer registration follows the steps outlined
            in the [service
            interfaces](http://code.google.com/apis/cloudprint/docs/proxyinterfaces.html)
            document.
*   For each registered printer, the proxy does the following:
    *   Checks to see if the printer still exists locally. If it does
                not, it deletes the printer from the server.
    *   Compares the hash of the printer's current capabilities against
                the last uploaded hash to the server. Any changes are then
                uploaded to Google Cloud Print.
    *   Checks for any pending jobs assigned to the printer. Pending
                jobs are downloaded and spooled to the local printer. See below
                for details of job status reporting.
    *   Registers for notifications of changes or deletions to the
                printer. Changes or deletions are synced with Google Cloud
                Print.
*   The proxy code also listens for print request notifications from
            Google Cloud Print. Print request notifications are delivered using
            the XMPP protocol. Upon receiving a print request, the proxy fetches
            the print jobs for that specific proxy from Google Cloud Print and
            spools them to the local printer. The printer notification code
            above checks for changes to the status of the job and updates Google
            Cloud Print with the status of the job.
*   The proxy code registers for notifications of printer additions.
            Newly added printers are registered with Google Cloud Print.
*   With the exception of the code to talk to the operating system print
            spooler and to perform printer-specific tasks, all of the cloud
            print proxy code is platform-independent. The platform-dependent
            code is abstracted behind a platform-agnostic interface.

Cloud services in the Google Chrome browser such as cloud print proxy and
Bookmarks Sync share a common framework for signing in to the user's Google
account as well as for receiving XMPP-based notifications. Users enter their
Google credentials only once to enable one or more of these features.

The cloud print proxy background thread continues to run in a persistent process
even when the user closes all visible browser windows. In addition, upon user
login, an invisible instance of this process with this background thread running
is launched automatically.

As listed in the table below, the cloud print proxy fetches print jobs in PDF
format. The format used for the user-selected printer settings (print ticket) on
each platform is also listed in this table. On Windows, the proxy code will use
a PDF interpreter library to print the PDF. The proxy code leverages platform
support on Mac OS X and Linux to print PDF documents.

**Data formats**

The following table summarizes the data formats used for the current proxy
implementations. Note that these formats are specific to the current proxy
implementations. Google Cloud Print supports multiple formats and the actual
data formats used for cloud-aware printers will likely be different from the
ones listed below.

<table>
<tr>
<td><b>Proxy operating system</b></td>
<td><b>Data format for document to be printed</b></td>
<td><b>Data format for printer capabilties</b></td>
<td><b>Data format for printer defaults</b></td>
<td><b>Data format for print job settings</b></td>
</tr>
<tr>
<td>Windows</td>
<td>PDF</td>
<td>XML-based Print Schema. <a href="http://msdn.microsoft.com/en-us/library/ms716431%28VS.85%29.aspx">See here for details</a>.</td>
<td>XML-based Print Schema. <a href="http://msdn.microsoft.com/en-us/library/ms715314%28VS.85%29.aspx">See here for details</a>.</td>
<td>XML-based Print Schema. <a href="http://msdn.microsoft.com/en-us/library/ms715314%28VS.85%29.aspx">See here for details</a>.</td>
</tr>
<tr>
<td>Mac OS</td>
<td>PDF</td>
<td>PPD. <a href="http://partners.adobe.com/public/developer/en/ps/5003.PPD_Spec_v4.3.pdf">See here for details</a>.</td>
<td>PPD. <a href="http://partners.adobe.com/public/developer/en/ps/5003.PPD_Spec_v4.3.pdf">See here for details</a>.</td>
<td>PPD. <a href="http://partners.adobe.com/public/developer/en/ps/5003.PPD_Spec_v4.3.pdf">See here for details</a>.</td>
</tr>
<tr>
<td>Linux</td>
<td>PDF</td>
<td>PPD.</td>
<td>PPD.</td>
<td>PPD.</td>
</tr>
</table>
**Note:** Not enough research has been done on the details of formats on the Mac
OS, and our decision might change.

## Open issues

*   Printer defaults for specific printers can be changed on the client
            OS. Should we upload the new set of defaults when they change? Or
            should we maintain a separate set of defaults in the cloud? If so,
            we need server-side UI to change defaults.

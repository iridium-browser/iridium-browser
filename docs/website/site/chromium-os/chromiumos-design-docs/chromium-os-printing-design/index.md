---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: chromium-os-printing-design
title: Chromium OS Printing
---

[TOC]

### Abstract

This document describes the cloud printing workflow and how it interacts with
Google Cloud Print. Support for and interactions with Google Cloud Print dialogs
will also be documented here as they are determined.

### **Chromium OS**

When asked to print, Chromium OS checks to see if the user is logged in. (This
case should be rare and occur only if the user entered via the guest account.)
If the user is not logged in, a login dialog is presented.

Once logged in, if the user has not yet set up cloud printing or has no
registered or shared printers, a server-hosted introductory wizard is presented.
In this case, printers that are universally shared (for example, "printing" a
PDF to Google Docs) do not count as shared with the user, as it's expected that
most users will be interested in printing to local printers. This wizard
outlines the steps necessary to set up cloud printing, including instructions
for setting up a proxy and registering printers. This should happen even if
there are printers shared globally with the user.

Once the user is logged in and has registered printers or has dismissed the
wizard as completed, the cloud printing workflow begins.

### Chromium-based browser

How cloud printing integrates into a Chromium-based browser is still to be
determined.

### Cloud printing workflow

On entering the cloud printing workflow, the browser checks to see if the
current page/application provides its own printing workflow (how to do this
TBD). Currently, some web applications, such as the document editor in Google
Docs, trap the keyboard shortcut for printing (Ctrl-P for English) and have
their own print menu items. The goal here is to unify the browser's print menu
item into that workflow for consistency. Once in the application's own print
flow, it is up to that application to work with Google Cloud Print or provide an
alternate means of printing. The application can choose to present its own print
settings user interface and re-join the common printing workflow by using the
window.print() JavaScript request, or it can provide its own entirely separate
printing solution.

If the application does not provide its own print workflow, a modal cloud print
common dialog is presented in a new popup window. This single dialog allows the
user to quickly print with some default settings but also allows for changing of
some page setup and advanced print settings without bringing up additional
dialogs. The contents of the dialog are hosted by Google Cloud Print, and the
browser provides calls to the scripts on the page in the dialog to change page
setup information (information needed for PDF generation) and for returning the
contents of the generated PDF file from the browser to the scripts for uploading
to the service. (This may be changed so that the browser could also provide the
upload service as a call, so that it can happen on the IO thread.) Any setting
of the page setup information will require re-generation of the PDF file, which
should happen in the renderer process that owns the tab being printed.

Optionally, in the future, the browser can get a thumbnail rendering of a
particular page and call into the dialog scripts to show it. Additionally,
application-specific simple and advanced settings may be incorporated into the
dialog.

After the user sets other post-PDF generation information (job settings, for
example, *N*-up printing), the scripts in the dialog upload the PDF the browser
provided and the job ticket containing the job settings. It then makes a call to
the browser to close the dialog.

#### Notes

This is a change in printing workflow from what is currently in Chromium OS. In
order to mix the page settings into a single simple print dialog, the print
settings can't be treated as final before the dialog comes up (which is how
printing in Chromium OS is currently working). But this also gives us an
opportunity to get a PDF generation progress meter somewhere in this process.
(There's a Chromium OS bug currently open on this issue.)

Care needs to be taken to generate the PDF in the background asynchronously. The
dialog script code needs to disable or otherwise indicate that it hasn't yet
gotten back the PDF information from the browser any time it makes a change to
the page setup information.

Uploading of the PDF and job settings happens in the background.

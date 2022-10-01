---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: rlz-api
title: RLZ Chrome API Proposal
---

## OBSOLETE -- the API has been removed in 2013.

## **Overview**

Google client products use a system called [RLZ](http://code.google.com/p/rlz)
to perform cohort tagging to manage promotion analysis. This is accomplished by
having the product record certain events in its life cycle, such as
installation, first Google search performed, home page set to Google, and so on,
and then forwarding that information to Google.

This document proposes exposing some RLZ function as a Chrome Extension.

For background documents, please see:

*   Chromium blog post: [In The Open, For
            RLZ](https://blog.chromium.org/2010/06/in-open-for-rlz.html)
*   Google Chrome Privacy Whitepaper: [Promotional
            Tokens](https://www.google.com/intl/en/chrome/browser/privacy/whitepaper.html#tokens)
*   [How to read an RLZ
            string](https://github.com/rogerta/rlz/blob/wiki/HowToReadAnRlzString.md)
*   The [RLZ open source project](https://github.com/rogerta/rlz/)
            (since merged into the [main Chromium source
            repo](https://chromium.googlesource.com/chromium/src/+/HEAD/rlz/))

## **Use Cases**

This API can be useful to any chrome-internal extension that wants do promotion
analysis.

## **Could this API be part of the web platform?**

No. This API is specifically for managing promotions of chrome-internal
extensions.

## **Do you expect this API to be fairly stable?**

This API is expected to be very stable. No changes are required to this API to
support new chrome extensions.

## **What UI does this API expose?**

This API proposal does not expose any UI elements.

## **How could this API be abused?**

An extension could record events for different products or for access points it
does not own. This could result in incorrect assessment of partnership
distribution. This problem could be mitigated by restricting the products and/or
access points made visible through this API. For example, a chrome extension
should not affect Google Desktop or Chrome Omnibox access points.

DoS attacks on Google are not possible since the underlying RLZ code prevents
more than one financial ping per product per 24 hour period for all RLZ users on
the same machine, not only chrome extensions.

This API is hidden behind the command line argument used for experimental APIs.

## **How would you implement your desired features if this API didn't exist?**

If this API did not exist, we would need to re-implement the existing [RLZ open
source library](http://code.google.com/p/rlz) as a combination of javascript and
NPAPI plugin.

## **Are you willing and able to develop and maintain this API?**

Yes.

**Will the API work on all platforms?**

The RLZ library is Windows-only at the moment, so this API will only be
supported on Windows until RLZ is adapted to work on other platforms.

**Draft API spec**

> Use the chrome.experimental.rlz module to record an RLZ events in your
> extension's life cycle.

> *Methods*

> **recordProductEvent**

> chrome.experimental.rlz.recordProductEvent(string *product*, string
> *accessPoint*, string *event*)

> Records an RLZ event for a given product's access point.

> ***Parameters***

> *product (string)*

> A one-letter product name for the chrome-internal extension. This name should
> be unique within the RLZ product name space. Refer to the function
> [GetProductName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc)
> in the RLZ code.

> *accessPoint (string)*

> A two-letter access point name used within the extension. This name should be
> unique within the RLZ access point name space. Refer to the function
> [GetAccessPointName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc)
> in the RLZ code.

> *event (string)*

> A string representing the name of the event that occurred on the access point.
> Valid possibilities:

> <table>
> <tr>
> <td><b>Name</b></td>
> <td><b>Description</b></td>
> </tr>
> <tr>
> <td>`install`</td>
> <td>Access point installed on the system</td>
> </tr>
> <tr>
> <td>`set-to-google`</td>
> <td>Point set from non-Google provider to Google</td>
> </tr>
> <tr>
> <td>`first-search`</td>
> <td>A first search has been performed from this access point since installation </td>
> </tr>
> <tr>
> <td>`activate`</td>
> <td>Product being used for a period of time </td>
> </tr>
> </table>

> **clearProductState**

> chrome.experimental.rlz.clearProductState(string *product*, string\[\]
> *accessPoints*)

> Clears all product-specific RLZ state from the machine, as well as clearing
> all events for the specified access points. This function should be called
> when the extension is uninstalled.

> *Is there any way for an extension to know it is being uninstalled so that it
> can make this call?*

> ***Parameters***

> *product (string)*

> *A one-letter product name for the chrome-internal extension. This name should be unique within the RLZ product name space. Refer to the function [GetProductName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc) in the RLZ code.*

> *accessPoints (array of string)*

> An array of two-letter access point names used within the extension. Each name
> should be unique within the RLZ access point name space. Refer to the function
> [GetAccessPointName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc)
> in the RLZ code.

> ****sendFinancialPing****

> **chrome.experimental.rlz.sendFinancialPing(string *product*, string\[\] *accessPoints, signature, brand, id, lang, exclude_machine_id, callback*)**

> **Sends a ping with promotional RLZ information to Google.**

> *******Parameters*******

> ***product (string)***

> ***A one-letter product name for the chrome-internal extension. This name should be unique within the RLZ product name space. Refer to the function [GetProductName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc) in the RLZ code.***

> ***accessPoints (array of string)***

> **An array of two-letter access point names used within the extension. Each name should be unique within the RLZ access point name space. Refer to the function [GetAccessPointName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc) in the RLZ code.**

> **signature *(string)***

> **A**

> ******product-specific signature******

> **string of the product.****

> **brand *(string)***

> **A promotional brand code assigned to the product.**

> **************Can be an empty string.**************

> **id *(string)***

> **A product-specific installation id. Can be an empty string.**

> **lang *(string)***

> **The language of the product. Used to determine cohort.**

> ********Can be an empty string.********

> **exclude_machine_id *(boolean)***

> **If true, the machine specific id will be excluded from the ping.**

> **callback *(function)***

> **A function that takes one boolean argument indicating whether the ping was successfully sent or not.**

> **getAccessPointRlz**

> chrome.experimental.rlz.getAccessPointRlz(string *accessPoint,* function
> *callback*)

> Gets the RLZ string to be used when accessing a Google property through the
> given access point. This string should be used in an `rlz=` CGI parameter in
> the Google property URLs.

> ***Parameters***

> **accessPoint (string)**

> *A two-letter access point name used within the extension. This name should be unique within the RLZ access point name space. Refer to the function [GetAccessPointName()](http://code.google.com/p/rlz/source/browse/trunk/win/lib/lib_values.cc) in the RLZ code.*

> **callback (function)**

> *A function that takes one string argument, which is the RLZ string for the given access point.*

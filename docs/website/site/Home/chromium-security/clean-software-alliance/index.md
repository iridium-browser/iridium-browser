---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: clean-software-alliance
title: Clean Software Alliance (CSA)
---

Chrome is participating in a pilot test of the [Clean Software
Alliance](http://www.cs-alliance.org/) (CSA), and provides a feature by which
CSA Installer Members can record information about what has been installed on
the system. These records consist of the following information:

    &lt;vendor_id&gt; - unique identifier of the installation vendor.

    &lt;install_id&gt; - unique installation ID (per vendor).

    &lt;publisher_id&gt; - unique identifier of the publisher (or main product)
    being installed.

    &lt;install_time_client&gt; - installation start time as per the user’s
    machine.

    &lt;install_time_server&gt; - installation start time as per the server
    time.

    &lt;advertisers_id&gt; - a comma-delimited list of unique advertiser IDs
    accepted by the user and installed in addition to the main product.

    &lt;hmac_sha256_validation&gt; - hmacSHA256 hash of a string containing all
    the entry fields using a private salt provided by every vendor. This field
    is used to validate the entry data and to prevent abuse.

If Chrome detects a [security
incident](https://www.google.ca/chrome/browser/privacy/whitepaper.html#malware),
some of this data may be reported as part of its environmental data collection.

---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: side-by-side-search-for-default-search-engines
title: Side by Side Search for Default Search Engines
---

This doc provides guidance and examples of how to adapt an existing Chrome Default Search Engine (DSE) to participate in the side-by-side search experience.

# What is side-by-side search?

Side-by-side search offers Chrome users the ability to open a small form factor Search Results Page (SRP), representing their most recent search query, in a browser sidebar UI. This UI gives users the ability to find the result they are looking for faster, without having to navigate back-and-forth between the SRP and a given results page.

The SRP hosted in the sidebar represents the most recent SRP visited for a particular tab. Different tabs may be associated with different SRPs.

# How Chrome Default Search Engines can participate in side-by-side search

Side-by-side search leverages a Chrome DSE's configuration as specified in [prepopulated_engines.json](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json). A DSE looking to participate in side-by-side search must define the `side_search_param` field with an appropriate string value.

Chrome will remember the most recent SRP that the user has landed on for each tab. The determination of whether or not a page is said to be a SRP belonging to the currently set DSE is handled by [TemplateURL::IsSearchURL()](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/template_url.h;l=815;drc=1946212ac0100668f14eb9e2843bdd846e510a1e). This method works by matching the current page against the DSE's `search_url` and `alternate_urls`, as defined in its prepopulated_engines.json entry.

The `side_search_param` value is used to construct a new sidebar URL from the most recent SRP. A query parameter is appended to the SRP URL of the form `value={version}` where

* value is the string value corresponding to the `side_search_param` field.

* `{version}` is an integer representing the current side-by-side search version. Currently only the version number `1` is supported.

This constructed URL is then used to fetch the SRP for the sidebar, at the request of the user. The page served for the sidebar should be responsive to accommodate a narrow but resizable UI surface and should reflect the original search page.

## Example - Google

Google's integration with side-by-side search offers an instructive example.

The `side_search_param` field is added to Google's entry in [prepopulated_engines.json](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json;l=124;drc=9795f20f95d3b8cf7719e62a18d2ccc2aeedf51a).

    "google": {
        "name": "Google",
        "keyword": "google.com",
        ...,
        "side_search_param": "sidesearch",
        "id": 1
    },

This is then used to construct the sidebar URL from a given Google SRP. For the following search query

    https://www.google.com/search?q=hello+world

The following sidebar URL is constructed

    https://www.google.com/search?q=hello+world&sidesearch=1

# Navigation

Side-by-side search routes navigations originating from the sidebar using the following rules.

* Navigations to other SRPs will proceed as normal within the sidebar (as determined by IsSearchURL() above).

* Navigations to non-SRPs will be redirected to the tab contents frame.

These rules allow users to open search result links from the sidebar directly into the associated tab, preventing the SRP from deloading and allowing them to quickly find the result they are looking for. This also allows users to refine their search query within the sidebar itself.

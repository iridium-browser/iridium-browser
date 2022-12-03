---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: side-image-search-for-default-search-engines
title: Side Image Search for Default Search Engines
---

This doc provides guidance and examples of how to adapt an existing Chrome Default Search Engine (DSE) to participate in side-image search experience.

# What is side-image search?

Side-image search offers Chrome users the ability to open a small form factor Image Search Results Page (ISRP), representing their image search query, in a browser sidebar UI. This UI gives users the ability to find the result they are looking for faster, without having to navigate back-and-forth between the ISRP and a given page that they are viewing.

The ISRP hosted in the sidebar represents the ISRP for the given page. This ISRP is universal i.e. it stays open and the same even when users are visiting different tabs.

# How Chrome Default Search Engines can participate in side-image search?

Side panel image search leverages a Chrome DSE's configuration as specified in [prepopulated_engines.json](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json). A DSE looking to participate in side panel image search must define the `side_image_search_param` field with an appropriate string value.

When the user issues an image search query by selecting `Search Images with {Search Engine}`  if the `side_image_search_param` exists, Chrome will use the value for `side_image_search_param` to construct a new sidebar URL by appending the query parameter to the image_url of the form `value={version}` where:
* `value` is the string value corresponding to the `side_image_search_param` field.
* `{version}` is an integer representing the current side panel image search version. Currently only the version number `1` is supported.

This constructed URL is then used to fetch the image search results for the sidebar, at the request of the user. The page served for the sidebar should be responsive to accommodate a narrow but resizable UI surface and should reflect the original search page.

When the user clicks on Open in a New Tab button, this additional query param (`side_image_search_param` value) will be removed from the URL and the resultant URL will be opened in a new tab, closing the side panel.

## Example - Google

Google's integration with the side panel image search offers an instructive example.

The `side_panel_search_param` field is added to Google's entry in [prepopulated_engines.json](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json;l=125).

    "google": {
        "name": "Google",
        "keyword": "google.com",
        ...,
        "side_image_search_param": "sideimagesearch",
        "id": 1
    },

This is then used to construct the sidebar URL from a given Google image search URL. For the following search query:

    https://www.google.com/imagesearch?p=xyz

The following sidebar URL is constructed:

    https://www.google.com/imagesearch?p=xyz&sidesearch=1

When the user hits Open in a New Tab, the `side_image_search_param` query param is removed and the the resultant URL is opened in a new tab:

    https://www.google.com/imagesearch?p=xyz

# How can I customize the label that we show in the image search context menu and combo box dropdown?

Side panel image search combo box label is derived from DSE's configuration as specified in [prepopulated_engines.json](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json). A DSE looking to customize the label in the side panel combobox should add `image_search_branding_label` to the file. This value will be used in the image search context menu string and the image search label in the combo box dropdown.

## Example - Google

Google for example, brands its image search as Google Lens so we [set that value](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json;l=126) for `image_search_branding_label` key:

    "google": {
        "name": "Google",
        ...,
        "image_search_branding_label": "Google Lens",
        "id": 1
    },

With this value, the right-click context menu string will read: `Search Images with Google Lens` and the combo box dropdown will have the `Google Lens` label instead of the search engine name.

# What icon do you show in the side panel combobox drop down?

For the side panel we show the same favicon as the search favicon.

# Navigation

Side panel image search routes navigations originating from the sidebar using the following rules:

* Any updates to the query params in the side panel is allowed. For example: for Google, when a user taps on any gleam or changes tabs in the results panel, we update the query param on the results URL and that query param update is preserved.
* Navigations resulting from user clicks that do not belong to the same domain as the results URL will be redirected to a new tab. E.g. `lens.google.com` can still make requests to `feedback.google.com` in the side panel.
* Any navigations not resulting from user clicks that do not belong to the same domain as the results URL will be blocked. E.g. In the above example, if the `lens.google.com` makes requests to `xyz.com`, then this request gets routed to a new tab.
* Tapping on images search results should open in a new tab.

These rules allow users to open search result links from the sidebar in a new tab, preventing the ISRP from deloading and allowing them to quickly find the result they are looking for. This also allows users to refine their search query within the sidebar itself.
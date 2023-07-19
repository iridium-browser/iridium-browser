---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: omnibox-prefetch-for-default-search-engines
title: Omnibox Prefetch and Prerender for Default Search Engines
---

This doc provides a quick guideline and example of how to adapt an existing
Chrome Default Search Engine (DSE) to prefetch and prerender likely suggestions.
This doc also provides some key details for understanding how Chrome treats
instructions to prefetch.

# How to recommend prefetches

Chrome utilizes the DSE suggestion API to fetch a list of suggestions that are
parsed in a fairly straightforward manner. This API fetches suggestions based on
the current user input in the omnibox, and a new set of suggestions is fetched
for each input change in the omnibox. Search engines specify their suggestion
API URL in a Chromium config file, e.g., [Bing is configured
here](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json;drc=453b22336b4546cf64373bfc8a7ba9ed31c24fff;l=67).
Note that “{searchTerms}” is the input in the omnibox, and various search
engines have implemented query params that any search engine can send to their
suggestion API.

There are up to 5 elements in an array, none of which are required. However,
they are parsed by position, so to provide the 5th element, all previous fields
must be present (but can be empty). The fields are:

1. The query value (must match the user input)

1. The suggestion list (an array of the suggestions text, i.e., possible autocompleted query values)

1. The description list (an array of the descriptive text corresponding to to the suggestion list that is shown to the user)

1. The query URL list (appears to be unused in Chrome, but needs to be the 4th item)

1. A JSON object of optional values

We will assume that the search engine has already implemented at least the first
and second field of the suggestion API. The third field is optional, but will be
implemented by most search engines that implement the suggestion API. The fourth
field must be present (in order to use the fifth field), but can be an empty
array. The fifth field is where we will focus our attention.

Assuming the provider has entries for the first 4 fields, the fifth field is a
JSON object, which can be initially added as an empty JSON object (“{}”) (note
there are other fields that can be added to this object, so en existing DSE
implementation may already provide a non-empty object). The prefetch
functionality is triggered using a new JSON field in this fifth field JSON
object. The provider needs to determine which suggestion result should be
prefetched using 0-indexing to represent the suggestion (often this is the first
suggestion result, as it is at the top of the list, so the index is 0). Once the
suggestion index is identified, the following should be added to the JSON object
replacing &lt;prefetch index&gt; with the appropriate value:

    "google:clientdata": {
        "phi": <prefetch index>;
    }

An example of the entire suggestion autocomplete response is provided below. It
will cause a prefetch for the first result in the suggestion list:

    [
        "do",
        ["Dog","Dolphin","Dodo"\],
        ["Terrestrial Mammal", "Aquatic Mammal", "Extinct Bird"],
        [],
        {
            "google:clientdata": {
                "phi": 0
            }

        }
    ]

This response will show the user the three results (Dog: Terrestrial mammal,
Dolphin: Aquatic Mammal, Dodo: Extinct Bird), and it will trigger a prefetch of
the search result for “Dog”. The prefetched result page will be fetched with the
“Purpose: Prefetch” header. The prefetched results page (if successfully
fetched) is stored in memory for 60 seconds, and ignores cache headers when
being served.

If a DSE wants to customize the prefetch behavior, such as to add a query
parameter, they may submit a change list to alter the [Chromium Source Code
(components/search_engines/prepopulated_engines.json)](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json?q=prepopulate%20-f:Debug%2Fgen%20f:json&ss=chromium)
to modify the search URL. Google has added a
[google:prefetchSource](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/prepopulated_engines.json?q=f:prepopulated%20google:prefetchSource%20-f:Debug%2Fgen&ss=chromium)
parameter to the search URL (i.e., the URL adds
“[&pf=cs](https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/template_url.cc;l=1153;drc=98898daa2e8f46ff098cea1f2e218f8a8266c838;bpv=1;bpt=1)”).
This query parameter is sent by chromium and used by Google Search’s server to
treat the request as a prefetch from the omnibox.

# How to recommend prerenders

Similar to Prefetch, Prerender also utilizes the same format to indicate prerender indexes,
with the key of "pre":

    "google:clientdata": {
        "pre": <prerender index>;
    }


# Things to note about the implementation

There are various reasons that Search prefetches might not be triggered by
Chrome or might not be served to the user:

* The user has disabled speculative prefetching

* The user has disabled Javascript (overall of for the search provider domain)

* The suggestion engine is not the DSE

* There was a recent error when prefetching

* There were too many recent prefetches that the user did not navigate to

* A prefetch (for that search term) was issued recently, whether it can be served or not

* Chrome (or an extension) blocked or delayed the search query URL

* The prefetch did not finish (see a 2XX response code) by the time the user navigated

* The user changed language, changed DSE, cleared history, etc. before the user navigates to the prefetch

* The prefetch became stale (60 seconds is the current configuration)

* The request failed (received a non 2XX error, timed out, network error)

* Prerender obeys all constraints above

Additionally, if the DSE adds a prefetch parameter, and the prefetch is served,
the address bar will show the URL the user would have otherwise navigated to
without the prefetch. This preserves history and link sharing to prevent users
seeing an error code or other problems when the server assumes the prefetch
parameter means the query is low priority due to being a prefetch.

Considering saving resources, Prerender would attempt to reuse the prefetched
response instead of sending a new network request, which means the search
engine will receive only one main resource request if it recommends prefetch
and prerender the same term.

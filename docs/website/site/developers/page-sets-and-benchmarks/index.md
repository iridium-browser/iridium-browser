---
breadcrumbs:
- - /developers
  - For Developers
page_name: page-sets-and-benchmarks
title: Benchmarks == Measurements + Page Set
---

## Overview

Telemetry is opinionated in what a benchmark is. A benchmark is:

*   A PageMeasurement, which is a canned way to measure an arbitrary
            page. The measurement really should work for any page.
    Good: Number of DOM Nodes in a Page
    Bad: Number of DOM Nodes in Gmail
*   A page set, which is a list of pages your PM is interested in
    Good: top 25 websites. The 10 most interesting ways users use our webapp
    Bad: page set for measuring memory, another page set for measuring scrolling

Here's why you care:

*   Writing a good measurement is really hard, and Chrome has a bunch in
            tools/perf/ that are all-ready-to-go!
*   Once you can measure a page, you'll often want to track more pages
            just like that. And each page you try to add is going to have corner
            cases. And quirks. A request will come in for A/B tests. Pretty
            soon, you'll have your original measurement hacked up with tons of
            special cases to handle all your app's different use cases. This is
            cray!
*   A page set is a product-level idea. PMs know what use cases are
            important for the product. Have them maintain the list of use cases
            as a page set.
*   The measurement is a code level idea. Given a given page, the
            measurement should evaluate how well you did. Engineers are good
            maintainers of a measurement class.

# Check out tools/perf/page_sets/top_25 for examples of page sets.

# ## An Example

## Lets say my users often go to a specific page, click a button, and ==scroll== a div that shows up. I could measure any number of things:

*   How long it takes to go to that URL in a fresh tab until the div is
            up
*   How smoothly I can scroll the div
*   How much time I spend in key WebKit systems: javascript, style,
            layout, paint, compositing
*   How much the memory footprint varies as I scroll the div
*   How much time I spend decoding images, and painting the page

Those are measurements. tools/perf has many measurements. **THINK TWICE BEFORE
YOU WRITE ANOTHER MEASUREMENT.** There probably is one already.

**Most telemetry users should be putting their effort into writing page sets.**
In this example, the page set isn't going to be "the page set for measuring this
specific url and how well it scrolls." Its going to be "the page sets for my
application," and since I have only one use case right now, it will be:

> {description: "my teams pages that are worth benchmarking",

> pages: \[

> {

> "url": "my page's url",

> "smoothness": \[

> {action: click_element, selector: "#show-dialog"},

> {action: scroll, selector: "#dialog &gt; #dialog-contents"}

> \]

> }

> }

Now if I wanted to measure the smoothness of this use case:

> ./tools/perf/run_multipage_benchmark smoothness_benchmark my_team.json

Suppose now you also want to test the perf of your main page without that dialog
showing. Ugh, write another benchmark? Not so much... just add another page:

> {

> "url": "my page's url",

> "smoothness": \[

> {action: scroll, selector: "body"}

> \]

> }

And maybe I want to monitor memory for that page, by clicking a compose button
10 times. Again, thats not a benchmark, its just another entry in yoru page.

> {

> "url": "my page's url",

> "smoothness": \[

> {action: scroll, selector: "body"}

> \],

> "stress_memory": \[

> {action: repeat, times: 10,

> repeat: \[

> {"action": click_element, selector: compose},

> {"action": "click_element", selector: "#discard"}

> \]

> \]

> }

Voila. Now we just:

> ./tools/perf/run_multipage_benchmark memory_benchmark my_team.json

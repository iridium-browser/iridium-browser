---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: using-r-to-reduce-page-cycler-regressions
title: Using R to reduce Page Cycler Regressions
---

See also
<https://www.chromium.org/developers/testing/page-cyclers/analyzing-page-cycler-results>.

IMPORTANT: When you see a page cycler regression, make sure to upload a saved
copy of the before/after stdio data off the bot. We regularly delete that data
off of buildbot, and you'll need it to do any of the analysis below.

Most of the time, with page cycler regressions, there are a handful of pages in
that test suite that regress. If you can narrow down which pages regressed, it's
easier to make a reduce suite that you can iterate on quickly to address the
regression.

R is a programming language for statistical analysis that make it easy to do
analysis on these results. I got a copy at
<http://cran.r-project.org/bin/macosx/>. I know very little R. If you have more
experience with R, please fix up this page to be moar awesome.

The page cycler stdio page has a detailed output of the runtime of each page in
the page cycler test. For example, page_cycler_moz spits out:

Pages:
\[bugzilla.mozilla.org,espn.go.com,home.netscape.com,hotwired.lycos.com,lxr.mozilla.org,my.netscape.com,news.cnet.com,slashdot.org,vanilla-page,web.icq.com,www.altavista.com,www.amazon.com,www.aol.com,www.apple.com,www.cnn.com,www.compuserve.com,www.digitalcity.com,www.ebay.com,www.excite.com,www.expedia.com,www.google.com,www.iplanet.com,www.mapquest.com,www.microsoft.com,www.moviefone.com,www.msn.com,www.msnbc.com,www.nytimes.com,www.nytimes.com_Table,www.quicken.com,www.spinner.com,www.sun.com,www.time.com,www.tomshardware.com,www.travelocity.com,www.voodooextreme.com,www.w3.org_DOML2Core,www.wired.com,www.yahoo.com,www.zdnet.com,www.zdnet.com_Gamespot.com\]
\*RESULT times: t=
\[376,292,128,85,178,79,87,38,9,54,27,40,43,20,73,35,32,92,37,45,8,44,21,77,34,47,24,52,68,69,42,18,91,71,39,153,144,47,46,80,101,27,35,34,27,53,32,31,29,10,43,22,25,39,14,57,28,25,24,33,35,8,17,14,20,25,44,19,38,66,22,37,15,38,50,32,95,122,40,18,47,49,27,34,35,27,53,31,30,28,10,44,19,26,39,14,57,29,26,24,33,36,8,17,15,19,25,44,19,38,65,24,37,15,40,49,33,94,123,42,18,46,48,26,35,33,27,53,33,30,29,10,44,20,26,40,15,56,29,26,24,33,37,9,17,15,19,26,43,18,38,66,23,37,15,38,51,32,94,121,41,18,47,49,26,35,34,26,54,31,29,28,10,45,21,25,39,14,56,29,26,24,32,36,7,17,14,19,25,43,18,39,65,21,36,15,38,50,31,92,122,41,18,47,49,26,35,35,26,53,31,30,29,9,44,20,25,38,14,57,29,26,25,33,37,8,16,15,20,25,41,19,39,66,23,34,15,39,50,32,93,122,41,19,47,50,26,35,35,25,53,31,31,29,9,44,21,25,39,14,56,30,25,25,33,36,9,17,15,19,25,44,19,38,67,22,36,14,40,51,31,94,122,41,18,47,50,25,36,33,27,53,33,29,29,10,45,21,25,39,14,57,30,27,24,33,37,8,17,15,20,25,43,19,38,66,22,35,14,39,51,33,95,124,40,19,47,49,26,35,35,25,53,32,29,28,10,45,20,25,40,14,56,30,26,25,33,37,9,17,15,20,25,43,19,38,66,22,38,14,40,51,32,94,123,42,19,45,50,27,35,35,26,52,32,30,29,9,44,20,25,39,15,57,29,27,25,34,37,8,17,16,20,27,43,20,39,66,22,37,13,38,51,33,96,123,41,19,45,50\]
ms

The pages listed under "Pages:" are loaded in a loop 10 times. So, t\[0\] is the
time to load bugzilla.mozilla.org the first time, and t\[10\] is the time to
load it the second time around.

Lets create some dummy page cycler data to work with. Lets stay that someone
commits a patch that causes a regression.

Before patch:

Pages: \[ojan.com, glens-head.org\];

\*RESULT times: t= \[0,99,2,56,0,99,2,56,0,99,2,56,0,99,2,56,0,99,2,56\] ms

After patch:

Pages: \[ojan.com, glens-head.org\];

\*RESULT times: t=
\[105,34,206,57,105,34,206,57,105,34,206,57,105,34,206,57,105,34,206,57\] ms

Clearly, looking at this, ojan.com got a lot slower. Looking at real page cycler
data, it's a lot hard to make sense of.

## Get the data into R

So, start the R interpreter and copy-paste the before and after runtimes:

&gt; before = c(0,99,2,56,0,99,2,56,0,99,2,56,0,99,2,56,0,99,2,56)

&gt; after =
c(105,34,206,57,105,34,206,57,105,34,206,57,105,34,206,57,105,34,206,57)

&gt; pages = c('ojan.com','glens-head.org')

## Working with the data

// Get an easy summary of before

&gt; summary(before)

Min. 1st Qu. Median Mean 3rd Qu. Max.

0.00 1.50 29.00 39.25 66.75 99.00

// Get all the before times for ojan.com

&gt; seq(1, 10 \* length(pages), length(pages))

\[1\] 1 3 5 7 9 11 13 15 17 19

&gt; before\[seq(1, 10 \* length(pages), length(pages))\]

\[1\] 0 2 0 2 0 2 0 2 0 2

// Get all the before times for glens-head.org

&gt; before\[seq(2, 10 \* length(pages), length(pages))\]

\[1\] 99 56 99 56 99 56 99 56 99 56

// Now lets make that a function

&gt; oneSite = function(data, index) {

data\[seq(index, 10 \* length(pages), length(pages))\];

}

&gt; oneSite(before, 1)

\[1\] 0 2 0 2 0 2 0 2 0 2

// And we can make a function to print the before/after summaries for a given
index.

&gt; summaries = function(index) {

print(pages\[index\]);

print(summary(oneSite(before, index)));

print(summary(oneSite(after, index)));

}

// Before/after summaries for ojan.com

&gt; summaries(1)

\[1\] "ojan.com"

Min. 1st Qu. Median Mean 3rd Qu. Max.

0 0 1 1 2 2

Min. 1st Qu. Median Mean 3rd Qu. Max.

105.0 105.0 155.5 155.5 206.0 206.0

// Now we can loop and see before/after summaries for each site in the cycler.

&gt; for (i in 1:length(pages)) {

summaries(i);

}

\[1\] "ojan.com"

Min. 1st Qu. Median Mean 3rd Qu. Max.

0 0 1 1 2 2

Min. 1st Qu. Median Mean 3rd Qu. Max.

105.0 105.0 155.5 155.5 206.0 206.0

\[1\] "glens-head.org"

Min. 1st Qu. Median Mean 3rd Qu. Max.

56.0 56.0 77.5 77.5 99.0 99.0

Min. 1st Qu. Median Mean 3rd Qu. Max.

34.0 34.0 45.5 45.5 57.0 57.0

// Print all sites where the mean regressed by more than 10ms.

&gt; threshold = 10;

&gt; for (i in 1:length(pages)) {

if (mean(oneSite(before, i)) &lt; (mean(oneSite(after, i) - threshold))) {

summaries(i);

}

}

## Graphs

// Bloxplot visualizing the median and stddev.

&gt; boxplot(before, after)

// Plot with the dots from before as one color and the dots of after as another.

&gt; plot(before, after, col=2:3)

// Plot each site individually using the oneSite function from above.

&gt; boxplot(oneSite(before, 1), oneSite(after, 1))

// Plot all the sites in separate graphs.

&gt; par(mfrow=c(ceiling(sqrt(length(pages))),ceiling(sqrt(length(pages))))) //
Make a graphics context big enough for all the graphs. Might need to make the
window that pops up very large for the next loop to work.

&gt; for (i in 1:length(pages)) {

boxplot(oneSite(before, i), oneSite(after, i), main=pages\[i\])

}

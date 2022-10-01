---
breadcrumbs:
- - /x-subresources
  - X-Subresources
page_name: x-subresources-1
title: 'X-Subresources: python server experiment'
---

### Experiment #1

Andrew Oates &lt;aoates@google.com&gt;

<table>
<tr>

<td># Purpose/Description</td>

<td>The purpose of this experiment was to determine if significant gains could
be had from adding the X-subresource headers to pages. If that was the case,
further experimentation would be done.</td>

<td>Specifically, I tested the effect of the prefetching on a page with several
external Javascript resources, which would need to be loaded and run serially by
the browser.</td>

<td># Setup</td>

<td>The experiment consisted of loading a single test page from a server, and
timing the page load times under various conditions. There were three
components: the server, the test files, and the client.</td>

<td>### Server</td>

<td>The server was a small HTTP server written in Python designed to serve
static content and optionally scan the pages for tags and generate X-subresource
for any external resources referenced in the document. It can also add an
artificial latency to simulate adverse network conditions --- in this case, it
simply waits a certain amount of time after receiving a GET request before
sending back the response.</td>

<td>TODO(aoates): publish the modified python server.</td>

<td>### Client</td>

<td>The client was a standard build of Firefox 3.0.10 with a custom extension
installed.</td>

<td>The extension listens for incoming requests, grabs any X-subresource headers
and initiates prefetch requests for all the referenced resources.</td>

<td>TODO(aoates): publish the firefox extension.</td>

<td>### Test Pages</td>

<td>The test pages consist of two variations on one page. In each case, the page
loads and executes 10 external Javascript files, and collects timing information
on how long the page load as well as each external JS load takes. The individual
Javascript files print out a message to the page, and stall for a small amount
of time (around 100ms).</td>

<td>In the first variation, `test5.html`, the external Javascript files are
hosted locally, alongside the page. They are therefore served by the Python
server, which cannot handle multiple parallel connections. In the second
variation, `test5_corp.html`, the Javascript files are hosted on a server which
can handle parallel connections.</td>

<td>TODO(aoates): publish the actual web pages.</td>

<td># Experimental Groups</td>

<td>There were four experimental groups, testing two variables:</td>

*   <td>Presence of X-subresource headers in test page headers, and</td>
*   <td>Location of external resources; either locally, served by the
            small Python HTTP server, or on remotely, on `www.corp.google.com`
            and served by Apache.</td>

<td>In each case, the test page itself was served locally by the Python
webserver (which generated the headers). The server was run with the following
command:</td>

<td>`./xsubserver.py --latency 300 [--no-xsubresource]`with the last parameter
varying. This introduced a "latency" of 300ms to all content served by the
Python web server; the server waited 300ms after receiving the request to send
back the response. The number 300 was chosen to approximate the latency of
downloading one of the javascript files from a typical server.</td>

<td># Trials and Results</td>

<td>Each group was run several times, and a representative set of results is
presented in the table below. For each group, an overall page load time is
presented, as is the load time of each Javascript `script` element (the time
taken to load and execute the Javascript file).<table></td>
<td><tr></td>
<td></tr></td>
<td><tr></td>
<td></tr></td>
<td><tr></td>
<td></tr></td>
<td><tr></td>
<td><td>*Results of Experiment #1*</td></td>
<td></tr></td>
<td><tr></td>
<td>External Resource Server</tr></td>
<td><tr></td>
<td>`xsubserver.py` (local)Apache/2.0.55 (remote)</tr></td>
<td><tr></td>
<td>X-subresource</td>
<td><td>Page Load: *3065 ms*</td>HeadersYes</td>
<td><td>JS #1 Load: 306 ms</td></td>
<td><td>JS #2 Load: 301 ms</td></td>
<td><td>JS #3 Load: 309 ms</td></td>
<td><td>JS #4 Load: 306 ms</td></td>
<td><td>JS #5 Load: 301 ms</td></td>
<td><td>JS #6 Load: 312 ms</td></td>
<td><td>JS #7 Load: 299 ms</td></td>
<td><td>JS #8 Load: 303 ms</td></td>
<td><td>JS #9 Load: 296 ms</td></td>
<td><td>JS #10 Load: 317 ms</td></td>
<td><td>Page Load: *1366 ms*</td></td>
<td><td>JS #1 Load: 379 ms</td></td>
<td><td>JS #2 Load: 108 ms</td></td>
<td><td>JS #3 Load: 116 ms</td></td>
<td><td>JS #4 Load: 106 ms</td></td>
<td><td>JS #5 Load: 107 ms</td></td>
<td><td>JS #6 Load: 106 ms</td></td>
<td><td>JS #7 Load: 107 ms</td></td>
<td><td>JS #8 Load: 107 ms</td></td>
<td><td>JS #9 Load: 107 ms</td></td>
<td><td>JS #10 Load: 106 ms</td></td>
<td></tr></td>
<td><tr></td>
<td><td>Page Load: *4178 ms*</td>No</td>
<td><td>JS #1 Load: 422 ms</td></td>
<td><td>JS #2 Load: 415 ms</td></td>
<td><td>JS #3 Load: 411 ms</td></td>
<td><td>JS #4 Load: 415 ms</td></td>
<td><td>JS #5 Load: 415 ms</td></td>
<td><td>JS #6 Load: 415 ms</td></td>
<td><td>JS #7 Load: 414 ms</td></td>
<td><td>JS #8 Load: 417 ms</td></td>
<td><td>JS #9 Load: 422 ms</td></td>
<td><td>JS #10 Load: 416 ms</td></td>
<td><td>Page Load: *3777 ms*</td></td>
<td><td>JS #1 Load: 487 ms</td></td>
<td><td>JS #2 Load: 353 ms</td></td>
<td><td>JS #3 Load: 342 ms</td></td>
<td><td>JS #4 Load: 342 ms</td></td>
<td><td>JS #5 Load: 352 ms</td></td>
<td><td>JS #6 Load: 356 ms</td></td>
<td><td>JS #7 Load: 352 ms</td></td>
<td><td>JS #8 Load: 347 ms</td></td>
<td><td>JS #9 Load: 484 ms</td></td>
<td><td>JS #10 Load: 351 ms</td></td>
<td></tr></td>
<td></table></td>

<td># Discussion</td>

<td>Let's examine each set of results.</td>

<td>### Python/local without prefetching</td>

<td>In this trial, there was an overall page load time (from execution of the
first script tag to sending the body's `onLoad` event) of 4178 ms, with each
individual javascript element taking a little over 400 ms to load and execute.
This is exactly as expected --- with a latency of 300 ms, and an execution
duration of 100 ms, each javascript element should take around 400ms to execute.
Multiply that by 10 elements, and we get a ~4000 ms page load time. This is our
worst-case scenario.</td>

<td>### Apache/remote without prefetching</td>

<td>Assuming we modeled the latency accurately with our local Python webserver
(which we didn't), these should be about the same as the local results. Each
Javascript load is 50-60ms faster, however. This can be chalked up to Apache
being just plain better than our local server.</td>

<td>### Python/local with prefetching</td>

<td>In this case, we shave about 1 second off our overall page load time. By
examining the server output, it is clear that the page is sending off its
prefetch requests all at the start of the page load, and the files are
downloaded in the network thread. However, since the server cannot handle
parallel connections/requests, the files must be read in serial. In this case,
it looks like the prefetches are around 100ms ahead of the execution in the UI
thread. Since we can load and execute at the same time, that 100ms stacks to
give us a savings of around a second. Pretty good!</td>

<td>### Apache/remote with prefetching</td>

<td>In this case, since Apache can handle multiple concurrent connections, the
browser can shoot off all its requests at once. Once again, the loading is a
little behind the execution of the scripts, but since we are downloading them
all concurrently, we've finished fetching the scripts by the time the second one
starts executing. Each subsequent script therefore only takes execution time (no
downloading needed), so we get excellent savings, very close to our optimal page
load time of ~1200-1300 ms.</td>

<td>### Costs</td>

<td>Adding the X-subresource headers can be costly. In this case, it increased
the size of the headers on `test5.html` from 166 bytes to 747 bytes. The
appended headers were:` X-subresource: test5_1.js; type=application/x-javascript
X-subresource: test5_2.js; type=application/x-javascript X-subresource:
test5_3.js; type=application/x-javascript X-subresource: test5_4.js;
type=application/x-javascript X-subresource: test5_5.js;
type=application/x-javascript X-subresource: test5_6.js;
type=application/x-javascript X-subresource: test5_7.js;
type=application/x-javascript X-subresource: test5_8.js;
type=application/x-javascript X-subresource: test5_9.js;
type=application/x-javascript X-subresource: test5_10.js;
type=application/x-javascript ` Note that the current implementation of the
extension doesn't use any of the metadata included in the header (it just sends
off a blind request), so that could be eliminated. Additionally, these could be
concatenated into one large X-subresource header:` X-subresource: test5_1.js,
test5_2.js, test5_3.js, test5_4.js, test5_5.js, test5_6.js, test5_7.js,
test5_8.js, test5_9.js, test5_10.js ` This would bring the size of the header
down to 302 bytes. While much more reasonable, this is not insignificant.
However, as plaintext, it's a good candidate for an optimization like gzip'd
headers.</td>

<td># Conclusions</td>

<td>When prefetching is enabled (as in the 1st row of results), the browser
doesn't have to block network loads on executing javascript. It can read the
prefetch headers, send off requests, and forget about them --- they'll continue
in the network thread while work is being done in the UI thread.</td>

<td>The most fruitful type of sub-resource to be prefetched is probably
Javascript; when the browser encounters an external JS file, it has to block the
main/UI thread until the file is downloaded and executed. If, however, the file
has been prefetched, the time taken to download is instant savings on the
overall load time of the page. In a case like this, with multiple consecutive
scripts, the difference can be dramatic.</td>

<td># Future Work</td>

<td>Some areas of future work include:</td>

*   <td>Testing with a dummynet for varying network bandwidth, RTT, and
            loss rate.</td>
*   <td>Writing an Apache module that generates X-subresource
            headers</td>
*   <td>Re-writing the extension in C++</td>
*   <td>Getting it to work with images and style sheets</td>
*   <td>Making it more robust in the face of strange cache settings (or,
            more generally, knowing when prefetching will make the user
            experience worse)</td>

</tr>
</table>

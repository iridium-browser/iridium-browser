---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: collecting-stats-for-graphite
title: Collecting Stats for Graphite
---

[TOC]

## Background

We have this amazing and fancy graph displaying utility called Graphite running
on [chromeos-stats](http://chromeos-stats/). It's beautiful. You all should use
it. This doc is about how to get data into the system so that you can view it in
Graphite.

There are two different ways to get data into the system:

The first is to write data to the raw backend of Graphite, which is called
*carbon*. It accepts data in the format of `<name> <value> <time>`, and one can
find a basic interface to sending data to carbon in
`site_utils.graphite.carbon`.

The second is to write data to a service which will calculate statistics over
the data you're sending, and then forward it onto carbon. This service is called
*statsd*. It provides better information, as it will calculate min/mean/max,
deviation, and provides a more intelligible interface. It also allows for better
horizontal scaling in case we ever start logging a truely hilarious amount of
stats. (Which we should!)

I would highly recommend using the statsd over carbon unless you have a specific
reason to be sending data directly to carbon.

## Walkthrough

We have in `site-packages` a library named `statsd`. This has been wrapped for
our purposes in a library located in `autotest_lib.site_utils.graphite.stats`,
which does some connection caching, prepending of autotest server name, and a
little other magic. The interface exposed is exactly the same as the one exposed
by statsd, and therefore this doc should work as a guide for both. (But you
should use the `site_utils` one!)

This guide serves to be copy-paste-able, so you should be able to take any
snippet out of this doc and run it. Therefore, here's the import boilerplate
you'll need when messing around with this code from within autotest:

`import time import common from autotest_lib.site_utils.graphite import stats `

If you prefer, you can find all the code listed in this doc (as of when this was
published) in [CL 45286](https://gerrit.chromium.org/gerrit/#/c/45286/).

As you go through and add some stats, or mess with the code shown here, at some
point you're going to want to see how the data is shown on Graphite. Navigate to
[chromeos-stats](http://chromeos-stats/). Drill down into `stats->[stat
type]->[your hostname]->[stat name]`. Main thing to note here is that statsd
dumps all of the stats under `stats/`, so if you go looking at the root level
for `[your hostname]->[stat name]`, you won't find anything. :P

`[your hostname]` here means "whatever value you have for `[SERVER] hostname =`
in your `shadow_config.ini`".

### Timers

The first stat to examine is how to log how long a function takes to run. The
easiest target for this is the scheduler tick. Let's define a fake scheduler
tick function:

`def tick(): time.sleep(10) # Sleeping is a very expensive computation `

And now we have a few different ways that we can get the runtime of this
function.

We can manually create a timer, and call `start()` and `stop()` at the beginning
and end of the function:

`def tick_manual(): timer = stats.Timer('testing.tick_manual') timer.start()
time.sleep(3) timer.stop() tick_manual() # You should now see a point at
3000(ms) in stats/timers/<hostname>/testing/tick_manual `

We can also take advantage of the decorator that is attached to the `Timer`
object:

`timer = stats.Timer('testing') @timer.decorate def tick_decorator():
time.sleep(5) tick_decorator() # You should now see a point at 5000(ms) in
stats/timers/<hostname>/testing/tick_manual `

Statsd timers report their value in milliseconds, so if you report a value by
hand using `send()`, you should probably report the time in milliseconds also.

#### Counters

If you're looking to keep track of how frequently something occurs, a counter is
a good choice. Statsd receives the counter stat, tallies it over time, and
flushes the value of events per second to carbon and resets the counter to zero
once every ten seconds. With counters, there are no extra statistics that statsd
can compute. The normal ones of min, max, std_dev, etc. make no sense in the
context of counters.

`# We can increment a counter every time we get an rpc request. def
create_job(): stats.Counter('testing.rpc.create_job').increment(delta=1) #
.increment() defaults to delta=1, so it could have been omitted for _ in
range(0, 10): create_job() # You should now see 1 at
stats/<hostname>/testing/rpc/create_job # 1 == 10 events / 10 seconds `

There also exists a `decrement()` on the counter object, but I'm not really sure
when one would use it. If you're trying to keep a running tally, you should
instead use a:

#### Gauge

If you're looking to be able to send in a number, or if your stat doesn't really
make sense as a timer or counter, then you should probably use a gauge. A gauge
allows you to just report a number. The benefit of using a gauge over just
sending raw data is that statsd will still compute the statistics about the
stats you're sending like it normally does.

`def running_jobs(): stats.Gauge('scheduler').send('running_jobs', 300)
running_jobs() # You should now see 300 at
stats/gauges/<hostname>/scheduler/running_jobs `

#### Average

Values submitted by an average are automatically averaged against the values in
the same bucket at the end of the flush interval. The only use case I can think
of for this is if you're trying to measure something in a gauge that's very
flaky, which is messing up all of the statistics that are being calculated.
However, I can't even think of an example to use in our codebase, so I'm just
mentioning this for completeness.

#### Raw

If all else fails, and you don't want any fancy statsd features, you can get
statsd to send your data to graphite "pretty much unchanged". Note that the
prefixing of your hostname still does happen (assuming you didn't turn it off).

One could use this to log the fact that something happened. Logging something so
that there's an obvious spike when you're overlaying graphs doesn't need any
sort of statistics calculated about it.

`# statsd automatically adds the current time to the data def
scheduler_initialized(): stats.Raw('scheduler.init').send('', 100)
scheduler_initialized() # 1 will now show up at the current time under
stats/<hostname>/scheduler/init`

# Gathering stats from Whisper via the Command Line

Stats can be queried directly from the command line using `whisper-fetch.py`

For example:

```none
whisper-fetch.py --pretty /opt/graphite/storage/whisper/stats/timers/cautotest/verify_time/lumpy/mean.wsp 
```

whisper-fetch can also output in JSON and you can specify the range of data you
wish to view via the --from and --until command lines. The default is to look at
a time slice of 24 hours.

# Gathering data directly from Graphite.

Create the graph of the information you are interested and copy the URL. With
the URL tack on &format=json and you will receive json formatted output with
time slice and data.

For example:

<http://chromeos-stats/render/?width=586&height=308&_salt=1367326398.143&target=stats.timers.cautotest.scheduler.tick.mean&format=json>

# Future Work/Needed Improvements

*   There's an ability to set a value in a gauge, and then modify it.
            This provides a counter that isn't reset to zero at each flushing,
            and is good for, say, number of RPC connections open at one time.
*   There's the ability to log events within Graphite that we currently
            have no API to do. This would allow us to log events like "build for
            3773.0.0 just came out", providing better context to the rest of the
            stats.
*   Utilizing Events, like adding a new board into the mix. System
            crash, etc.

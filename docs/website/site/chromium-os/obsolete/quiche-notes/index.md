---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/obsolete
  - Obsolete Documentation
page_name: quiche-notes
title: random notes by quiche@
---

Tracing wireless API events:

# cd /sys/kernel/debug/tracing

# echo 1 &gt; events/cfg80211/enable # requires wireless-3.8 or newer

# echo 1 &gt; events/mac80211/enable # available with wireless-3.4

&lt;do some stuff here&gt;

# cat trace

Example output

# tracer: nop

#

# entries-in-buffer/entries-written: 12/12 #P:4

#

# _-----=&gt; irqs-off

# / _----=&gt; need-resched

# | / _---=&gt; hardirq/softirq

# || / _--=&gt; preempt-depth

# ||| / delay

# TASK-PID CPU# |||| TIMESTAMP FUNCTION

# | | | |||| | |

wpa_supplicant-427 \[000\] ..s. 6636.389338: rdev_mgmt_frame_register: phy605,
wdev(1), frame_type: 0xd0, reg: true

wpa_supplicant-427 \[000\] ..s. 6636.389373: rdev_return_void: phy605

wpa_supplicant-427 \[000\] .... 6636.389900: rdev_set_power_mgmt: phy605,
netdev:mlan0(1818), enabled, timeout: -1

wpa_supplicant-427 \[000\] .... 6636.394420: rdev_return_int: phy605, returned:
0

wpa_supplicant-427 \[000\] .... 6636.453881: rdev_scan: phy605

wpa_supplicant-427 \[000\] .... 6636.453925: rdev_return_int: phy605, returned:
0

ksdioirqd/mmc1-5353 \[003\] .... 6636.609710: cfg80211_return_bss:
04:f0:21:05:4e:95, band: 0, freq: 2412

ksdioirqd/mmc1-5353 \[000\] .... 6640.651105: cfg80211_scan_done: aborted: false

Tracing cfg80211/mac80211 function calls:

# cd /sys/kernel/debug/tracing/

# echo 0 &gt; tracing_on

# echo &gt; trace # clear any existing data

# echo function_graph &gt; current_tracer

# echo funcgraph-abstime &gt; trace_options

# echo ':mod:cfg80211' &gt; set_ftrace_filter

# echo ':mod:mac80211' &gt;&gt; set_ftrace_filter # NOTE: two '&gt;'s here

# echo 1 &gt; tracing_on

# cat trace

Tracing general network events:

# cd /sys/kernel/debug/tracing/

# echo 1 &gt; events/net/enable

&lt;do some stuff here&gt;

# cat trace

\[...\]

sshd-5179 \[000\] ..s. 6940.442983: net_dev_queue: dev=eth0 skbaddr=ed0feb38
len=114

sshd-5179 \[000\] ..s. 6940.443025: net_dev_xmit: dev=eth0 skbaddr=ed0feb38
len=114 rc=0

Recording a trace:

Dumping the trace by hand (cat trace) works fine for manual work. But if you
want a long-running trace, do this instead:

1.  cd /sys/kernel/debug/tracing/
2.  echo 0 &gt; tracing_on
3.  Configure tracing by using the commands above (e.g. echo 1 &gt;
            events/net/enable)
4.  cat trace_pipe | gzip -c &gt; /var/log/trace.log.gz &
5.  echo 1 &gt; tracing_on

---

build

to see the compiler command-line for emerge/ebuild, add
CXXFLAGS="-print-cmdline" to the

environment of emerge/ebuild.

---

ccache

the chromiumos build tools use ccache, with the cache located in
/var/cache/distfiles/ccache.

you can inspect the cache as follows.

$ CCACHE_DIR=/var/cache/distfiles/ccache ccache -s

cache directory /var/cache/distfiles/ccache

cache hit (direct) 1

cache hit (preprocessed) 0

cache miss 2

called for link 2

compile failed 1

files in cache 6

cache size 16.8 Mbytes

max cache size 1.0 Gbytes

you can clear it with

$ CCACHE_DIR=/var/cache/distfiles/ccache ccache --clear

if you run ccache manually (without using the build tools), CCACHE_DIR will
probably not be set.

in this case, ccache will default to ~/.ccache as the cache directory.

$ ccache --show-stats

cache directory /home/quiche/.ccache

cache hit (direct) 1

cache hit (preprocessed) 0

cache miss 1

files in cache 3

cache size 8.5 Mbytes

max cache size 1.0 Gbytes

---

Working with mhtml files:

Before Ubuntu Precise:

> $ for x in \*.mhtml; do kmhtconvert -n -f $x; done

> $ for x in \*/index.html; do w3m -dump $x &gt; $(dirname $x).txt; done

With Ubuntu Precise:

> $ for x in \*.mhtml; do mu extract --save-all $x; done

Listing branch names:

> # in an existing checkout

> $ git branch -a

Checking out a branch: see [Working on a
Branch](/chromium-os/how-tos-and-troubleshooting/working-on-a-branch)

Enabling verbose logging for Chrome:

> # mount / -oremount,rw
> # echo "vmodule=network_event_log=1" &gt;&gt; /etc/chrome_dev.conf
> # restart ui

Monitoring cellular activation:

> enable verbose logging for Chrome (see above)

> # ff_debug +cellular+dbus+modem+portal+service

> # ff_debug --level -5

> # tail -F /var/log/chrome/chrome | logger -t chrome-main &

> # tail -F /var/log/ui/ui.LATEST | logger -t chrome-ui &

> # tail -F /home/chronos/user/log/chrome | logger -t chrome-user &

> # dbus-monitor --system | logger -t dbus-monitor &

Resetting modem:

> (see also
> <http://support.verizonwireless.com/clc/devices/knowledge_base.html?id=38519>)

> 1.  Network Options -&gt; Verizon Wireless -&gt; View Account -&gt;
              Account Overview -&gt; log in -&gt; cancel account
> 2.  crosh&gt; modem factory-reset 000000 y

> make sure to do these steps in order.

Monitor mode packet capture (requires another chromebook, with a dev image):

*   on connected chromebook:
    Ctrl-Alt-t to open crosh
    crosh&gt; network_diag --wifi
    ctrl-m to open file manager
    open most recent “network_diagnostics” file
    search for “BSS … associated”
    remember “freq” for next step
    look for “secondary channel offset” and “STA channel width” under “HT
    operation”
    if “STA channel width” is 20, use HT20 in next step
    if “STA channel width” is 40, and “secondary channel offset” is “above”, use
    "above" in next step
    if “STA channel width” is 40, and “secondary channel offset” is “below”, use
    "below" in next step
    otherwise, omit HT parameter in next step

*   on capture chromebook:
    disable wifi using UI
    crosh&gt; packet_capture --frequency &lt;freq&gt; --ht-location
    &lt;above|below&gt;

---

Monitoring D-Bus signals:

> $ sudo gdbus monitor --system --dest fi.w1.wpa_supplicant1 | logger -t
> supplicant-dbus &

---

Monitoring D-Bus calls:

By default, you can only monitor D-Bus signals (broadcast messages from one
process to the who system)

*   To enable monitoring of method calls, run
    ./mod_test_image_for_dbusspy.sh -i
    ../build/images/${BOARD}/latest/chromiumos_test_image.bin
*   dbus calls will be logged in /var/log/dbusspy.log

---

OOBE:

How to [force
OOBE](http://www.chromium.org/chromium-os/force-out-of-box-experience-oobe).

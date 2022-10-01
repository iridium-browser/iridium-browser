---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: a-brief-perf-how-to
title: Timechart how-to
---

Developer builds contain a performance analysis command called `perf` that can
be used to create an SVG output file similar to `bootchart`; the chart shows how
CPU cycles and I/O wait times are distributed across processes in the system
over time.

Below is a short primer in four lessons describing how to generate and view
output from `perf timechart`.

## Lesson 1 - a simple example

1.  Boot Chromium OS, and open a terminal.
2.  Run this command:

    ```none
    sudo perf timechart record
    ```

3.  Run your workload. A workload isn't necessary if all you want to see
            is a chart of an idle system. :-)
4.  When your workload is done, interrupt the process started in step 2
            using ^C, or kill -2.

**Explanation**: Without arguments, `perf timechart record` runs forever
gathering data, until stopped by SIGINT. Note that only SIGINT works; SIGTERM
will kill the process without producing the necessary output. When the command
completes, you'll see two new files: `perf.data` and `trace.out.`

## Lesson 2 - how to generate and view the chart

1.  In the directory where you ran Lesson 1, run this command:

    ```none
    sudo perf timechart
    ```

2.  The output image will be stored in a file named `output.svg`. Use
            `scp` or some equivalent to copy the file to another system for
            viewing.

**Tips for viewing**: Some browsers may have trouble displaying the image. The
author of timechart recommends the Inkscape image editor:

<http://www.inkscape.org/>

Inkscape does a good job of displaying the fine details, but it may be a bit
slow for the large timechart images. You should exercise patience when opening,
magnifying, or scroling images.

## Lesson 3 - how to avoid using SIGINT

1.  Run this command:

    ```none
    sudo perf timechart record sleep 5
    ```

2.  Run a workload that will finish within 5 seconds; for longer
            workloads, use a more appropriate sleep time in step 1.
3.  Generate and view the output as described in Lesson 2.

**Explanation**: If there are arguments to `perf timechart record`, the
arguments are treated as a command to run as a subprocess of `perf. perf`
gathers data until the process terminates.

If your workload is triggered by a single command, that command can be used in
place of 'sleep 5'. Note that if the workload acts as a daemon (that is, forks a
child and exits), `perf` will terminate with the parent terminates; this likely
isn't what you'd want.

## Lesson 4 - how to get a timechart of system boot

1.  Install bootchart on your workstation. For ubuntu:

    ```none
    sudo apt-get install bootchart
    ```

2.  emerge and install bootchart onto your DUT:

    ```none
    emerge-$BOARD bootchart &&  cros deploy $DUT bootchart
    ```

3.  reboot DUT

    ```none
    ssh $DUT reboot
    ```

4.  bootchart will log events in
            `/var/log/bootchart/boot-<timestamp>.tgz`. It will collect data
            until the DUT upstart sequence has fully completed. Retrieve the
            archive(s) with

    ```none
    scp $DUT:/var/log/bootchart/boot-<timestamp>.tgz  /tmp
    ```

5.  generate SVG graphics

    ```none
    F=boot-<timestamp> ; bootchart --format=svg -o $F.svg  $F.tgz
    ```

    or in a loop with

    ```none
    scp $DUT:/var/log/bootchart/boot-*.tgz  . && for i in *; do F=${i%.tgz}; bootchart --format=svg -o "${F}.svg"  "${F}.tgz"; done
    ```

    The svg file(s) are ready for viewing.

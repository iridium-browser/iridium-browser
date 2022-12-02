---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: service-processes
title: Service Processes
---

Overview

> Service processes (processes with --type=service) are child processes that are
> intended to possibly outlive the browser process that spawned them. They are
> currently used by the [cloud print
> feature](/developers/design-documents/google-cloud-print-proxy-design).
> Service processes can be set to start at login. They communicate with a
> browser via standard chromium IPC channels, but the way these channels is set
> up varies by platform.

## Start At Login

> ### Windows

> > Service processes are launched at login by adding them to the current users
> > auto run key path in the registry.

> ### Linux

> > Service processes are launched at login by adding an entry to the autostart
> > file in $XDG_CONFIG_HOME.

> ### MacOS X

> > Service processes are launched at login by adding a property list (.plist)
> > to ~/Library/LaunchAgents. This means that service processes on MacOS X are
> > controlled by
> > [launchd](http://www.afp548.com/article.php?story=20050620071558293). This
> > has some interesting ramifications that are discussed below.

## Process Lifetime

> Service processes continue to run as long as the service that they are
> providing is enabled and/or there is a browser process attached to them.
> Service processes are identified on the system by a unique identifier based on
> a service name, the user-data directory and the version of the service process
> supplying the service. If the user enables something that requires a service
> process (eg CloudPrint) the browser will launch the service process
> immediately, and set it up so that the service process will be started at
> login. On Windows and Linux, the service process is simply launched, while on
> Mac OS X the service process is started as a launchd job. Browser processes
> can force a service process to quit. On Windows and Linux this is performed by
> sending the equivalent of a SIGTERM to the service process. On MacOS X this is
> performed by telling launchd to remove the service process which in turn
> causes launchd to send the service process a SIGTERM. The relevant code is in
> ForceServiceProcessShutdown(...).

> **MacOS X note:** Since service processes are launched by launchd on MacOS X
> they will automatically be relaunched if they ever crash. The proper way to
> stop the service processes manually on MacOS X is to manipulate them using
> [launchctl](http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man1/launchctl.1.html).
> Sending a SIGTERM directly to the child process without using launchctl will
> result in a job still being registered with launchd. Launching a service
> process manually on MacOS X is difficult as it requires a properly configured
> property list containing several entries.

## IPC

> ### Windows

> > IPC is performed using a named pipe. The pipe name is based on the path to
> > the user-data directory and the version of the browser/service creating the
> > pipe.

> ### Linux

> > IPC is performed using unix domain sockets. The name of the socket is based
> > on the path to the user-data directory and the version of the
> > browser/service creating the sockets.

> ### MacOS X

> > IPC is performed using unix domain sockets. The name of the socket is
> > assigned to the browser by looking up an environment variable that is set by
> > launchd. On the service process side, the IPC channel is created using the
> > file descriptor that is passed to the service process when it checks in with
> > launchd. More information on this can be found on the [launchd man
> > page](http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man5/launchd.plist.5.html).
> > The name of the environment variable is based on the path to the user-data
> > directory and the version of the browser/service creating the sockets.

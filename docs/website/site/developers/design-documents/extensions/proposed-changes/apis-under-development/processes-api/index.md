---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: processes-api
title: Process Model API
---

**Proposal Date**
**02/24/2012**
**Who is the primary contact for this API?**
**Nasko Oskov (nasko@chromium.org), Charlie Reis (creis@chromium.org)**
**Who will be responsible for this API? (Team please, not an individual)**

**Site Isolation**

Overview**
Add chrome.processes API that lets extensions access data about the Chrome’s
process model, such as process ID and CPU usage.**

Use cases
Web applications are becoming more complex and resource demanding, and power
users may want a better view of which pages are responsible for resource use. It
may also be useful as a debugging or diagnosis tool, to see which tabs are
currently sharing fate. Here are a few types of extensions that could use this
API:

*   A Browser Action showing all tabs sharing the current process. All
            of these tabs would slow down or crash together if something went
            wrong.<img alt="image"
            src="https://lh5.googleusercontent.com/KnOBPV-gjd9L3rQLK966uNUTnsow-9mikKEj049XGuwTJf6UHxe3NXigKywxKyPuPvXEFoe71Vlxn9gDNE2vGTqpK5ffMjx7YmDCXlmVMqPzwR1TThw"
            height=189px; width=500px;>
*   A Browser Action showing the current CPU or memory use of the tab's
            process with a utilization graph, much like the Windows Task Manager
            icon in the system tray.
*   An extension that automatically restores all affected tabs if a
            process crashes, unless the crash repeats itself (much like IE8).
*   An extension that implements a custom task manager, perhaps with
            graphs or other visualizations.

Do you know anyone else, internal or external, that is also interested in this
API?
Chris Bentzel and Erik Kay have expressed interest in using this API to
prototype different policies for process management (such as killing prerender
processes). Patrick Stinson [has expressed
interest](http://groups.google.com/a/chromium.org/group/chromium-dev/browse_thread/thread/e5f0963407ccfb4d/c7bdd506c06646eb)
in creating a custom “Aw-snap” screen to use in kiosk scenarios and reload the
current URL.
Could this API be part of the web platform?
Unlikely, because it is mainly useful in multi-process browser architectures.
While an increasing number of browsers are moving toward such architectures,
it's certainly not a requirement for all web browsers. It also could expose
functionality specific to the Chromium process model in the future.
Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
**Yes. The API will expose functionality that is already present in the Task Manager and unlikely to disappear in the future.**
**The initial version of the API will not include memory details other than the
private memory for the process. The reason is the complexity of adding memory
usage information, which needs to be retrieved in asynchronous model (through a
MemoryDetails object). In addition, for getting the overall memory usage of the
browser, we need platform specific logic for calculating the data. The goal is
to release an initial version to allow feedback on the API and its usage, with a
follow up update adding the extra information.**
List every UI surface belonging to or potentially affected by your API:
The API will not affect UI surface. It will expose internal browser data to
extensions, which can author their own UI based on the data.
How could this API be abused?
Like most extension APIs, it could be used to leak information about the user's
browsing habits, such as the number of processes running or CPU usage patterns.
It could also be used to load attack pages in the same process as another page,
but that can already be done without this API using iframes. It could be used as
a DoS tool by crashing particular renderer processes, but extensions can already
do this by visiting "about:crash" in a given tab.
Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1) Kill random or all processes, causing tabs showing “Oh Snap” screens.
2) Use the terminate method to try and kill processes outside of Chromium.
3)
Alright Doctor, one last challenge:
Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?
No, the API will provide access to the process model only. The only state change
the API will allow is to terminate child processes, which should not lead to any
permanent changes to the system.
How would you implement your desired features if this API didn't exist?
We are not aware of another way to obtain this information. Some of the data
(though not CPU usage) is provided by about:memory, but extensions are not
allowed to load this page to parse its contents. Even if about:memory could be
loaded and parsed by extensions, it would be difficult to match a given tab to a
process simply by its title.
Draft API spec
API reference: chrome.processes
Methods:
**terminate**
**chrome.experimental.processes.terminate(integer processId, optional function callback)**
**Terminates the specified Chrome process. For renderer processes, it is equivalent to visiting about:crash, but without changing the tab's URL.**
**Parameters**

**processId ( integer )**

**The ID of the process to be terminated.**

**Callback function**
**The callback parameter should specify a function that looks like this:**
**function(boolean result) {...}**
**Parameters**

**result ( boolean )**

**True if terminating the process was successful, otherwise false.**

getProcessInfo
**chrome.experimental.processes.getProcessInfo(integer or array of integer
processIds, boolean includeMemory, function callback)**

Retrieves the process information for each process ID specified.
**Parameters**

**processIds ( integer or array of integer )**

**The list of process IDs or single process ID for which to return the process information. An empty list indicates all processes are requested.**

**includeMemory ( boolean )**

**True if detailed memory usage is required. Note, collecting memory usage information incurs extra CPU usage and should only be queried for when needed.**

**callback ( optional function )**

**Called when the processes information is collected.**

**Callback function**
**The callback parameter should specify a function that looks like this:**
**function(object processes) {...}**
**Parameters**

**processes ( object )**

**A dictionary of Process objects for each requested process that is a live child process of the current browser process, indexed by process ID. Metrics requiring aggregation over time will not be populated in each Process object.**

Events:
onCreated
chrome.experimental.processes.onCreated.addListener(function(Process process)
{...});
Fires when a new process is created.
Parameters

process ( Process )

Details of the process that was created. Metrics requiring aggregation over time
will not be populated in the object.

**onExited**
**chrome.experimental.processes.onExited.addListener(function(integer processId, integer exitType, integer exitCode) {...});**
**Fires when a process is terminated, either cleanly or due to a crash.**
**Parameters**

**processId ( integer )**

**The ID of the process that exited.**

**exitType ( integer )**

**The type of exit that occurred for the process - normal, abnormal, killed, crashed. Only available for renderer processes.**

**exitCode ( optional integer )**

**The exit code if the process exited abnormally. Only available for renderer processes.**

**onUnresponsive**
**chrome.experimental.processes.onUnresponsive.addListener(function(Process process) {...});**
**Fires when a process becomes unresponsive.**
**Parameters**

**process ( Process )**

**Details of the unresponsive process. Metrics requiring aggregation over time will not be populated in the object. Only available for renderer processes.**

onUpdated
chrome.experimental.processes.onUpdated.addListener(function(object processes)
{...});
Fires each time the Chrome Task Manager updates its process statistics,
providing a dictionary of updated Process objects, indexed by process ID.
Parameters

processes ( object )

A dictionary of updated Process objects for each live process in the browser,
indexed by process ID. Metrics requiring aggregation over time will be populated
in each Process object.

**onUpdatedWithMemory**
**chrome.experimental.processes.onUpdatedWithMemory.addListener(function(object processes) {...});**
**Fires each time the Chrome Task Manager updates its process statistics, providing a dictionary of updated Process objects, indexed by process ID. Identical to onUpdate, with the addition of memory usage details included in each Process object. Note, collecting memory usage information incurs extra CPU usage and should only be listened for when needed.**
**Parameters**

**processes ( object )**

**A dictionary of updated Process objects for each live process in the browser, indexed by process ID. Memory usage details will be included in each Process object.**

Types:
**Cache**
**( object )**
**The Cache object contains information about the size and utilization of a cache used by Chrome.**
**size ( integer )**

**The size of the cache, in bytes.**

**liveSize ( integer )**

**The part of the cache that is utilized, in bytes.**

**Process**
**( object )**
**The Process object contains various types of information about the underlying OS process. Some of the information, such as cpu or sqliteMemory, needs to be collected over time. Such data is present in the object only when it is part of a callback for the onUpdated or onUpdatedWithMemory events and not for getProcessInfo. Also, privateMemory is expensive to measure, so it is only present in the object when it is part of a callback for onUpdatedWithMemory or getProcessInfo with the includeMemory flag.**
**id ( integer )**
**The unique ID of the process provided by Chrome.**
**osProcessId ( integer )**

**The operating system process ID.**

**type ( string )**
**The type of process. Either browser, renderer, extension, notification, plugin, worker, nacl,**
**utility, gpu, or other.**
**profile ( string )**

**The profile which the process is associated with.**

**tabs ( array of integer )**

**Array of Tab IDs that have a page rendered by this process. The list will be non-empty for renderer processes only.**

**cpu ( optional number )**

**The most recent measurement of the process's CPU usage, between 0 and 100%. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**privateMemory ( optional number )**

**The most recent measurement of the process's private memory usage, in bytes. Only available when receiving the object as part of a callback from onUpdatedWithMemory or getProcessInfo with the includeMemory flag.**

**network ( optional number )**

**The most recent measurement of the process's network usage, in bytes per second. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**jsMemoryAllocated ( optional number )**

**The most recent measurement of the process’s JavaScript allocated memory, in bytes. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**jsMemoryUsed ( optional number )**

**The most recent measurement of the process’s JavaScript memory used, in bytes. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**sqliteMemory ( optional number )**

**The most recent measurement of the process’s SQLite memory usage, in bytes. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**fps ( optional number )**

**The most recent measurement of the process’s frames per section. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**imageCache ( optional object )**

**The most recent information about the image cache for the process. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**scriptCache ( optional object )**

**The most recent information about the script cache for the process. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

**cssCache ( optional object )**

**The most recent information about the CSS cache for the process. Only available when receiving the object as part of a callback from onUpdated or onUpdatedWithMemory.**

Open questions

*   Does the Tab object need a process ID added to it?
    *   After discussion with Charlie Reis, it will be best to be a list
                or tree structure, to accommodate future out of process IFrame
                rendering work. Ideally we can include plugin processes as
                Process objects and form a full tree of all processes involved
                in rendering a single tab.
*   Should Site Instance be exposed through the API?
    *   At this point in time it is hard to come up with a good example
                other than some of our own visualizing/debugging. If any
                compelling usage presents itself, we can see if it justifies
                including the information. For now I don't think it is
                worthwhile information to expose.
*   Should we maintain a list of tabs associated with a process, or
            should we let developers do that by looping over the current tabs?
    *   *Aaron: That seems useful.*

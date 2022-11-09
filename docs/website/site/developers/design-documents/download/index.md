---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: download
title: Download
---

**Diagnose download issues**

Download used to use NetLog for logging purpose, as it is considered part of the
network operation. With network servicification, NetLog will no longer be
accessible outside the network process. As a result, download is switching to
TraceEvent for logging.

# Netlog format (this is deprecated)

For a DownloadItem, its netlog looks like this:

#### 21571799: DOWNLOAD

#### gzip-1.3.9.tar

Start Time: 2017-11-06 11:36:31.319

t=81821 \[st= 0\] +DOWNLOAD_ITEM_ACTIVE \[dt=539\]
--&gt; danger_type = "NOT_DANGEROUS"
--&gt; file_name = "gzip-1.3.9.tar"
--&gt; final_url = "http://ftp.gnu.org/gnu/gzip/gzip-1.3.9.tar"
--&gt; has_user_gesture = true
--&gt; id = "3640"
--&gt; original_url = "http://ftp.gnu.org/gnu/gzip/gzip-1.3.9.tar"
--&gt; start_offset = "0"
--&gt; type = "NEW_DOWNLOAD"
t=81821 \[st= 0\] DOWNLOAD_URL_REQUEST
--&gt; source_dependency = 21571791 (URL_REQUEST)
t=81822 \[st= 1\] DOWNLOAD_FILE_CREATED
--&gt; source_dependency = 21571800 (DOWNLOAD_FILE)
t=81825 \[st= 4\] DOWNLOAD_ITEM_UPDATED
--&gt; bytes_so_far = "0"
t=81833 \[st= 12\] DOWNLOAD_ITEM_SAFETY_STATE_UPDATED
--&gt; danger_type = "MAYBE_DANGEROUS_CONTENT"
t=81865 \[st= 44\] DOWNLOAD_ITEM_RENAMED
--&gt; new_filename = "/usr/local/google/home/qinmin/Downloads/Unconfirmed
246585.crdownload"
--&gt; old_filename = ""
t=82304 \[st=483\] DOWNLOAD_ITEM_UPDATED
--&gt; bytes_so_far = "1648640"
t=82305 \[st=484\] DOWNLOAD_ITEM_UPDATED
--&gt; bytes_so_far = "1648640"
t=82360 \[st=539\] DOWNLOAD_ITEM_RENAMED
--&gt; new_filename = "/usr/local/google/home/qinmin/Downloads/gzip-1.3.9.tar"
--&gt; old_filename = "/usr/local/google/home/qinmin/Downloads/Unconfirmed
246585.crdownload"
t=82360 \[st=539\] DOWNLOAD_ITEM_COMPLETING
--&gt; bytes_so_far = "1648640"
--&gt; final_hash =
"A08ABC68B607A70450930E698A90A0C30C81CE45A656CC47D2B89CA87C059DF6"
t=82360 \[st=539\] DOWNLOAD_ITEM_FINISHED
--&gt; auto_opened = "no"

For a DownloadFile, it looks like:

#### 21571800: DOWNLOAD_FILE

Start Time: 2017-11-06 11:36:31.320

t=81822 \[st= 0\] +DOWNLOAD_FILE_ACTIVE \[dt=538\]
--&gt; source_dependency = 21571799 (DOWNLOAD)
t=81822 \[st= 0\] +DOWNLOAD_FILE_OPENED \[dt=43\]
--&gt; file_name =
"/usr/local/google/home/qinmin/.local/share/.com.google.Chrome.pzFBTU"
--&gt; start_offset = "0"
t=81822 \[st= 0\] DOWNLOAD_STREAM_DRAINED
--&gt; num_buffers = 0
--&gt; stream_size = 0
t=81865 \[st= 43\] -DOWNLOAD_FILE_OPENED
t=81865 \[st= 43\] DOWNLOAD_FILE_RENAMED \[dt=0\]
--&gt; new_filename = "/usr/local/google/home/qinmin/Downloads/Unconfirmed
246585.crdownload"
--&gt; old_filename =
"/usr/local/google/home/qinmin/.local/share/.com.google.Chrome.pzFBTU"
t=81865 \[st= 43\] +DOWNLOAD_FILE_OPENED \[dt=493\]
--&gt; file_name = "/usr/local/google/home/qinmin/Downloads/Unconfirmed
246585.crdownload"
--&gt; start_offset = "0"
t=81883 \[st= 61\] DOWNLOAD_FILE_WRITTEN \[dt=0\]
--&gt; bytes = "2262"

…...
t=82304 \[st=482\] DOWNLOAD_FILE_WRITTEN \[dt=0\]
--&gt; bytes = "2582"
t=82358 \[st=536\] -DOWNLOAD_FILE_OPENED
t=82358 \[st=536\] DOWNLOAD_STREAM_DRAINED
--&gt; num_buffers = 2
--&gt; stream_size = 10898
t=82359 \[st=537\] DOWNLOAD_FILE_RENAMED \[dt=0\]
--&gt; new_filename = "/usr/local/google/home/qinmin/Downloads/gzip-1.3.9.tar"
--&gt; old_filename = "/usr/local/google/home/qinmin/Downloads/Unconfirmed
246585.crdownload"
t=82359 \[st=537\] DOWNLOAD_FILE_ANNOTATED \[dt=0\]
t=82360 \[st=538\] DOWNLOAD_FILE_DETACHED
t=82360 \[st=538\] -DOWNLOAD_FILE_ACTIVE

# Collect trace event

To start tracing, open Chrome and type in chrome://tracing in the URL bar, Click
on the record button and click on the “Edit Categories” button, check the
“download” category and uncheck all other categories, then hit the record
button.

# Migrating to Trace Event

With trace event, we still want to group most of the file operations and
download events under the same download. Otherwise, it will be hard to analyze
the trace if multiple downloads are on going. The downloadId is passed from
DownloadItem to DownloadFile, so it could be used to group all the nestable
events.

Because DownloadFile and DownloadItem live on different thread, DownloadFile’s
trace may outlive DownloadFile’s although the former is created later. As a
result, we will still keep the DownloadFile and DownloadItem events into
separate nestable groups.

Trace event is thread based, so instant events will not be grouped to into any
nestable events. A possible solution is to make any instant events a nestable
async events, but that doesn’t seems very necessary.

The trace event will contain all the information captured by the previous
NetLog, and will be shown in the following format:

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/sGXr-eMkBNj31sftVEkr-7A/image?w=595&h=127&rev=33&ac=1&fmt=svg"
height=127 width=595>

As see in the graph, most of the download file events are grouped under
DownloadFileActive group. Instant events doesn’t belong to any group, and could
take place on different threads.

Events under DownloadFileActive include DownloadFileOpen and DownloadFileWrite.

Events under CrBrowserMain include most of the DownloadItem event, such as
DownloadStarted, DownloadItemUpdated, DownloadItemInterrupted,
DownloadItemCancelled, DownloadItemRenamed, DownloadItemSafetyStateUpdated,
DownloadItemCompleting, DownloadItemFinished and DownloadFileCreated event.

There could be multiple TaskScheduler threads for the write to happen. So events
like DownloadStreamDrained, DownloadFileRename, DownloadFileAnnotate,
DownloadFileDetached.

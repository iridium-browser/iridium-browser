---
breadcrumbs:
- - /audio-video
  - Audio/Video
page_name: media-internals
title: Media Internals
---

# [chrome://media-internals](javascript:void(0);) is a tool to dig into the guts of the Chrome [audio/video stack](/audio-video).

It currently displays 3 things:

1.  Everything it can dig up from the media stack about active media
            players. Includes buffered data, video properties, measured times
            between events, and a log of events.
2.  Status and volume of active audio streams. These are not yet
            associated with a particular tab.
3.  Cache activity, including reads from and writes to the media cache.

## Adding Support for your New Feature

If your code lies in media/ or webkit/glue/, information about it can appear on
chrome://media-internals. Simply get a hold of the MediaLog instance associated
with WebMediaPlayer or PipelineImpl and start logging!

1.  You’ll probably need to add a new event type to
            media/base/media_log_events.h and potentially a helper method or two
            to media/base/media_log.{h,cc}; see those already there for
            reference. Parameter names should be unique!
2.  Add logging calls to your code.
3.  That’s it; you're done. Your events will show up in the Log section,
            and any parameters you passed will show up in Properties.

Well, ok, not quite. You may have noticed the Metrics section. If you’d like a
new time measurement to appear there, simply open up
chrome/browser/ui/webui/media/metrics.js and add the two events between which
you’d like to measure to the list at the top.

## Media Internals Internals

## <img alt="image" src="https://docs.google.com/a/chromium.org/drawings/image?id=sBsZFnDZRDJWcztz8vkkn6Q&w=622&h=495&rev=214&ac=1" height=495px; width=622px;>

MediaLog contains helper methods for creating MediaLogEvents, and
ContentMediaLog implements its AddEvent() function to send a MediaLogEvent to
the browser process as an IPC message. A single instance of MediaLog is
associated with each WebMediaPlayer. This instance currently gets passed down to
PipelineImpl and BufferedResourceLoader and can easily be handed off to other
potential data sources (it’s RefCountedThreadSafe).

MediaInternals is a singleton attached to the IO thread. The IPC messages from
ContentMediaLog get passed to it, and it handles notifications from
AudioRendererHost. It lives in chrome/, but it implements the MediaObserver
interface in content/, allowing AudioRendererHost and RenderMessageFilter (the
IPC message handler) to call it.

MediaInternals talks to [chrome://media-internals](javascript:void(0);) by
packaging up and sending JavaScript function calls to it via MediaInternalsUI.
It also currently stores the information it receives from AudioRendererHost to
hand off to newly created media-internals tabs, but this functionality may be
removed in the future.
MediaInternalsUI is a WebUI component that hosts chrome://media-internals.
There’s not much of interest here, just lots of proxying function calls across
threads.
[chrome://media-internals](javascript:void(0);) contains most of the logic. It
handles MediaLogEvents, NetLog events, and updates from AudioRendererHost. It
infers the contents of the media cache from NetLog events (cache_entry.js,
disjoint_range_set.js) and displays a log of events (event_list.js) and
properties (media_player.js) of each active media player. I also displays time
measurements between pre-defined events (metrics.js) and some basic information
about audio streams from AudioRendererHost.

## Potential Future Work

*   Associate data from AudioRendererHost with data from the MediaLog
            from the same player.
*   Embed a video player in the page, something like scherkus@’s
            IHeartVideo project.

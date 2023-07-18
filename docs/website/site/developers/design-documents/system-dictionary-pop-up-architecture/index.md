---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: system-dictionary-pop-up-architecture
title: System Dictionary Pop-Up Architecture
---

The Mac OS X system function "Lookup in Dictionary" pops open a hover panel that
shows the dictionary definition of the word at which the mouse cursor points.
This functionality is provided by the system but is difficult to make work with
Chromium's multi-process architecture. The underlying implementation uses
-\[NSTextInput characterIndexAtPoint:\] to return the index of the character in
the text stream where the mouse is pointing. Our current accessibility work
provides the text content of the webpage over IPC to the browser process.
Unfortunately, the mapping of points to characters is a prohibitively large data
set and cannot be moved over IPC. Since the underlying implementation of the
dictionary function relies on a synchronous accessibility API, the problem is
further complicated.

One possible implementation is to send a synchronous IPC message from the
browser process to the renderer. This would have to have a very short timeout
because this synchronous message will block the UI thread, which could lead to a
janky experience. But, the browser process, as a general rule, does not send
synchronous messages to the renderer; only the vice versa is allowed. In order
to perform this, then, the browser's IPC channel would need to be made
synchronous as well as asynchronous. However, doing so would make it easier for
people to add other synchronous IPC View messages, which should be avoided at
all costs.

Another option, rather than making the main IPC channel synchronous, would be to
add another channel between the browser and renderer. This shares the same
drawbacks as the first implementation, but at least would separate out the
single synchronous View message into its own member, which could be documented
with verboten warnings. But it also has an additional drawback of requiring
another IPC channel, which would consume another file descriptor per renderer
process. Considering file descriptor exhaustion could already be an issue due to
Chrome's architecture, this is not desirable for the implementation of a single
feature.

The third option is to communicate to the renderer with asynchronous messages
and use thread synchronization techniques to make the communication behave
synchronously. To do this, we use a condition variable and a Singleton member.

A fourth option is to completely bypass how the system implements this using
Accessibility of NSText and instead call the 10.6+ API to show the popup
manually. The API in question is: -\[NSView
showDefinitionForAttributedString:range:options:baselineOriginProvider:\] with
NSDefinitionPresentationTypeKey=NSDefinitionPresentationTypeOverlay in options.
This works well on 10.6, but it means that the browser process would need to
simulate the cmd-ctrl-d-then-keep-cmd-ctrl-down-and-drag-mouse-around logic,
which is unfortunate. On the plus side, this would be completely async.

## Dictionary Pop-Up Internals

This section will detail how Mac OS X brings up the dictionary pop-up. This
examination was done on Mac OS X 10.6.5.

All typical Cocoa apps get the dictionary pop-up behavior for free because
NSTextField and NSTextView conform to NSTextInput (or NSTextInputClient). These
protocols define the three methods used to implement the dictionary
functionality:

1.  -
            (NSAttributedString\*)attributedSubstringFromRange:(NSRange)theRange
2.  - (NSUInteger)characterIndexForPoint:(NSPoint)thePoint
3.  - (NSRect)firstRectForCharacterRange:(NSRange)theRange

The system calls these methods in that order, with this rough pattern:

1.  -attributedStringForRange:{0, 10}
2.  -characterIndexForPoint:{mouseX, mouseY} → returns index **C**
3.  -attributedStringForRange:{**C**, 50} → returns string **S**
4.  -firstRectForCharacterRange:{**C** ± 50, **S\***.length}

It's unclear why each dictionary popup request always starts with (1), but it's
likely some sort of warmup test. The rest is fairly forward: it receives the
index of the character in the text stream over which the mouse points. From
there, it gets the attributed string at that point and the 50 characters after
it. The string it returns is attributed because the highlighted word effect is
done by drawing the attributed string with the gray background over the word
that is being looked up. Finally, it gets the drawing rectangle for the word so
it knows where to position the popup. The range it passes is determined by the
Dictionary framework, which breaks up the 50 character string and finds the
individual word or phrase that is being looked up.

In Chromium, our native view that lives in the browser process conforms to the
NSTextInput protocol but the text stream and web contents live in a separate
process. This means that the access to the necessary data has to be brought
across the process boundary. Unfortunately, the API assumes that everything is
within the same process (a safe assumption for almost all Cocoa applications),
so it is synchronous. To work within that constraint, we use the third option
outlined above: send asynchronous ViewMsg messages to the renderer and then wait
on a condition variable. Each of the 3 NSTextInput methods listed above follow
this pattern in render_widget_host_view_mac.mm, using the example pseudocode
below.

---

### Sample Code:

#### //chrome/browser/cocoa/lookup_in_dictionary.h:

#include "base/singleton.h"

class LookupInDictionary {

public:

// Locks the internal condition for use before the asynchronous message is sent
to the renderer

// to lookup the character index at a given point.

void BeforeRequest() {

lock_.Acquire();

character_index_ = NSNotFound;

}

// Blocks the calling thread with a short timeout after the async message has
been sent to the

// renderer to lookup the character index of a given mouse point. This will
return NSNotFound

// if the timeout expires or if no character at the given point was found,
otherwise it will return

// the character index.

NSUInteger WaitForCharacterIndex() {

condition_.TimedWait(1.5 seconds);

return character_index_;

}

// Called at the end of the critical section. This will release the lock and
condition.

void AfterRequest() {

lock_.Release();

}

// Sets the character index for the last point message. This is called from the
IO thread upon

// receipt of the reply message. This will signal the condition to wake the
sleeping UI thread if it

// has not yet timed out.

void SetCharacterIndexAndSignal(NSUInteger index) {

lock_.Acquire();

character_index_ = index;

lock_.Release();

condition_.Signal();

}

private:

friend struct DefaultSingletonTraits;

LookupInDictionary()

: character_index_(NSNotFound),

lock_(),

condition_(&lock_) {

}

NSUInteger character_index_;

Lock lock_;

ConditionVariable condition_;

};

---

render_widget_host_mac.mm

- (NSUInteger)characterIndexAtPoint:(NSPoint)thePoint {

gfx::Point point(thePoint.x, thePoint.y);

LookupInDictionary\* service = Singleton&lt;LookupInDictionary&gt;::get();

service-&gt;BeforeRequest();

Send(new ViewMsg_CharacterIndexAtPoint(routing_id(), point));

NSUInteger index = service-&gt;WaitForCharacterIndex();

service-&gt;AfterRequest();

return index;

}

---

resource_message_filter.cc

void ResourceMessageFilter::OnCharacterIndexAtPoint(uint index) {

LookupInDictionary\* service = Singleton&lt;LookupInDictionary&gt;::get();

service-&gt;SetCharacterIndexAndSignal(index);

}

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
page_name: usb-api
title: USB API
---

Proposal Date

March 7th, 2012
Who is the primary contact for this API?
gdk@chromium.org
Who will be responsible for this API? (Team please, not an individual)

Apps Extension API Team

Overview
The USB API aims to provide access to fundamental low-level USB operations from
within the context of an extension.
Use cases

- Devices which require third-party drivers to function in ChromeOS are
currently unusable. One of the use cases for this API would be to provide the
ability for a Chrome extension to function as a device driver and allow
previously unusable devices to be used under ChromeOS.

- Conversely, this would also allow device driver implementors to be able to
quickly deploy new driver versions to applications which are written within
Chrome that make use of USB functionality, instead of depending on writing
platform-dependent code.
Do you know anyone else, internal or external, that is also interested in this
API?
Not yet.
Could this API be part of the web platform?

Maybe. This API has the potential to be very controversial, and I believe there
are people within the standards organization that would not back it.

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?

The API should remain fairly stable, as the fundamental mechanism for
interacting with USB devices is standardized and does not change. This API will
surface the underlying USB mechanisms for communication, and so itself should
not need to change.

List every UI surface belonging to or potentially affected by your API:

- The extension installation dialog box (potentially). There should be a
mechanism for informing users about the kinds of USB devices that an extension
wishes to access, and this information should be surfaced.

How could this API be abused?

- This API could be used (without per-extension VID/PID lockdown) to map the
devices attached to a computer, thereby gaining knowledge outside of the scope
of the extension.

- This API could be used to cause physical devices to damage themselves or their
surroundings. I've yet to see a printer catch fire due to software commands, but
I would not rule the possibility out, for example.

Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1) Use a USB-attached device to cause physical harm.
2) Cause permanent harm to a USB device.
3) TODO.
Alright Doctor, one last challenge:
Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?

Yes. There are circumstances where a USB device could be put into a bad state
(or frozen entirely!) that may be impossible to restore to a pristine state
without physically reconnecting the device. The attached devices could also, for
example, themselves leave a physical trace. A printer driver could print a page,
a CD writer could write a disc, a robot karate arm could break a table, etc.

How would you implement your desired features if this API didn't exist?

By writing code that used libusb and then surfacing some form of WebSocket
interface that would allow a client to communicate with my device-specific
server.

Draft API spec

## API reference: chrome.experimental.usb

### Methods

#### bulkTransfer

chrome.experimental.usb.bulkTransfer(integer device, string direction,integer
endpoint, string data, function callback)

Performs a USB bulk transfer to the specified device.

#### Parameters

device

*( integer )*

A device handle on which the transfer is to be made.

direction

*( string )*

The direction of the control transfer. "in" for an inbound transfer, "out" for
an outbound transfer.

endpoint

*( integer )*

The endpoint to which this transfer is being made.

data

*( string )*

(TODO(gdk): ArrayBuffer) The bulk data payload for this transfer.

callback

*( optional function )*

A callback to be invoked with the results of this transfer.

#### Callback function

If you specify the *callback* parameter, it should specify a function that looks
like this:

function(object result) {...};

result

*( object )*

data

*( optional string )*

(TODO(gdk): ArrayBuffer) If the transfer results in inbound data, it is returned
here.

result

*( integer )*

On success, 0 is returned. On failure, -1.

#### closeDevice

chrome.experimental.usb.closeDevice(integer device, undefined callback)

Close a USB device handle.

#### Parameters

device

*( integer )*

The device handle to be closed.

callback

*( optional function )*

A callback function that is invoked after the device has been closed.

#### controlTransfer

chrome.experimental.usb.controlTransfer(integer device, string direction, string
recipient, string type,integer request, integer value, integer index, string
data, function callback)

Performs a USB control transfer to the specified device.

#### Parameters

device

*( integer )*

A device handle on which the transfer is to be made.

direction

*( string )*

The direction of the control transfer. "in" for an inbound transfer, "out" for
an outbound transfer.

recipient

*( string )*

The intended recipient of this message. Must be one of "device", "interface",
"endpoint" or "other".

type

*( string )*

The type of this request. Must be one of "standard", "class", "vendor" or
"reserved".

request

*( integer )*

value

*( integer )*

index

*( integer )*

data

*( string )*

(TODO(gdk): ArrayBuffer) The data payload carried by this transfer.

callback

*( optional function )*

An optional callback that is invoked when this transfer completes.

#### Callback function

If you specify the *callback* parameter, it should specify a function that looks
like this:

function(object result) {...};

result

*( object )*

data

*( optional string )*

(TODO(gdk): ArrayBuffer) If the transfer is inbound, then this field is
populated with the data transferred from the device.

result

*( integer )*

On success, 0 is returned. On failure, -1.

#### createContext

chrome.experimental.usb.createContext(function callback)

Creates a USB context by which devices may be found.

#### Parameters

callback

*( function )*

#### Callback function

The callback *parameter* should specify a function that looks like this:

function(integer result) {...};

result

*( integer )*

The handle to the created context. On failure, -1.

#### destroyContext

chrome.experimental.usb.destroyContext(integer context)

Disposes of a context that is no longer needed. It is not necessary that this
call be made at all, unless you want to explicitly free the resources associated
with a context.

#### Parameters

context

*( integer )*

#### findDevice

chrome.experimental.usb.findDevice(integer context, integer vendorId, integer
productId, function callback)

Locates an instance of the device specified by its vendor and product
identifier.

#### Parameters

context

*( integer )*

A handle to a USB context.

vendorId

*( integer )*

The vendor ID of the device to search for.

productId

*( integer )*

The product ID of the device to search for.

callback

*( function )*

#### Callback function

The callback *parameter* should specify a function that looks like this:

function(integer result) {...};

result

*( integer )*

On success, the handle to the found USB device. On failure, -1.

#### interruptTransfer

chrome.experimental.usb.interruptTransfer(integer device, string direction,
integer endpoint, string data,function callback)

Performs a USB interrupt transfer to the specified device.

#### Parameters

device

*( integer )*

A device handle on which the transfer is to be made.

direction

*( string )*

The direction of the control transfer. "in" for an inbound transfer, "out" for
an outbound transfer.

endpoint

*( integer )*

The endpoint to which this transfer is being made.

data

*( string )*

(TODO(gdk): ArrayBuffer) The interrupt data payload for this transfer.

callback

*( optional function )*

A callback to be invoked with the results of this transfer.

#### Callback function

If you specify the *callback* parameter, it should specify a function that looks
like this:

function(object result) {...};

result

*( object )*

data

*( optional string )*

(TODO(gdk): ArrayBuffer) If the transfer results in inbound data, it is returned
here.

result

*( integer )*

On success, 0 is returned. On failure, -1.

Open questions
Note any unanswered questions that require further discussion.

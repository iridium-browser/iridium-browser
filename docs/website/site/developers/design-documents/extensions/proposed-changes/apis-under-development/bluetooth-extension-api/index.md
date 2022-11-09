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
page_name: bluetooth-extension-api
title: Bluetooth Extension API
---

Proposal Date

Last updated 2012-03-07
Who is the primary contact for this API?
==[keybuk@chromium.org](mailto:keybuk@chromium.org)==

Who will be responsible for this API? (Team please, not an individual)

Apps Extension API Team

Overview
A bluetooth API that is (eventually) *at least* on par with the Android and iOS
APIs. Version 1 will support basic RFCOMM communication. Profile support will be
left for a future version.

Use cases
This API will allow extensions to communicate with bluetooth devices.
Do you know anyone else, internal or external, that is also interested in this
API?

*I'm not aware of anyone at this time.*

Could this API be part of the web platform?
Eventually, yes.

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
I expect the functionality proposed in this API to be stable. Additional
functionality may be added in the future, but changes will be
backwards-compatible.

List every UI surface belonging to or potentially affected by your API:
Bluetooth Settings Panel.

How could this API be abused?
- bluetooth can be very resource intensive, causing battery drain for devices
running extensions using this API

Describe any concerns you have with exposing this API. Particular attention
should be given to issues of security, privacy and performance.
Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1)
2)
3)
Alright Doctor, one last challenge:
Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?
Another main tenet of the Chrome Web Platform is that extensions and apps should
“leave no trace” when they are removed. If someone using your API could leave
lasting changes, we need to know.
How would you implement your desired features if this API didn't exist?
It would be impossible.

Draft API spec

### ### Methods

### #### acceptConnections

### chrome.experimental.bluetooth.acceptConnections(string uuid,
stringservice_name, string service_description, function callback)

### Accept incoming bluetooth connections by advertising as a service.

### #### Parameters

### uuid *( string )*The UUID of the service being advertised.

### service_name *( string )*The human-readable name of the service being
advertised.

### service_description *( string )*The human-readable description of the
service being advertised.

### callback *( function )*Called once for each connection that is established.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(BluetoothSocket result) {...};result *(
[BluetoothSocket](experimental.bluetooth.html#type-BluetoothSocket) )*A
bluetooth socket identifier that can be used to communicate with the connected
device.

### #### connect

### chrome.experimental.bluetooth.connect(BluetoothDevice device, string uuid,
function callback)

### Connect to a service on a bluetooth device.

### #### Parameters

### device *(
[BluetoothDevice](experimental.bluetooth.html#type-BluetoothDevice) )*The target
device of the connection.

### uuid *( string )*The target service of the connection.

### callback *( function )*Called when the connection is established.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(BluetoothSocket result) {...};result *(
[BluetoothSocket](experimental.bluetooth.html#type-BluetoothSocket) )*A
bluetooth socket identifier that can be used to communicate with the connected
device.

### #### disconnect

### chrome.experimental.bluetooth.disconnect(BluetoothSocket socket, function
callback)

### Close the bluetooth connection specified by socket.

### #### Parameters

### socket *(
[BluetoothSocket](experimental.bluetooth.html#type-BluetoothSocket) )*The
bluetooth socket to read from.

### callback *( optional function )*Called with a boolean value to indicate
success.

### #### Callback function

### If you specify the *callback* parameter, it should specify a function that
looks like this:

### function(boolean result) {...};result *( boolean )*True if successful, false
otherwise.

### #### getBluetoothAddress

### chrome.experimental.bluetooth.getBluetoothAddress(function callback)

### Get the bluetooth address of the system.

### #### Parameters

### callback *( function )*Called with the result.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(string result) {...};result *( string )*The bluetooth address of
the system.

### #### getDevicesWithService

### chrome.experimental.bluetooth.getDevicesWithService(string service_uuid,
function callback)

### Request a list of bluetooth devices that support service.

### #### Parameters

### service_uuid *( string )*The UUID of the desired service.

### callback *( function )*Called with a list of bluetooth devices.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(array of BluetoothDevice results) {...};results *( array of
[BluetoothDevice](experimental.bluetooth.html#type-BluetoothDevice) )*An array
of BluetoothDevice objects, all of which provide the specified service.

### #### getOutOfBandPairingData

### chrome.experimental.bluetooth.getOutOfBandPairingData(function callback)

### Get the local Out of Band Pairing data.

### #### Parameters

### callback *( function )*Called with the Out of Band Pairing data.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(array of ArrayBuffer result) {...};result *( array of ArrayBuffer
)*The local Out of Band Pairing data. The array will have length exactly 2 (or
be null, in case of an error).

### #### isBluetoothCapable

### chrome.experimental.bluetooth.isBluetoothCapable(function callback)

### Check if this extension has access to bluetooth.

### #### Parameters

### callback *( function )*Called with the result.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(boolean result) {...};result *( boolean )*True if the extension has
access to bluetooth, false otherwise.

### #### isBluetoothPowered

### chrome.experimental.bluetooth.isBluetoothPowered(function callback)

### Check if the bluetooth adapter has power.

### #### Parameters

### callback *( function )*Called with the result.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(boolean result) {...};result *( boolean )*True if the bluetooth
adapter has power, false otherwise.

### #### read

### chrome.experimental.bluetooth.read(BluetoothSocket socket, function
callback)

### Read data from a bluetooth connection.

### #### Parameters

### socket *(
[BluetoothSocket](experimental.bluetooth.html#type-BluetoothSocket) )*The
bluetooth socket to read from.

### callback *( function )*Called when data is available.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(ArrayBuffer result) {...};result *( ArrayBuffer )*An ArrayBuffer of
data.

### #### setOutOfBandPairingData

### chrome.experimental.bluetooth.setOutOfBandPairingData(string
bluetooth_address, array of ArrayBuffer data,function callback)

### Set the Out of Band Pairing data for the bluetooth device at
bluetooth_address.

### #### Parameters

### bluetooth_address *( string )*The bluetooth address that is supplying the
data.

### data *( array of ArrayBuffer )*An array of length exactly equal to 2
containing the Out of Band Pairing data for the device at bluetooth_address.

### callback *( optional function )*Called with a boolean value to indicate
success.

### #### Callback function

### If you specify the *callback* parameter, it should specify a function that
looks like this:

### function(boolean result) {...};result *( boolean )*True if successful, false
otherwise.

### #### write

### chrome.experimental.bluetooth.write(BluetoothSocket socket, ArrayBuffer
data, function callback)

### Write data to a bluetooth connection.

### #### Parameters

### socket *(
[BluetoothSocket](experimental.bluetooth.html#type-BluetoothSocket) )*The
bluetooth socket to read from.

### data *( ArrayBuffer )*The data to be written.

### callback *( function )*Called with a boolean value to indicate success.

### #### Callback function

### The callback *parameter* should specify a function that looks like this:

### function(boolean result) {...};result *( boolean )*True if successful, false
otherwise.

### ### Events

### #### onBluetoothAvailabilityChange

###
chrome.experimental.bluetooth.onBluetoothAvailabilityChange.addListener(function(boolean
available) {...});

### Fired when the availability of bluetooth on the system changes.

### #### Listener parameters

### available *( boolean )*True if bluetooth is available, false otherwise.

### #### onBluetoothPoweredChange

###
chrome.experimental.bluetooth.onBluetoothPoweredChange.addListener(function(boolean
powered) {...});

### Fired when the powered state of bluetooth on the system changes.

### #### Listener parameters

### powered *( boolean )*True if bluetooth is powered on, false otherwise.

### Types

#### BluetoothDevice

*( object )*Used to identify a specific bluetooth device to the system.

address *( string )*The address of the bluetooth device.

name *( string )*The name of the bluetooth device.

#### BluetoothSocket

*( object )*Used to identify a bluetooth socket to the system.id *( integer
)*The id of the socket.

Open questions
Note any unanswered questions that require further discussion.

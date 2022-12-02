---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: client-api
title: Client API
---

The Aura Client API is an API Aura uses to communicate with the client
application using the Aura system. Since Aura is very simple, providing just
basic window and event types, there are many details it cannot know about
directly. It delegates some functionality to its client (such as window
stacking) and also provides the client API as a conduit for users of Aura to
communicate with the embedding environment.

For example, Aura does not implement tooltips directly, these are a feature
implemented by an client application. The Client API has knowledge of tooltips
however so that a system like Views (which depends on Aura) can specify tooltip
parameters. The client application can interpret this data and show the relevant
UI.

The Aura Window class supports opaque properties, and a list of well known keys
is provided as part of the Client API.

In the ChromeOS Desktop Window Manager, the Aura Client is a component known as
the "Aura Shell" or "Ash":

[<img alt="image"
src="/developers/design-documents/aura/client-api/ClientAPI.png">](/developers/design-documents/aura/client-api/ClientAPI.png)

A breakdown of responsibility between Aura and the Aura Shell (as implemented
for ChromeOS):

<table>
<tr>

<td>### Aura</td>

1.  <td>Basic window type and hierarchy </td>
2.  <td>Event dispatch </td>
3.  <td>Compositor integration </td>
4.  <td>Client API </td>

<td>### Aura Shell</td>

1.  <td>"Window Manager" features like: </td>
    1.  <td>Window dragging and resizing </td>
    2.  <td>Transient and modal windows </td>
    3.  <td>Maximize/Restore/Full Screen </td>
    4.  <td>Tooltips </td>
    5.  <td>Drag and drop </td>
    6.  <td>Window embellishments like shadows </td>
    7.  <td>Window stacking </td>
    8.  <td>Advanced layout heuristics </td>
2.  <td>Shell features like: </td>
    1.  <td>Application launcher(s) </td>
    2.  <td>Customizable desktop background </td>
    3.  <td>Global keyboard shortcuts </td>
    4.  <td>Login/lock screens, screen savers, etc. </td>

</tr>
</table>
The Aura Shell implements the Aura Client API, and uses Aura and Views to
achieve all of this.

In the diagram above, I show a second Aura Client API implementation. This is
what we will end up with when we transition Desktop Chrome on Windows to use the
Aura system, sometime in 2012. This will allow us to hardware accelerate our UI.

With this in mind, it is important to keep the Aura layer of the framework clean
and simple, since much of the ChromeOS DWM functionality and feature set is not
relevant to desktop Chrome, just the basic windowing primitives.

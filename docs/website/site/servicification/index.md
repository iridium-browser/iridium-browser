---
breadcrumbs: []
page_name: servicification
title: Servicification
---

## Summary

The Chromium codebase now supports many platforms and use cases. In response, we
need to migrate the code base to a more modular, service-oriented architecture.
This will produce reusable and decoupled components while also reducing
duplication. It also allows the Chrome team and other groups to experiment with
new features without needing to modify Chrome.

**Contact**

mailing list:
[services-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/services-dev)

## Background

*   [design
            doc](https://docs.google.com/document/d/15I7sQyQo6zsqXVNAlVd520tdGaS8FCicZHrN0yRu-oU/edit#)
*   [presentation](https://drive.google.com/file/d/0BwPS_JpKyELWN3BHSFRicEJ0SDg/view)
            & [slide deck](http://go/servicification-presentation) (both
            internal only)
*   [how to create a
            service](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/servicification.md)
*   [Mandoline](https://docs.google.com/document/d/1AjTsDoY6ugaykfqGLyOHYfp67hMp0tMjDbZcJ5EH9fw/edit#heading=h.otewm6d8oykp):
            experimental browser that we prototyped these ideas in

## Major projects

As part of this effort, we are refactoring existing components of Chromium into
isolated services. Here are some of the major projects underway:

**UI Service**

[UI ](/developers/mus-ash)(aka MUS)

mailing list:
[mustash-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/mustash-dev)

**Network Service**

[Network](https://docs.google.com/document/d/1wAHLw9h7gGuqJNCgG1mP1BmLtCGfZ2pys-PdZQ1vg7M/edit)

mailing list:
[network-service-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/network-service-dev)
[sherifing
notes](https://docs.google.com/document/d/1xjFD9yJVuUtekJe3a9rpBmYqTOvRuGwML3BP2SBvC3s/edit#heading=h.nyhdzanbjl3i)
[conversion cheat
sheet](https://docs.google.com/document/d/1OyBYvN0dwvpqfSZBdsfZ29iTFqGnVS2sdiPV14Z-Fto/edit?usp=sharing)
[bug triaging
instructions](https://docs.google.com/document/d/1hMav0DUXW5ZF67L_g5B92VR-hYEgfUchhU9oHBByeJA/edit?usp=sharing)

**Device Service˜**

[Device](https://bugs.chromium.org/p/chromium/issues/detail?id=612328)

**Identity Service**

[Identity](https://docs.google.com/document/d/1gbS6QjjobxwSyl1FUv-4wct7q4YdLE2KjFF433uPvqI/edit#heading=h.c3qzrjr1sqn7)

[Conversion cheat
sheet](https://docs.google.com/document/d/14f3qqkDM9IE4Ff_l6wuXvCMeHfSC9TxKezXTCyeaPUY/edit)

mailing list:[
identity-service-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/identity-service-dev)

**Past Projects**

*   [Prefs](https://docs.google.com/document/d/1JU8QUWxMEXWMqgkvFUumKSxr7Z-nfq0YvreSJTkMVmU/edit?usp=sharing)
*   [Process/Sandbox](https://bugs.chromium.org/p/chromium/issues/detail?id=654986)

A prioritized list of projects that still need to be done is
[available](https://docs.google.com/document/d/1VB0v_xwd7TqBLEF-5sFsC-zwqveEMS3EgJCtS_enzz8/edit?usp=sharing).

## Code Location

[src/services/](https://chromium.googlesource.com/chromium/src/+/HEAD/services/)

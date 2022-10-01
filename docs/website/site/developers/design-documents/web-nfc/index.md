---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: web-nfc
title: Web NFC
---

W3C Web NFC API implementation in Chromium

Rijubrata Bhaumik
&lt;[rijubrata.bhaumik@intel.com](mailto:rijubrata.bhaumik@intel.com)&gt;

Leon Han &lt;[leon.han@intel.com](mailto:leon.han@intel.com)&gt;

Donna Wu &lt;[donna.wu@intel.com](mailto:donna.wu@intel.com)&gt;

Former: Alexander Shalamov
&lt;[alexander.shalamov@intel.com](mailto:alexander.shalamov@intel.com)&gt;

Last updated: 22 Dec 2020

# Objective

# **This document explains how W3C Web NFC API is implemented in Chromium on both renderer and browser process sides. Future work and implementation challenges are described in “Future work” section of this document.**

# **ED Spec (8 Dec 2020):**

# **W3C Web NFC API[ https://w3c.github.io/web-nfc/](https://w3c.github.io/web-nfc/)**

# Background

# **The[ Web NFC API](https://docs.google.com/document/d/1zowdPEgg_EhIp17MWUg-OhI9VSHv9FXjMMiHCrAiASY/edit#heading=h.nfo3i2oc05zq) enables wireless communication in close proximity between[ active](https://docs.google.com/document/d/1zowdPEgg_EhIp17MWUg-OhI9VSHv9FXjMMiHCrAiASY/edit#heading=h.b6v3ppngc8ml) and[ passive](https://docs.google.com/document/d/1zowdPEgg_EhIp17MWUg-OhI9VSHv9FXjMMiHCrAiASY/edit#heading=h.b6v3ppngc8ml) NFC devices. The means of communication are based on[ NDEF](https://docs.google.com/document/d/1zowdPEgg_EhIp17MWUg-OhI9VSHv9FXjMMiHCrAiASY/edit#heading=h.m4gclg6qqivi) message exchange specification. The API provides simple, yet powerful interfaces to create, read (receive) or write (send) NDEF compliant messages. The NDEF format was chosen to hide low level complexity and laborious handling of various NFC technology types. The NFC APIs are available on Android, Windows (UWP),[ iOS](https://developer.apple.com/documentation/corenfc), Chrome OS and Linux platforms.**

# High level overview

# **The implementation of Web NFC in Chromium consists of two main parts:**

# **The NFC module in Blink located at[ third_party/blink/renderer/modules/nfc/](https://cs.chromium.org/chromium/src/third_party/blink/renderer/modules/nfc/) which contains Blink JS bindings for Web NFC, and the browser side platform level adaptation that is located at[ services/device/nfc](https://cs.chromium.org/chromium/src/services/device/nfc/). The Blink NFC module communicates with the browser adaptation through NFC mojo interface defined in[ nfc.mojom](https://cs.chromium.org/chromium/src/services/device/public/mojom/nfc.mojom) file and implemented by the[ services/device/nfc](https://cs.chromium.org/chromium/src/services/device/nfc/) module. The browser communicates with the android NFC implementation also through [nfc_provider.mojom](https://source.chromium.org/chromium/chromium/src/+/HEAD:services/device/public/mojom/nfc_provider.mojom), in order to get the NFC object associated with which either resumes or suspends NFC operation in case of webpage visibility status change. At the time of writing, only Android platform is supported.**

# **Other platforms provide NFC interfaces and can be supported in[ the future](https://docs.google.com/document/d/1zowdPEgg_EhIp17MWUg-OhI9VSHv9FXjMMiHCrAiASY/edit#heading=h.430ahftk4vy).**

# **[NDEFReader](https://w3c.github.io/web-nfc/#dom-ndefreader) is the primary interface of the Web NFC. The NDEFReader interface has both, write and scan methods:**

# **- The write method is for writing data to an NFC tag. This method returns a promise, which will be resolved when the message is successfully written to an NFC tag, or rejected either when errors happened or process is aborted by setting the AbortSignal in the NDEFWriteOptions.**

# **- The scan method tries to read data from any NFC tag that comes within proximity. Once there is some data found, an NDEFReadingEvent carrying the data is dispatched to the NDEFReader.**

# Detailed design

## Blink module

### Details:

# **<img alt="image" src="https://docs.google.com/drawings/u/1/d/sXWxXeOVTW7-4k_g3YrWR9w/image?w=624&h=447&rev=1361&ac=1&parent=1Q8cCKRJXIMLEw_bOHH6eRjzU9GN2nlpZTRK56knAyfQ" height=447 width=624>**

    # **NFCProxy will be the mojo interface proxy for NDEFReader(s).**

        # **There will be one NFCProxy instance per Document, and its lifecycle
        is bound with the document which will align with the Spec’s description
        “per ExecutionContext”**

    # **NDEFReader Read:**

        # **NFCProxy will have a &lt;reader, watch_id&gt; map to keep all active
        readers.**

        # **NDEFReader::scan() will call NFCProxy::StartReading(reader), which
        will assign a |watch_id| for the |reader| and add it into NFCProxy’s
        active reader list then call device::mojom::blink::NFC::Watch() to make
        a request to DeviceService.**

        # **The promise returned by scan() being resolved means DeviceService
        has already started the scanning process.**

        # **The scanning can be aborted anytime by setting the corresponding
        AbortSignal in NDEFScanOptions, NDEFReader will call
        NFCProxy::StopReading(reader) to remove itself from NFCProxy’s active
        reader list and make a request to DeviceService to cancel.**

        # **Stop itself when document is destroyed or it is destroyed itself.**

        # **This will be an EventTarget and an ActiveScriptWrappable. If the
        reader is in the middle of scanning and there are event listeners
        registered, then it should remain alive until all pending activities get
        done.**

    # **NDEFReader Write:**

        # **It will implement ContextClient for a valid Document to get NFCProxy
        and pass the write request to the NFCProxy for push() method.**

        # **The push() can be aborted anytime by setting the corresponding
        AbortSignal.**

## Android adaptation

# **<img alt="image" src="https://docs.google.com/drawings/u/1/d/s9b1hpRzyLqspPbIWs5MOeg/image?w=624&h=435&rev=1703&ac=1&parent=1Q8cCKRJXIMLEw_bOHH6eRjzU9GN2nlpZTRK56knAyfQ" height=435 width=624>**

# **The most important classes for Android adaptation are[ NfcImpl](https://cs.chromium.org/chromium/src/services/device/nfc/android/java/src/org/chromium/device/nfc/NfcImpl.java),[ NfcTagHandler](https://cs.chromium.org/chromium/src/services/device/nfc/android/java/src/org/chromium/device/nfc/NfcTagHandler.java) and[ NdefMessageUtils](https://cs.chromium.org/chromium/src/services/device/nfc/android/java/src/org/chromium/device/nfc/NdefMessageUtils.java).**

# **NfcImpl class implements mojo NFC interface and uses Android platform NFC APIs to access NFC functionality. NfcTagHandler wraps android.nfc.Tag object and provides a unified interface for reading / writing NDEF messages. NdefMessageUtils has few static methods for conversion between android.nfc.NdefMessage and mojom.NdefMessage objects.**

# **At runtime, the latest NFC.push operation is stored in PendingPushOperation object and as soon as the NFC tag appears within close proximity, write operation is performed. Same pattern is applicable for NFC.watch operations.**

# Runtime view

## NDEFReader#write()

# **<img alt="image" src="https://lh5.googleusercontent.com/pGJyH8SbkcZ8dIxDUmb6w2WJIED5-t44IR-igSv5IAwSL1PIvVzSg7hq2WX0kghDJXSKwHq87sbsJjFBwWMAnOfNc4WOH-ApMTM-2QzRT-RJ5vO8XGA8l1lAHCtz81KFMk_7o1Li" height=397 width=624>**

# **NDEFReader::write() will resolve the promise if there is no error occurred and the message is written to a matched target successfully. In other cases, any errors, or the AbortSignal is set during the process, it will reject the promise.**

## NDEFReader#scan()

# **<img alt="image" src="https://lh3.googleusercontent.com/SseJ6tQdMf97GuUQ3mPfCrmHrMv8sqGE2UYNKtiKmN0a7FNj1O-vCJOVPg_XXpP1bvFUoQnUqRlTPVVpC6EKovMWddphgnZ_MaYRn13lp0REBQlHTRE5aZsCDLQVjsvBn6Ps0qQ5" height=497 width=624>**

# **NDEFReader#scan() may trigger NDEFReader#{onreading,onreadingerror}() events during the whole process. NDEFReader::scan() can be called multiple times on the same reader object but there is maximum only one active at any time point.**

# Future work

    # **Support for ISO-DEP. The ISO-DEP (ISO 14443-4) protocol provides low
    level I/O operations on a NFC tag. This functionality might be required by
    developers who would like to communicate with smart cards or implement
    custom communication mechanisms.**

    # **Support for HCE.[ Host-based Card Emulation
    (HCE)](https://developer.android.com/guide/topics/connectivity/nfc/hce.html)
    would allow web developers to emulate secure element component. This feature
    would enable ticketing, payments, and other complex use-cases.**

    # **Bootstrapping WebVR. HMD enclosure can have NFC tag that contains
    information about lens configuration (dual / single element), FOV, magnet
    button, URL to be opened and other features.**

    # **Support for: Chrome OS, Linux and Windows (UWP) platforms.**

    # **Support for: NFC assisted WebBluetooth handover / pairing.**

    # **Research whether[ iOS Core
    NFC](https://developer.apple.com/documentation/corenfc) can be used to
    implement Web NFC support in Chromium on iOS platform.**

# Security and Privacy

# **Please see the[ UX design](https://docs.google.com/document/d/1ZpPGoMkPwIl-uT9ixE_g3acEZSXdoZAdegMfvmBqh5w/edit#heading=h.o3wrh87n7a7o) for detailed information about the permission model.**

# **<img alt="image" src="https://lh6.googleusercontent.com/JuVnI4n1F5L7oFbaRwHQbbeCDKgchJrWYRo7Y6LD6fS0n_lCwFc9afzyf9PaChDP3sALFQfQpv5vm5pzmpTNiaksqTCady2Wt4M4662cKccdkHWkGbQHtFg5uevVKMCRGzOZoxvK" height=317 width=624>**

# **Web NFC API can be only accessed by top-level secure browsing contexts and user permission is required to access NFC functionality. Web NFC API specification addresses security and privacy topics in chapter[ Security and Privacy.](https://w3c.github.io/web-nfc/#security)**

# Glossary

### \[Web NFC\] W3C Web NFC API[ https://w3c.github.io/web-nfc/](https://w3c.github.io/web-nfc/)

### \[active\] Active NFC device (phones, NFC readers, powered devices)

### \[passive\] Passive NFC device (tags, smart cards, etc)

### \[NDEF\] NFC Data Exchange Format[ NDEF definition](http://nfc-forum.org/our-work/specifications-and-application-documents/specifications/data-exchange-format-technical-specification/)

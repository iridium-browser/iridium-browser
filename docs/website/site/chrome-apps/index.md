---
breadcrumbs: []
page_name: chrome-apps
title: Chrome Apps Migration
---

# Overview

# This page will serve as your guide for how to move Chrome.\* App API to the Web Platform and its APIs.

# Upcoming Changes

# **Our team has identified [these](https://docs.google.com/spreadsheets/d/13Zi0UvTkRAu3HmcDNMm_tRoKnLqrHKM6CuEEtFK3ANM/edit#gid=0) highly used Chrome.\* APIs and we outline below the targeted milestone where those APIs will be coming to the Web Platform. If you use these APIs in your existing Chrome App, you can find out more about these APIs on the [Web Updates page](https://developers.google.com/web/updates/capabilities). If there is an API you use and it isn’t on our [Web Platform Fugu API Roadmap](https://docs.google.com/spreadsheets/d/1de0ZYDOcafNXXwMcg4EZhT0346QM-QFvZfoD8ZffHeA/edit#gid=557099940) , please [file a new feature request](https://developers.google.com/web/updates/capabilities#the_new_capabilities) where you can provide information about your desired use case. As we work towards full deprecation, this feedback will help us determine prioritization of APIs.**

# **Web Supported - Green**

# **Web Partial Support - Yellow**

# **Web Planned - Orange**

# **Web Not Supported - Red**

# **<table>**
# **<tr>**

# **<td>Chrome App feature/functionality</td>**

# **<td>Web Support</td>**

# **<td>Web platform functionality</td>**

# **<td>Origin Trial</td>**

# **<td>Release</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/accessibilityFeatures">chrome.accessibilityFeatures</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developer.chrome.com/extensions/accessibilityFeatures">Requires Chrome Extension</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/alarms">chrome.alarms</a></td>**

# **<td>Planned</td>**

# **<td><a href="http://crbug.com/891339">Notification Triggers</a></td>**

# **<td>78</td>**

# **<td>81</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_runtime">chrome.app.runtime</a></td>**

# **<td>Planned</td>**

# **<td><a href="https://bugs.chromium.org/p/chromium/issues/detail?id=844279">Launch Event</a></td>**

# **<td>79</td>**

# **<td>82</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_runtime#event-onRestarted">chrome.app.runtime.onRestarted</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developers.google.com/web/updates/2018/07/page-lifecycle-api">Page Life Cycle</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_window">chrome.app.window</a></td>**

# **<td>Planned</td>**

# **<td><a href="https://crbug.com/897300">Window Placement / Screen Enumeration</a></td>**

# **<td>83</td>**

# **<td>86</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/audio">chrome.audio</a></td>**

# **<td>Planned</td>**

# **<td><a href="http://crbug.com/897326">Audio Device Client</a></td>**

# **<td>84</td>**

# **<td>87</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_bluetooth">chrome.bluetooth</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/5264933985976320">Web Bluetooth API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/bluetoothLowEnergy">chrome.bluetoothLowEnergy</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/5264933985976320">Web Bluetooth API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/bluetoothLowEnergy">chrome.bluetoothLowEnergy Peripheral</a></td>**

# **<td>Not Supported</td>**

# **<td>Support via ARC++<a href="https://developer.android.com/guide/topics/connectivity/bluetooth-le"> BTLE</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/bluetoothSocket">chrome.bluetoothSocket</a></td>**

# **<td>Not Supported</td>**

# **<td>Support via ARC++<a href="https://developer.android.com/guide/topics/connectivity/bluetooth-le"> BTLE</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/browser">chrome.browser</a></td>**

# **<td>Supported</td>**

# **<td>window.open</td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/commands">chrome.commands</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://w3c.github.io/uievents/">UI Events</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/contextMenus">chrome.contextMenus</a></td>**

# **<td>Supported</td>**

# **<td>(HTML/script)</td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/documentScan">chrome.documentScan</a></td>**

# **<td>Not Supported</td>**

# **<td>Via extensions<a href="https://developer.chrome.com/extensions/documentScan"> chrome.documentScan</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/events">chrome.events</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://bugs.chromium.org/p/chromium/issues/detail?id=891339">Alarms</a></td>**

# **<td>78</td>**

# **<td>81</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/extensionTypes">chrome.extensionTypes</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developer.chrome.com/extensions/extensionTypes">Extension with extensionTypes</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/fileSystem">chrome.fileSystem</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="http://crbug.com/853326">Native FileSystem API</a></td>**

# **<td>78</td>**

# **<td>84</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/fileSystemProvider">chrome.fileSystemProvider</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developer.chrome.com/extensions/fileSystemProvider">Extension w/ fileSystemProvider</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/gcm">chrome.gcm</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://developers.google.com/web/fundamentals/push-notifications/">Web Push Notifications</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/hid">chrome.hid</a></td>**

# **<td>Planned</td>**

# **<td><a href="https://www.chromestatus.com/feature/5172464636133376">Web HID API</a></td>**

# **<td>84</td>**

# **<td>87</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/i18n">chrome.i18n</a></td>**

# **<td>Supported</td>**

# **<td>(HTML/script)</td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/identity">chrome.identity</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/5669923372138496">Workaround using OAuth API or Credential Management API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/idle">chrome.idle</a></td>**

# **<td>Planned</td>**

# **<td><a href="http://crbug.com/878979">User Idle Detection API</a></td>**

# **<td>82</td>**

# **<td>85</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/instanceID">chrome.instanceID</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developers.google.com/web/fundamentals/push-notifications/">Web push</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/mdns">chrome.mdns</a></td>**

# **<td>Not Supported</td>**

# **<td>Via partial support from ARC++<a href="https://developer.android.com/training/connect-devices-wirelessly/nsd"> NDS Discovery</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/mediaGalleries">chrome.mediaGalleries</a></td>**

# **<td>Planned</td>**

# **<td><a href="http://crbug.com/853326">Native FileSystem API</a></td>**

# **<td>77</td>**

# **<td>84</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/networking_onc">chrome.networking.onc</a></td>**

# **<td>Not Supported</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/notifications">chrome.notifications</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://www.chromestatus.com/feature/5064350557536256">Notifications API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/permissions">chrome.permissions</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://developer.mozilla.org/en-US/docs/Web/API/Permissions_API">Web Permissions API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/power">chrome.power</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/4636879949398016">WakeLock API</a> - Screen wake lock, but not System wake lock</td>**

# **<td>78</td>**

# **<td>82</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/printerProvider">chrome.printerProvider</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developer.chrome.com/extensions/printerProvider">Transition to Extension</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/runtime">chrome.runtime</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developers.google.com/web/updates/2018/07/page-lifecycle-api">Service Workers + Page Lifecycle API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/messaging#connect">chrome.runtime.connect</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/6710044586409984">Channel Messaging API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/messaging#simple">chrome.runtime.sendMessage</a></td>**

# **<td>Not Supported</td>**

# **<td>Only between Android apps / Only within ARC++</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_serial">chrome.serial</a></td>**

# **<td>Planned</td>**

# **<td><a href="https://www.chromestatus.com/feature/6577673212002304">Web Serial API</a></td>**

# **<td>80</td>**

# **<td>83</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_network">chrome.sockets.tcp</a></td>**

# **<td>Not Supported</td>**

# **<td><a href="https://developer.android.com/reference/java/net/Socket">Partial ARC++ support through android.net.ConnectivityManager and Java standard socket API</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/sockets_tcpServer">chrome.sockets.tcpServer</a></td>**

# **<td>Not Supported</td>**

# **<td><a href="https://developer.android.com/reference/java/net/Socket">Partial ARC++ support via android.net.ConnectivityManager and Java standard socket API</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/sockets_udp">chrome.sockets.udp</a></td>**

# **<td>Not Supported</td>**

# **<td><a href="https://developer.android.com/reference/java/net/DatagramSocket">Partial support via ARC++ DatagramSocket API for UDP</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/storage">chrome.storage</a></td>**

# **<td>Partial Support</td>**

# **<td>Cache API:<a href="https://www.chromestatus.com/feature/5072127703121920"> https://www.chromestatus.com/feature/5072127703121920</a></td>**

# **<td>IndexedDB:<a href="https://www.chromestatus.com/feature/6507459568992256"> https://www.chromestatus.com/feature/6507459568992256</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/extensions/manifest/storage">chrome.storage.managed</a></td>**

# **<td>Partial Support</td>**

# **<td>Support via ARC++ or Extension with<a href="https://developer.chrome.com/extensions/storage#property-managed"> storage.managed</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/syncFileSystem">chrome.syncFileSystem</a></td>**

# **<td>Partial Support</td>**

# **<td>Not supported - Integration with Drive restful API but no OS intergation</td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developers.chrome.com/apps/system.cpu">chrome.system.cpu</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/6248386202173440">navigator.hardwareConcurrency</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developers.chrome.com/apps/system.display">chrome.system.display</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://drafts.csswg.org/cssom-view/#the-screen-interface">window.screen</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developers.chrome.com/apps/system.memory">chrome.system.memory</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/5119701235531776">navigator.deviceMemory</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developers.chrome.com/apps/system.network">chrome.system.network</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/6338383617982464">Network Information API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developers.chrome.com/apps/system.storage">chrome.system.storage</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://www.chromestatus.com/feature/5630353511284736">navigator.storage</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developers.chrome.com/apps/tts">chrome.tts</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://www.chromestatus.com/feature/4782875580825600">Web Speech API (Synthesis)</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/types">chrome.types</a></td>**

# **<td>Not Supported</td>**

# **<td>Extension with<a href="https://developer.chrome.com/extensions/types"> chrome.types</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/app_usb">chrome.usb</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://www.chromestatus.com/feature/5651917954875392">Web USB API</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/virtualKeyboard">chrome.virtualKeyboard</a></td>**

# **<td>Not Supported</td>**

# **<td>Partial support via ARC++<a href="https://developer.android.com/training/keyboard-input"> Soft Input Method</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/vpnProvider">chrome.vpnProvider</a></td>**

# **<td>Not Supported</td>**

# **<td>Partial support via ARC++<a href="https://developer.android.com/reference/android/net/VpnService"> VpnService</a> or Extensions<a href="https://developer.chrome.com/extensions/vpnProvider"> chrome.vpnProvider</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/wallpaper">chrome.wallpaper</a></td>**

# **<td>Not Supported</td>**

# **<td>Support via ARC++<a href="https://developer.android.com/reference/android/app/WallpaperManager"> WallpaperManager</a> or Extensions<a href="https://developer.chrome.com/extensions/wallpaper"> https://developer.chrome.com/extensions/wallpaper</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/manifest/externally_connectable">externally_connectable</a></td>**

# **<td>Not Supported</td>**

# **<td><a href="https://developer.chrome.com/extensions/content_scripts#host-page-communication">Use Window.postMessage</a> to communicate with Extensions</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/manifest/kiosk_enabled">kiosk_enabled</a></td>**

# **<td>Planned</td>**

# **<td>PWAs will be supported in Kiosk mode.</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/manifest/minimum_chrome_version">minimum_chrome_version</a></td>**

# **<td>Not Supported</td>**

# **<td>Use feature detection or via Extenisions<a href="https://developer.chrome.com/apps/manifest/minimum_chrome_version"> minimum_chrome_version</a></td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/offline_apps">offline_enabled</a></td>**

# **<td>Supported</td>**

# **<td><a href="https://developers.google.com/web/fundamentals/codelabs/offline/">Offline via Service Workers</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/native-client">NaCl</a></td>**

# **<td>Partial Support</td>**

# **<td><a href="https://developer.chrome.com/native-client/migration">Migration Guide</a></td>**

# **<td>Shipped</td>**

# **</tr>**
# **<tr>**

# **<td><a href="https://developer.chrome.com/apps/tags/webview">&lt;webview&gt; tag</a></td>**

# **<td>Not Supported</td>**

# **</tr>**
# **</table>**

## Relevant Links

    # **[Web Platform Fugu API
    Roadmap](https://docs.google.com/spreadsheets/d/1de0ZYDOcafNXXwMcg4EZhT0346QM-QFvZfoD8ZffHeA/edit#gid=557099940)**

    # **F[ile a new feature
    request](https://developers.google.com/web/updates/capabilities#the_new_capabilities)**

    # **[Android development for Chrome
    OS](https://developer.android.com/chrome-os)**

    # **Past announcement from blog:
    [Link](https://blog.chromium.org/2016/08/from-chrome-apps-to-web.html) to
    2016 announcement.**

## FAQ

### Developers

What are my next steps for converting my existing Chrome app?

Migrating to web api: <https://developers.chrome.com/apps/migration>

Migrating to Android for Chrome OS: https://developer.android.com/chrome-os

Where can I find a list of new Web Platform Fugu APIs that corresponds to
existing Chrome app APIs?

See this[
tracker](https://docs.google.com/spreadsheets/d/1de0ZYDOcafNXXwMcg4EZhT0346QM-QFvZfoD8ZffHeA/edit#gid=272423396).

I’m migrating a Chrome App to PWA and missing an API, will an equivalent API be
available? If not, how can I request a new Web Capability?

See <https://developers.google.com/web/updates/capabilities>; request APIs using
this [link](https://goo.gl/qWhHXU).

I’ve created a PWA or Android app to replace my Chrome App, what do I need to do
to help transition my users?

If you are migrating users to PWAs, you need to add the replace_web_app field
and the url for hosting the new PWA. (note, this field exists since M75+)

If you are migrating users to ARC++, you need to add the replacement_android_app
field and the package name for the replacement Android App. (note, this field
will be added for Chrome M81+).

Once you have defined one of the two fields, you also need to invoke
chrome.management.installReplacementWebApp() in your app to prompt Chrome for
the migrating dialogue.

See the example definition in <https://developer.chrome.com/apps/manifest>.

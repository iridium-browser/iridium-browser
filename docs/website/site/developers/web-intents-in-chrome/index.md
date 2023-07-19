---
breadcrumbs:
- - /developers
  - For Developers
page_name: web-intents-in-chrome
title: Web Intents in Chrome
---

[TOC]

**# Introduction to Web Intents**

**First, what is web intents? Imagine you're building a web app and you want to allow users to edit pictures. You could write that functionality yourself, but it will require a huge amount of work and likely won't be nearly as good as other existing photo web apps. You could hard-code integration with a collection of existing photo editing web apps, but that can lead to a cluttered interface and requires manual intervention later to integrate with new photo editing apps that may emerge. Worse, it forces your chosen integrations upon your end users. One of the greatest strengths of the web is that the the ease of linking allows innovative new apps to succeed without asking anyone else's permission--but up until now that hasn't applied to integrations between web apps.**
**Web Intents is an emerging [W3C
specification](http://dvcs.w3.org/hg/web-intents/raw-file/tip/spec/Overview.html)
inspired by Android's [Intents
system](http://developer.android.com/guide/topics/intents/intents-filters.html)
that aims to solve that problem. Web apps can register as a service that
provides specific types of functionality. Later, other client web apps can
request that type of functionality, and the browser will mediate the connection,
allowing the user to pick which application to service the request--or easily
discover compatible applications from the Chrome Web Store. The apps don't need
to know anything about one another. This video gives a quick overview:**

**Web Intents makes web developers' lives easier--for client web apps using web services, it means you have to do less work to include functionality that others have already built, and for service web apps providing those services it means your app can succeed on its quality alone--not which hardcoded integrations you happen to have negotiated.**
**But the real benefit comes for users, who get to use the apps they choose themselves from among a richer ecosystem of powerful, interconnected apps.**

**# Status of the API**

**Web Intents is enabled as an experimental API starting in Chrome 19 in order to gather feedback from web developers to shape the future of the API. In particular, this experimental version is prefixed and only allows applications to register as services in their Chrome Web Store app manifest.**
**This is an experimental API that will change over time with feedback from real world usage, potentially in backwards-incompatible ways. If you choose to experiment with Web Intents, be sure to follow the[ Web Intents Google + page](https://plus.google.com/116171619992010691739/posts), where we'll announce any impending breaking changes in Chrome's implementation based on the feedback we receive from this experimental release. You can provide feedback on the API or ask questions on the [Web Intents discussion list](https://groups.google.com/forum/?fromgroups#!forum/web-intents-discuss).**

**# Walkthrough of an example**

**See the [reference](/developers/web-intents-in-chrome#TOC-Reference) section for low-level API documentation.**
**[Imagemator.com](http://www.imagemator.com/) is a simple image workbench app
that uses Web Intents for most of its functionality.**

**[<img alt="image"
src="/developers/web-intents-in-chrome/Screen%20shot%202012-05-09%20at%207.43.31%20PM.png">](/developers/web-intents-in-chrome/Screen%20shot%202012-05-09%20at%207.43.31%20PM.png)**

It does not host any complex functionality, instead it orchestrates the use of the features of other apps to provide a rich user experience.**
The user will need to pick an image to start, once selected the user will be presented with a Choice of service to use that support the “[pick](http://webintents.org/pick) image” intent, the user will choose their preferred service to select the image of their choice.**
If the user doesn't have any apps installed that can handle the intent, they'll
see a list of suggested apps that they can install with one click.**

[<img alt="image"
src="/developers/web-intents-in-chrome/noinstalls.PNG">](/developers/web-intents-in-chrome/noinstalls.PNG)

**If, however, the user already has apps installed that can handle the intent,
they'll see a list like this:**

[<img alt="image"
src="/developers/web-intents-in-chrome/with.PNG">](/developers/web-intents-in-chrome/with.PNG)

**In our case, the user might choose CloudFilePicker.**

[<img alt="image"
src="/developers/web-intents-in-chrome/Screen%20shot%202012-05-11%20at%2012.02.41%20AM.png">](/developers/web-intents-in-chrome/Screen%20shot%202012-05-11%20at%2012.02.41%20AM.png)

**The user selects the file and is returned back to the original application
with the data.**

[<img alt="image"
src="/developers/web-intents-in-chrome/Screen%20shot%202012-05-14%20at%204.48.28%20PM.png">](/developers/web-intents-in-chrome/Screen%20shot%202012-05-14%20at%204.48.28%20PM.png)

**The user can now choose three options: [Edit](http://webintents.org/edit),
[Save](http://webintents.org/save) or [Share](http://webintents.org/share). It
is likely that the user will want to edit the image before sharing it to their
friends. When the user clicks “Edit” the browser will resolve all the services
that support “image editing” and offer them the choice of their preferred
service. When the service has been chosen the browser packages up all the data
from imagemator.com and passes it to the service app (in this case
<http://mememator.com>)**

[<img alt="image"
src="/developers/web-intents-in-chrome/Screen%20shot%202012-05-14%20at%204.56.03%20PM.png">](/developers/web-intents-in-chrome/Screen%20shot%202012-05-14%20at%204.56.03%20PM.png)

**A lot has happened in this workflow, luckily the code is pretty simple. Let’s dive into the code that made this all possible.**
**The user can't do much without an image, but Imagemator doesn't have built in functionality to choose an image--so it will fire a pick intent. The code for this is as follows:**

**var choose = document.getElementById('choose')**

**choose.addEventListener('click', function(e) {**

**var params = {**

**'action': '<http://webintents.org/pick>',**

**'type': 'image/\*'**

**};**

**var intent = new WebKitIntent(params);**

**window.navigator.webkitStartActivity(intent, function(data) {**
**loadImage(data);**
**});**
**}, false);**

**Lets break down the code:**

1.  **Create an intent object. The intent object describes the data that
            will be used by the browser to resolve the list of services that the
            user can use.**

**To do this, we create a “parameters” dictionary. The dictionary contains a bag of parameters that are used by the browser to determine the services to present to the user.**

**var params = {**

**'action': '<http://webintents.org/pick>',**

**'type': 'image/\*'**

**};**

**var intent = new WebKitIntent(params);**

**In this example we are telling the browser to find a list of services that support the “pick” (action) intent and also support that operation on “images” (type). The type argument is constructed as a MIME type: image/png represents png’s, video/webm represents web videos and so on. The type argument can also be a wildcard substitution where “image/\*” is shorthand for every image type.**

1.  **Now that the intent object has been created we need to ask the
            browser to find the services to use to satisfy the request. This can
            be achieved using the following code:**

**var onSuccess = function(data) {**

**loadImage(data);**

**};**

**var onFailure = function() {**

**// perform error logic**

**}**

**window.navigator.webkitStartActivity(intent, onSuccess, onFailure);**

**startActivity takes three parameters: the intent, the success callback and the failure callback.**

**The success callback receives one parameter “data”. This is the data that is returned from the service the user chose. Once we have that data we can do anything we want with it - in our case loading it into an image object.**

**The failure callback receives no data, but can be used to provide feedback to the user that they might need to try and re-perform the action. Failures occur when the user cancels the action in the webpage or there is an error such as Authentication issues that the service app wants to let the calling app know something is wrong. In the future there may be more reasons why the error handler is called; this is an active area of exploration in the specification.**

**For an application to be available in this list it has to first be in the Chrome Web Store with an intents declaration as follows:**

**{**

**"name": "Cloud File Picker",**

**"version": "1.0.0.6",**

**"icons" : {**

**"128" : "128.png"**

**},**

**"app" : {**

**"urls" : \["http://www.cloudfilepicker.com/"\],**

**"launch" : {**

**"web_url" : "http://www.cloudfilepicker.com/"**

**}**

**},**

**"intents": {**

**"http://webintents.org/pick" : \[{**

**"type" : \["image/png", "image/jpg", "image/jpeg", "image/bmp"\],**

**"href" : "<http://www.cloudfilepicker.com/index.html>",**

**"disposition" : "window",**

**"title" : "Pick file from CloudFilePicker.com"**

**}\]**

**}**

**}**

**This small piece of JSON in the manifest is a dictionary that describes the services your applications offer. In this case, we are telling Chrome that our app supports the [pick intent](http://webintents.org/pick). Each intent declaration is an array of objects that contain the types (“type”) of data our app can work on, what to launch (“href”) and the title to present to the user in the service picker.**
**More information about the manifest declaration can be found on [code.google.com](http://code.google.com/chrome/extensions/beta/manifest.html#intents). Once you have this in your manifest you are ready to roll.**
**The data will be available on window.webkitIntent. Because we are handling the pick intent, we don’t need to access any of the data on the intent, but rather send the selected data back to the user. Other intent types may have different rules.**

**var select = document.getElementById('select')**

**select.addEventListener('click', function(e) {**

**var intent = window.webkitIntent;**

**var blob = /\* construct a blob of the image \*/**

**intent.postResult(blob);**

**}, false);**

**This is all that needs to be done. We find the intent object that has been passed in to the client and call “postResult” to return the data back to the calling app.**
**To be able to edit the image that the user has selected, you can invoke the “[edit](http://webintents.org/edit)” intent as follows:**

**var edit = document.getElementById('edit')**

**edit.addEventListener('click', function(e) {**

**var params = {**

**'action': '<http://webintents.org/edit>',**

**'type': 'image/\*',**

**'data': toBlob(img) // convert dataURI to Blob**

**};**

**var intent = new WebKitIntent(params);**

**var onSuccess = function(data) {**

**loadImage(data);**

**};**

**var onFailure = function() {**

**// perform error logic**

**}**

**window.navigator.webkitStartActivity(intent, onSuccess, onFailure);**

**}, false);**

**The code for “edit” is very similar to “pick”, and this is the point of Web Intents. Apps now only need to care about how to create the data to send between apps and not how to send the data or discover the services to integrate with.**
**The demo so far has only ever opened applications in a new window. Applications can however, be opened in two modes: new window context and inline. The new window context, the default, will open up a brand new tab and direct the user’s focus to this experience; the inline context allows the user to keep the existing task in focus whilst presenting the new app in a small region over the content.**
**Currently inline intents will show up in a fixed window size (300px by 300px),
but in the future the inline intents window will expand to fit the contents of
the service.**

[<img alt="image"
src="/developers/web-intents-in-chrome/Screen%20shot%202012-05-10%20at%2012.40.51%20AM.png">](/developers/web-intents-in-chrome/Screen%20shot%202012-05-10%20at%2012.40.51%20AM.png)

**Web intents is a powerful model with many available actions ([share](http://webintents.org/share), [edit](http://webintents.org/edit), [save](http://webintents.org/save), [pick](http://webintents.org/pick)) allowing us to build eco-systems of functionality that developers can take advantage of to deliver great apps. By opening your existing application up via the intents system you are enabling a new set of client applications and users to discover, install and use your applications in ways that you might not have imagined.**

**# FAQ**

**## What's the support like in other browsers?**

**Web Intents is an emerging specification that at the moment is only supported in Chrome (starting in version 19). Other browsers have expressed interest in the concept, but haven't announced formal plans at this time. Limited [shims exist](http://webintents.org/#javascriptshim), but we recommend feature detection to support browsers that don't have Web Intents support.**

**## How can I influence the API's development?**

**First and foremost, by using it! It's impossible to design APIs in a vacuum, especially for ones as potentially complex as Web Intents. Make sure you're subscribed to the [Web Intents discussion list](https://groups.google.com/forum/?fromgroups#!forum/web-intents-discuss), and send any comments, questions, or suggestions you have. If you're interested in directly influencing the specification, we encourage you to participate in the [W3C mailing list](http://lists.w3.org/Archives/Public/public-web-intents/).**

**## What types of changes are expected in the future?**

**Web Intents is an evolving specification, and it's impossible to know with certainty how it will develop in the future. However, here are some features that may be added in the future:**

*   **Explicit intents: allows the client app developer to specify the
            exact service to use without showing the picker.**
*   **Embedded intents: an extension of explicit intents which allows
            the client to connect to a client-supplied iframe embedded in the
            client page.**
*   **Intent suggestions: allows the client app developers to specify a
            list of potential services that the user can use.**
*   **URL filtering: analogous to Androids URL filtering. By letting the
            developer define a URL pattern that encompasses the scope of the
            app.**
*   **User-configured default services**
*   **Protocol registration: enabling intents to be able to register
            handlers for mailto etc**

**## Why is this an "experimental" API?**

**Although the Web Intents specification has been developed in the open with the input of many people, it's still difficult to predict exactly how it will be used in practice. Web Intents is an experimental API because we want to incorporate feedback from real web developers to ensure the feature is as useful as it can be.**
**This means that in the future there may be backwards-incompatible changes. To be alerted of breaking changes in upcoming versions of Chrome, follow our [Google + page](https://plus.google.com/116171619992010691739/posts). We'll also send announcements to the discussion list.**

**## I'm having trouble getting it working. Where can I go for help?**

**Send any comments, questions, or suggestions to the [Web Intents discussion list](https://groups.google.com/forum/?fromgroups#!forum/web-intents-discuss).**

**## How will breaking changes be communicated?**

**We'll give advance warning of breaking changes on our [Google + page](https://plus.google.com/116171619992010691739/posts). We'll also send announcements to the discussion list.**

**## What intents are available now?**

**There are several defined at <http://webintents.org/>**

*   **<http://webintents.org/share> - Share - when the user wants to
            share**
*   **<http://webintents.org/view> - View - when the user wants to open
            a file or data in another application**
*   **<http://webintents.org/edit> - Edit - when the user wants to edit
            data in another application**
*   **<http://webintents.org/save> - Save - when the user needs to save
            data to another app**
*   **<http://webintents.org/pick> - Pick - when the user needs to
            retrieve data from another app**
*   **<http://webintents.org/subscribe> - Subscribe - when the user
            wants to subscribe to a resource such as an RSS feed**

**## Can we define our own intents?**

**Yes. The “action” verb is a string so you have the ability to define your own actions.**
**We are using URI’s to define actions. URIs allows us to namespace the action verb to avoid conflicts and it also gives us the ability to provide a place for the developer to go to read the documentation, although there is nothing special about "webintents.org".**

**## Should we define our own intents?**

**Try to see if your action will fit into an existing scheme. This will allow you to take advantage of existing app ecosystems that use that. If there is no verb that matches then create a new action.**

**# Staying in the loop**

**Remember that this is an experimental API that is evolving rapidly. In the future we may make breaking changes to the API. Only use the API if you can commit to staying current with any changes that may occur.**
**To be alerted of breaking changes in upcoming versions of Chrome, follow our [Google + page](https://plus.google.com/116171619992010691739/posts). We'll also send announcements to the discussion list.**
**To ask for help, talk about the implementation in Chrome, float informal ideas, or general discussion, use the [Web Intents discussion list](https://groups.google.com/forum/?fromgroups#!forum/web-intents-discuss).**
**To participate in formal discussions about evolving the specification,
participate in the [W3C mailing
list](http://lists.w3.org/Archives/Public/public-web-intents/).**

---

**## Reference**

**Web Intents allows your application to quickly communicate with other applications on the user’s system and inside their browser. Your application can register to handle specific user actions such as “Editing” “Images” via the manifest.json; your application can also invoke actions to be handled by other applications.**

**## Configuring Intents**

**### Register to handle an action**

**The [Web Intents specification](http://dvcs.w3.org/hg/web-intents/raw-file/tip/spec/Overview.html) describes a mechanism to register your applications abilities with the browser via a special &lt;intent&gt; HTML tag.**
**In Chrome M19, to register the ability for your application to be able to handle specific actions you are required to host your application in the Chrome Web Store AND to add an [intent section](http://code.google.com/chrome/extensions/beta/manifest.html#intents) to your [manifest](http://code.google.com/chrome/extensions/beta/manifest.html) that describes the actions your app supports.**

**#### Example**

**"intents":{**
**"http://webintents.org/edit" : \[{**
**"title" : "Best Image editing app",**
**"type" : \["image/\*"\],**
**"href" : "/index.html"**
**}\]**
**}**

**### Handling just Content-types**

**Your application can be the user’s preferred choice for handling a file type. For example, your application could handling viewing images or viewing pdfs.**
**As of Chrome version 19, only the following content-types may be registered:**
**All Operating Systems**

*   **"application/rss+xml"**
*   **"application/atom+xml"**

**ChromeOS-only**

*   **"application/msword"**
*   **"application/vnd.ms-powerpoint"**
*   **"application/vnd.ms-excel"**
*   **"application/vnd.openxmlformats-officedocument.wordprocessingml.document"**
*   **"application/vnd.openxmlformats-officedocument.presentationml.presentation"**
*   **"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"**

**Over time we may add more content-types to the list.**

**#### Required Manifest Items**

**You must supply the intent in the manifest and use the “<http://webintents.org/view>” action**

**#### Example**

**"intents": {**
**"http://webintents.org/view" : \[{**
**"title" : "RSS Feed Reader",**
**"type" : \["application/atom+xml", "application/rss+xml"\],**
**"href" : "/index.html"**
**}\]**
**}**

**### Controlling how your app is opened**

**You can control how your app is opened by users by defining a “disposition”. You currently have two options: open in a new window specified by the disposition “window”, and open in the intent picker specified by the disposition “inline”.**

**#### Required Manifest Items**

**You must supply the intent in the manifest and you can optionally add in a “disposition”, by default it is “window”.**

**#### New Window Example**

**"intents":{**
**"http://webintents.org/edit" : \[{**
**"title" : "Best Image editing app",**
**"type" : \["image/\*"\],**
**"href" : "/index.html",**
**"disposition" : "window"**
**}\]**
**}**
**<img alt="image" src="https://lh4.googleusercontent.com/OYOr5ooakp79Q0N6XcQ2CVcpE9KPwENmW0sv4NavGlrq4-B_2yrWAGDmwR1TZtSHXdC5BdD10boOPrZFVtGVoJh6rJBRsz6BiQNAEmLb4Nw5SOYebEY" height=318px; width=493px;>**

**#### Inline Example**

**"intents": {**
**"http://webintents.org/edit" : \[{**
**"title" : "Best Image editing app",**
**"type" : \["image/\*"\],**
**"href" : "/index.html",**
**"disposition" : "inline"**
**}\]**
**}**
**<img alt="image" src="https://lh3.googleusercontent.com/rKFBstgk0oxp9UuMYjtXB068n1isi_lpqOoYXwPeljy3hwyjfpGttn4iR-j9bcx3n3Eu8f0sK3oJBaO2DDOVbNk4Vpa1poYh1_hN9xq8tLEjDF9bb3k" height=324px; width=476px;>**

**### Localizing your app title**

**If your application or extension is localalized as per the guidelines in[ http://code.google.com/chrome/extensions/i18n.html](http://code.google.com/chrome/extensions/i18n.html) you will find it possible to also localize the title of your intent in the picker using the exact same infrastructure.**
**"intents": {**
**"http://webintents.org/edit" : \[{**
**"title" : "__MSG_intent_title__",**
**"type" : \["image/\*"\],**
**"href" : "/index.html",**
**"disposition" : "inline"**
**}\]**
**}**

**## Invoking Intents**

**### Invoking an action**

**If your application needs to be able to use the functionality of another application it can simply ask the browser for it. For example to ask for an application that supports image editing it is a as simple as:**
**var intent = new WebKitIntent("http://webintents.org/edit", "image/png", "dataUri://");**
**var onSuccess = function(data) { /\* process remote data \*/ };**
**var onFailure = function() { /\* handle the error \*/ };**
**window.navigator.webkitStartActivity(intent, onSuccess, onFailure);**

**### Handling data returned to the calling application**

**A lot of applications want to cooperate with the app that invoked them. It is easy to send data back to the calling client by using \`intent.postResult\`:**
**var intent = new WebKitIntent("http://webintents.org/edit", "image/png", "dataUri://");**
**var onSuccess = function(data) {**
**// Load the data that was edited into an image.**
**if(data instanceof Blob) {**
**img.src = window.URL.createObjectURL(data)**
**}**
**else {**
**img.src = data;**
**}**
**};**
**var onFailure = function() { /\* handle the error \*/ };**
**window.navigator.webkitStartActivity(intent, onSuccess, onFailure);**

**### Handling Errors and Exceptions**

**If a service application has signalled to the client application that an un-recoverable error has occurred then your onError callback will be invoked .**
**var intent = new WebKitIntent("http://webintents.org/edit", "image/png", "dataUri://");**
**var onSuccess = function(data) {};**
**var onFailure = function() {**
**console.log(“an error has occurred with: ” + intent.action);**
**// Update the UI to ask the user to re-choose their action.**
**};**
**window.navigator.webkitStartActivity(intent, onSuccess, onFailure);**

**## Handling Intents (Service)**

**### Reading the data sent in to your application**

**Once you have registered your application to be able to handle intents, you will find that the window.intent object is populated when your application loads. The specific data that is populated will depend on the calling conventions for the intent you are using. You can find out more about the calling conventions for various intents at <http://webintents.org>.**
**document.addEventListener("load", function() {**
**if(!!window.webkitIntent) {**
**var action = window.webkitIntent.action;**
**var data = window.webkitIntent.data;**
**var type = window.webkitIntent.type;**
**// Do something magical.**
**}**
**}, false);**

**### Returning data to the calling application**

**A lot of applications want to cooperate with the app that invoked them. It is easy to send data back to the calling client by using \`intent.postResult\`:**
**document.addEventListener("load", function() {**
**if(!!window.webkitIntent) {**
**var data = window.webkitIntent.data;**
**// Send the same data back to the client.**
**window.webkitIntent.postResult(data);**
**}**
**}, false);**

**### Handling Errors and Exceptions**

**If your service application needs to signal to the client application that an un-recoverable error has occurred then your application will need to call \`postFailure\` on the intent object. This will signal to the client’s onError callback that something has gone wrong.**
**document.addEventListener("load", function() {**
**if(!!window.webkitIntent) {**
**// Something has gone Wrong! Abort!**
**window.webkitIntent.postFailure();**
**}**
**}, false);**

**# Code Examples of Common Actions**

**## Share a link**

**### Client**

**var intent = new WebKitIntent("http://webintents.org/share", "text/uri-list", location.href);**
**var onSuccess = function(data) { /\* woot \*/ };**
**var onError = function(data) { /\* boooo \*/ };**
**window.navigator.webkitStartActivity(intent, onSuccess, onError);**

**### Service**

**document.addEventListener("load", function() {**
**if(!!window.webkitIntent) {**
**var data = window.webkitIntent.data;**
**// Add the url to the UI**
**window.webkitIntent.postResult(data);**
**}**
**}, false);**

**## Share an Image link**

**### Client**

**var imageUrl = document.getElementById("main-image").src;**
**var intent = new WebKitIntent("http://webintents.org/share", "image/\*", imageUrl);**
**var onSuccess = function(data) { /\* woot \*/ };**
**var onError = function(data) { /\* boooo \*/ };**
**window.navigator.webkitStartActivity(intent, onSuccess, onError);**

**### Service**

**document.addEventListener("load", function() {**
**if(!!window.webkitIntent) {**
**var data = window.webkitIntent.data;**
**// Add the url to the UI, or proxy the request so we can load it in a canvas**
**window.webkitIntent.postResult(data);**
**}**
**}, false);**

**## Subscribe to an RSS feed**

**### Client**

**var intent = new WebKitIntent("http://webintents.org/subscribe", "application/rss+xml", "http://paul.kinlan.me/rss.xml");**
**var onSuccess = function(data) { /\* woot \*/ };**
**var onError = function(data) { /\* boooo \*/ };**
**window.navigator.webkitStartActivity(intent, onSuccess, onError);**

**### Service**

**document.addEventListener("load", function() {**
**if(!!window.webkitIntent) {**
**var data = window.webkitIntent.data;**
**// Add the url to the UI**
**window.webkitIntent.postResult(data);**
**}**
**}, false);**

**# Examples**

**To show you Web Intents in action we have build a series of demos. These demos show you how client applications can invoke intents and how service applications can be opened and receive data.**
**To run these examples you currently need Chrome M19 and an internet connection to visit the live client apps.**
The code for all of these examples can be found on our [Github project
page](http://github.com/PaulKinlan/webintents) including:
[Extensions](https://github.com/PaulKinlan/WebIntents/tree/master/tools/chrome/extensions),
[Hosted
Apps](https://github.com/PaulKinlan/WebIntents/tree/master/tools/chrome/apps/hosted)
and [Packaged
Apps](https://github.com/PaulKinlan/WebIntents/tree/master/tools/chrome/apps/packaged)
that we have put in the Chrome Web Store.

## Client Apps

Clients are applications that fire intents to ask for additional functionality.
Client applications don’t need to be installed.

*   Imagemator - <http://www.imagemator.com> - An image work bench
*   Mememator - <http://www.mememator.com> - Add funny captions to
            interesting pictures
*   Inspirationmator - <http://www.inspirationmator.com> - Add
            motivational quotes to pictures
*   QuickSnapr - <http://www.quicksnapr.com> - Take a picture of
            yourself

## Services

Services are applications that accept intents and provide additional
functionality. You shouldn’t need to install any of these directly as you will
be asked to choose one the first time your client application fires an intent.

### Picking

The following example lets you pick files from your cloud storage solutions

*   **CloudFilePicker <http://www.cloudfilepicker.com>**

### Sharing

The following examples let you share links to many popular social networks and
email clients.

*   **Gmail
            <https://chrome.google.com/webstore/detail/hekfknefmaehloiloponcondjheopola>**
*   **Blogger
            <https://chrome.google.com/webstore/detail/lcmfhgcdahdihmmccinakboobnkijkng>**
*   **Twitter
            <https://chrome.google.com/webstore/detail/hgkghfefpdaflmkeelgkkpbpljggidoh>**
*   **LinkedIn
            <https://chrome.google.com/webstore/detail/jilmplpacdgkpfchbgiojhgmbgohcnaj>**
*   **Digg
            <https://chrome.google.com/webstore/detail/ffhijmiaebdachkmgflbbmlceejnhbkn>**
*   **Delicious
            <https://chrome.google.com/webstore/detail/cdhmgngofcgdannheihgihdhlnogjfpc>**

### Saving

The following examples let you save links, pages and images to cloud storage
solutions.

*   **Instapaper
            <https://chrome.google.com/webstore/detail/nlaaimjmkjiggpbplaeolciiopmohoag>**
*   **Read It Later
            <https://chrome.google.com/webstore/detail/lcmbpenbacgfpknjpibidbmcejekigke>**
*   **Box.net
            <https://chrome.google.com/webstore/detail/kabfhdbpgilinjkhfefkjhiijmambpim>**

### Shortening

The following examples let you shorten links using popular URL shortening API’s.

*   **Goo.gl
            <https://chrome.google.com/webstore/detail/ngkcdmekigeplbgofmgljdjccdpmhikp>**

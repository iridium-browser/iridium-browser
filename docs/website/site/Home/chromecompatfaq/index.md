---
breadcrumbs:
- - /Home
  - Chromium
page_name: chromecompatfaq
title: Fixing Google Chrome compatibility bugs in websites - FAQ
---

## NOTE: The information on this page is out of date.

## This page mostly contains information from 2011. Since then, Chrome has switched its rendering engine to [Blink](/blink) (a fork of WebKit).

## Introduction

# This document provides a concise list of common compatibility issues with Google Chrome along with their solutions. It's aimed at Web developers trying to fix compatibility issues with Google Chrome or interested in a list of things to avoid when authoring Websites to use in Google Chrome.

The list is based on analysis of a large number of real-world sites with
compatibility issues. It's important to note that in nearly all cases we've
seen, the fixes required to get a Website working well in Google Chrome have
been minimal. Developers are often surprised that problems that looked like they
could take weeks of developer time were often solved in under an hour and
matched closely with the list below.

Each item is described along with its solution, at the end we have a section
that lists useful tools and points of reference that you may find useful in
diagnosing problems.

## Preamble - Google Chrome's rendering Engine:

Google Chrome uses WebKit (<http://webkit.org/>) to draw Web pages. WebKit is a
mature (~9 years) open source layout engine used by Apple (Safari, iPhone),
Google (Android, Google Chrome), Nokia and many other companies. Google Chrome
aims to render sites exactly like Safari. This means that if your site works in
Safari there is a large chance it will work in Google Chrome and vice versa.

# Common Issues:

*   [UserAgent
            Detection](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#UserAgent_Detection)
*   [Paragraphs Overflowing/Text
            Cutoff](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#Paragraphs_Overflowing/Text_Cutoff)
*   [Correct page
            encoding](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#Correct_page_encoding)
*   [Correct Plugin
            Tags](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#Correct_Plugin_Tags)
*   [Use of Browser-Specific CSS or JavaScript
            objects](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#Use_of_Browser-Specific_CSS_or_JavaScript_objects)
*   [Useful
            Tools](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#Useful_Tools)
*   [Additional
            Resources](http://code.google.com/p/doctype/wiki/ArticleGoogleChromeCompatFAQ#Additional_Resources)

# UserAgent Detection

### The Symptom

Page not displayed correctly in Google Chrome, or you get a message noting that
Google Chrome is not a "supported browser".

### The problem

By far the most common problem we see is JavaScript (or server-side) code that
tries to detect the browser by looking at the navigator.userAgent string. Often
the checks used are buggy and do not identify Google Chrome correctly.

### Recommendations:

*   Do everything you can to avoid this, parsing navigator.userAgent and
            navigator.vendor is [notoriously
            bug-prone](http://my.opera.com/hallvors/blog/2008/12/19/10-is-the-one)
            and usually the wrong thing to do! [Object
            detection](http://www.quirksmode.org/js/support.html) is a much
            safer method to achieve per-browser behavior...

*   On Windows, Google Chrome's useragent string looks something like
            the following:
    Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/525.13 (KHTML, like Gecko) Chrome/2.0.167.0 Safari/525.13

In nearly all cases you don't want to check if you're running under Google
Chrome, but if the browser is using the WebKit rendering engine (see above). If
you must look at the navigator.userAgent string look for the substring
'AppleWebKit', nothing else is guaranteed to continue working in the future!!

var isWebKit =
navigator.userAgent.indexOf("AppleWebKit") &gt; -1;

isWebKit will be true if you're running in Google Chrome, Safari or any other
browser using WebKit.

To check the version of WebKit, use:

var WebKitVersion =
parseFloat(navigator.userAgent.split("AppleWebKit/")\[1\]) ||
undefined;
if (WebKitVersion && WebKitVersion &gt; 500 ) {
// use spiffy WebKit feature here
}

You can find a list of Google Chrome Releases and their corresponding WebKit
revisions [here](/developers/webkit-version-table).

*   Avoid code like the following:

if (isChrome) {
doSomethingChromeSpecific();
} else {
// Didn't detect browser type, so assume IE.
doSomethingIESpecific();
}

The problem is that the above snippet assumes that any browser not explicitly
identified is IE. The problem is that it's far more likely for other browsers to
act alike than it is for them to act like IE. And IE9 is very similar to all
other browsers, so the code fork is bound to fail. Stick to feature detection
instead.

# Paragraphs Overflowing/Text Cutoff

### The Symptom:

A single line header wraps over multiple lines, messing up a site's layout. Text
gets cut off or overlaps other elements.

### The problem:

HTML & CSS can't do pixel perfect layout. So font and element sizes can change
slightly between browser versions and OSs. If a site depends on a font being an
exact size then text can get cut off or wrap on other browsers or OSs.

### Recommendations:

Whenever possible, make use of dynamically sized elements rather than specifying
fixed pixel widths. This is often easier said than done, but it ensures that
content will adapt well to all browsers. Test your site in multiple browsers and
OSs, enlarge fixed pixel width elements to accommodate the maximum size you see.
Use the white-space:nowrap css attribute to ensure that single line headings
don't wrap over multiple lines.

# Correct page encoding

### The Symptom:

Your page looks garbled in Google Chrome. Garbage characters may be displayed,
and RTL language pages (e.g. Hebrew or Arabic) may appear with their letters
reversed.

### The problem:

If character encoding is not specified precisely, different browsers can
interpret the encoding in different ways or not at all. The impact on users is
dire since it prevents them from viewing the site.

### Recommendations:

*   Declare your page's content-type correctly, this can either be in an
            [HTTP
            header](http://www.w3.org/International/tutorials/tutorial-char-enc/#Slide0270)
            or a [Meta tag specified in your
            HTML](http://code.google.com/p/doctype/wiki/MetaCharsetAttribute).
*   The character set your page uses must be a legal value from the
            [Official IANA
            List](http://www.iana.org/assignments/character-sets), please only
            use the encodings that have the text (preferred MIME name) listed
            next to them e.g. ISO-8859-1, Shift_JIS.
*   If you specify two different values for the character encoding in
            the HTTP Header and the Meta tag, Google Chrome will use the value
            in the HTTP Header. Conflicting declarations of character encoding
            in the HTTP Header and Meta tag is asking for trouble. More
            information on this subject can be [found
            here](http://blog.whatwg.org/the-road-to-html-5-character-encoding).
*   We recommend using UTF-8 for all Web content. If you have to use
            legacy encoding for some reason, make sure to identify the encoding
            correctly as outlined above. For legacy situations involving Hebrew
            sites use [Logical Hebrew encoding
            (ISO-8859-8-I)](http://www.w3.org/International/geo/html-tech/tech-bidi.html#ri20030112.21380914).
            We strongly **discourage** the use of Visual Hebrew encoding
            (ISO-8859-8). It has no place on the Web anymore and is a remnant of
            old systems lacking logic for rendering RTL text. It causes many
            bugs and lots of confusion.

# Correct Plugin Tags

### The Symptom:

Plug-ins, such as Flash videos, Windows Media Player movies, or Java applets, do
not appear in Google Chrome, but do appear in Internet Explorer.

### The problem:

There are 2 types of plugins on Windows: ActiveX & NPAPI. IE uses ActiveX
plugins, all other browsers (including Google Chrome) use NPAPI plugins. ActiveX
is Windows-only, plugins on other platforms usually use NPAPI.

### Recommendations:

1.  Do not use plug-ins for which only an Active-X version exists, they
            won't work in Google Chrome, Firefox, Safari, Opera or any browser
            not using the IE rendering engine.
2.  Be sure that parameters in your &lt;object&gt; and &lt;embed&gt;
            tags are the same. A common problem is changing the parameter in
            only one of the tags, for example:

&lt;object ...&gt;
&lt;param name="src" value="flash_ad.swf"&gt;
&lt;embed src="different_file.swf" ...&gt;
&lt;/object&gt;

This embeds a flash video. IE will use the parameters in the object tag and thus
will load the file **flash_ad.swf**. All other browsers will use the embed tag
and play **different_file.swf**. Another common error is to specify different
values for the transparency attribute when embedding flash.

# Use of Browser-Specific CSS or JavaScript objects

### The Symptom:

Some CSS styling does not work in Google Chrome, even though they seem fine in
IE or Firefox.

### The problem:

Each browser has its own private CSS selectors and JavaScript objects. Use of
these types of markup is, by definition, not compatible with other browsers.
These should only be used for non-critical tasks (e.g. adding text shadows). It
is safest not to use them at all.

### Recommendations:

*   Do not use
            [document.all](http://simonwillison.net/2003/Aug/11/documentAll/) in
            JavaScript. This is an outdated IE feature, all modern browsers
            support
            [document.getElementById()](https://developer.mozilla.org/En/Document.getElementById)
            and you should [use that
            instead](http://www.w3schools.com/HTMLDOM/dom_nodes_access.asp).
*   When diagnosing JavaScript issues, use Google Chrome's [built-in
            JavaScript
            debugger](http://www.google.com/chrome/intl/en/webmasters-faq.html#jsexec).
*   Do not use browser-specific (e.g. -moz-\*, -webkit-\*, -ie-\*) css
            selectors such as -moz-center or -webkit-highlight for critical
            visual features of your site, instead use standard CSS.
*   Do not use [CSS
            expressions](http://msdn.microsoft.com/en-us/library/ms537634%28VS.85%29.aspx)
            (e.g. width:expression()). These only work in IE, [have serious
            performance issues and have been
            deprecated](http://blogs.msdn.com/ie/archive/2008/10/16/ending-expressions.aspx).

# Useful Tools

We've found the following tools extremely useful when diagnosing compatibility
problems with Websites. Using them can greatly decrease the amount of effort and
guesswork that goes into fixing compatibility issues:

1.  Google Chrome has a [variety of built-in
            tools](http://www.google.com/support/chrome/bin/answer.py?hl=en&answer=95691)
            to help developers track down compatibility and performance issues.
2.  [Firebug](http://getfirebug.com/) - An excellent Firefox extension
            that can help examining markup, JavaScript and performance issues.
3.  [Fiddler](http://www.fiddler2.com/) - A free Windows-only tool that
            allows you to examine and replay HTTP requests and responses.

# Additional Resources

1.  [Google Chrome Webmaster
            FAQ](http://www.google.com/chrome/intl/en/webmasters-faq.html) -
            Contains all the information in this document and loads more.
2.  [quirksmode](http://www.quirksmode.org/) - Useful Information about
            which features are implemented in which browser.
3.  [Reporting a bug in Google
            Chrome](/for-testers/bug-reporting-guidelines) - guidelines and
            pointers on how to write an effective clear bug report.

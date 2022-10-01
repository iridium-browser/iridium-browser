---
breadcrumbs: []
page_name: searchbox
title: SearchBox API
---

**OBSOLETE** - replaced by the [Embedded Search API](/embeddedsearch).In
addition to [OpenSearch](http://www.opensearch.org/) compliance, Chromium also
provides an experimental JavaScript API that enables the default search provider
to interact with the [Omnibox](/user-experience/omnibox). This allows the search
page to instantly respond to user interaction without reloading the page. It
also allows the search page to communicate autocompletion suggestions. This page
provides the technical specification for the SearchBox API. **Status of this
document:** Editor's Draft March 19, 2011.This is a **work in progress** and may
change without any notices. This API has been [proposed to the
WHATWG](http://lists.whatwg.org/htdig.cgi/whatwg-whatwg.org/2010-October/028818.html)
and we are eager to formally standardize it as it stabilizes. Please send
comments to tonyg at chromium dot org.

### 1. WebIDL interfaces

**Note** that this entire interface is only available to the page the user has
selected as the default search provider. The searchBox object is not exposed to
general pages. See section 1.2.1 for details.

#### 1.1 SearchBox interface

<pre><code>interface <b>SearchBox</b> { readonly attribute DOMString value; readonly attribute boolean verbatim; readonly attribute unsigned long selectionStart; readonly attribute unsigned long selectionEnd; readonly attribute unsigned long x; readonly attribute unsigned long y; readonly attribute unsigned long width; readonly attribute unsigned long height; void setSuggestions(in Object suggestions);          attribute Function onchange;          attribute Function onsubmit;          attribute Function oncancel;          attribute Function onresize;};
</code></pre>

#### 1.1.1 Attributes

These attributes describe the state of the user agent's search box to the search
page. All attributes must be "live" and valid throughout the lifetime of the
page.

**value** of type DOMString

> This attribute must return the text that has been entered into the search box.

**verbatim** of type boolean

> This attribute must return true if and only if the user agent suggests that
> the search page treat value as a verbatim search query (as opposed to a
> partially typed search query). Otherwise this attribute must return false. It
> is recommended that the user agent return true in the following circumstances:

> *   The user presses the forward delete key at the end of an input.
> *   The user highlights an option in the search box's dropdown and
              value is updated to reflect the option.
> *   The user agent in any way overridden the value entered by the
              user.

**selectionStart** of type unsigned long

> This attribute must return the offset (in logical order) to the character that
> immediately follows the start of the selection in the search box. If there is
> no selection, then it must return the offset (in logical order) to the
> character that immediately follows the text entry cursor. *Note that in the
> case there is no explicit text entry cursor, the cursor is implicitly at the
> end of the input.*

**selectionEnd** of type unsigned long

> This attribute must return the offset (in logical order) to the character that
> immediately follows the end of the selection in the search box. If there is no
> selection, then it must return the offset (in logical order) to the character
> that immediately follows the text entry cursor. *Note that in the case there
> is no explicit text entry cursor, the cursor is implicitly at the end of the
> input.*

**x** of type unsigned long

> This attribute must return the horizontal offset (in pixels relative to the
> [window](http://dev.w3.org/html5/spec/Overview.html#window)'s origin) of the
> portion of the search box that overlaps with the
> [window](http://dev.w3.org/html5/spec/Overview.html#window) (i.e. the search
> box's drop down menu). If the search box does not overlap the
> [window](http://dev.w3.org/html5/spec/Overview.html#window), this attribute
> must return zero.

**y** of type unsigned long

> This attribute must return the vertical offset (in pixels relative to the
> [window](http://dev.w3.org/html5/spec/Overview.html#window)'s origin) of the
> portion of the search box that overlaps with the
> [window](http://dev.w3.org/html5/spec/Overview.html#window) (i.e. the search
> box's drop down menu). If the search box does not overlap the
> [window](http://dev.w3.org/html5/spec/Overview.html#window), this attribute
> must return zero.

**width** of type unsigned long

> This attribute must return the horizontal width in pixels of the portion of
> the search box that overlaps with the
> [window](http://dev.w3.org/html5/spec/Overview.html#window) (i.e. the search
> box's drop down menu). If the search box does not overlap the
> [window](http://dev.w3.org/html5/spec/Overview.html#window), this attribute
> must return zero.

**height** of type unsigned long

> This attribute must return the vertical height in pixels of the portion of the
> search box that overlaps with the
> [window](http://dev.w3.org/html5/spec/Overview.html#window) (i.e. the search
> box's drop down menu). If the search box does not overlap the
> [window](http://dev.w3.org/html5/spec/Overview.html#window), this attribute
> must return zero.

**1.2.2 Methods**

This method allows the search page to control the state of the user agent's
search box.

**setSuggestions**

> This method accepts one argument, an object in [JavaScript Object
> Notation](http://www.json.org/) form. It must adhere to the following JSON
> specification.

> { suggestions: \[ { value: "..." }, { value: "..." } \] }

> **suggestions** of type array - An ordered array of suggestions to be
> displayed. *Note that calling this method with an empty suggestions array
> effectively clears search-page-provided-suggestions.*

> **value** of type string - The string completion for this suggestion.

> The user agent must process suggestions as follows:

> 1.  Let search-page-provided-suggestions be an initially empty array
              of strings.
> 2.  Upon invoking setSuggestions, atomically replace all items in
              search-page-provided-suggestions with the value of each object in
              the suggestions array.

> If the first search-page-provided-suggestions starts with the current search
> box value, it is recommended that the user agent display it as an inline
> autocomplete option. It is recommended that the user agent display the
> remaining N search-page-provided-suggestions as autocomplete options in a drop
> down menu below the search box input.

**1.2.3 Event handlers**

These event handlers allow the search page to register to receive notifications
of changes in the search box attributes.

**onchange**

> If the value of this attribute is a Function, that function must be called
> when the value of any of the value, verbatim, selectionStart or selectionEnd
> attributes is changed.

**onsubmit**

> If the value of this attribute is a Function, that function must be called
> when the user commits the search query. It is recommended that the user agent
> consider the search query committed when the user presses enter/return.

**oncancel**

> If the value of this attribute is a Function, that function must be called
> when the user cancels the search query. It is recommended that the user agent
> consider the search query cancelled when the search box loses focus.

**onresize**

> If the value of this attribute is a Function, that function must be called
> when the value of any of the x, y, width, or height attributes is changed.

#### 1.2 Navigator interface

NOTE: Because the SearchBox interface is still under development, the Chromium
implementation exposes it under window.chrome instead of window.navigator. Some
form of vendor prefixing is typical before new interfaces are standardized.

<pre><code>[Supplemental]interface <b>Navigator</b> { readonly attribute SearchBox searchBox;};
</code></pre>

**1.2.1 Attributes** **searchBox**
> **Upon creation of a new [browsing
> context](http://dev.w3.org/html5/spec/browsers.html#browsing-context), if the
> location indicates that the page is the user's default search provider, a new
> SearchBox instance must be created.** If there is a SearchBox instance for the
> current browser context (the page is the default search provider), this
> attribute must return the current SearchBox instance. Otherwise this attribute
> must not be exposed to the DOM.

### 2. Example usage

This trivial example demonstrates how a default search provider may obtain a
reference to the SearchBox instance, register for events, read state and set
state.

```none
var searchBox = window.navigator.searchBox;searchBox.onchange = function() {  if (this.selectionStart == this.selectionEnd &&      this.selectionStart == this.value.length)    alert('Cursor is at end of input');  alert('Setting suggestions for: ' + this.value);  this.setSuggestions({    suggestions: [      { value: "one"      },      { value: "two"      }    ]  });}searchBox.onsubmit = function() {  alert('User searched for: ' + this.value);}searchBox.oncancel = function() {  alert('Query when user cancelled: ' + this.value);}searchBox.onresize = function() {  alert('Resized to: ' +        [this.x,         this.y,         this.width,         this.height].join(','));}
```

### 3. Change log

<table>
<tr>
<td> <b>Date</b></td>
<td><b> Description</b></td>
</tr>
<tr>
<td> Jan 4, 2011</td>
<td> Change setSuggestions from array of strings to JSON.</td>
</tr>
<tr>
<td> Jan 7, 2011</td>
<td> Remove "query" from setSuggestions JSON. Update processing model for setSuggestions.</td>
</tr>
<tr>
<td> March 19, 2011</td>
<td> Change chrome.searchBox to navigator.searchBox.</td>
</tr>
</table>

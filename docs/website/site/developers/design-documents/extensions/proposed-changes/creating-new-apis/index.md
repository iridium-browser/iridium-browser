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
page_name: creating-new-apis
title: Implementing a new extension API
---

**Proposal**

See [API Proposals (New APIs Start
Here)](/developers/design-documents/extensions/proposed-changes/apis-under-development).

So you want to add the Widgets API. Let's call it **widgets**.

**Defining the Interface**

#### How will extensions declare their intent to use widgets?

### You need to decide this now. In other words, what will a user of widgets need to write in their manifest?

Typically this will be either via a **permission string** or **manifest entry**.
There is no need for both. By convention it should be called "widgets".

* Use a **manifest entry** for declarative style APIs, or when you
            need to express some sort of rich structure. For example,
            [commands](http://developer.chrome.com/extensions/commands.html).

```
{
  "name": "Demo widget extension",
  "widgets": {
    "foo": "bar",
    "baz": "qux"
  }
  ...
}
```

* Use a **permission string** for procedural APIs, typically those
            which are just a collection of functions. Most APIs are of this
            form. For example,
            [storage](http://developer.chrome.com/extensions/storage.html).

```
{
  "name": "Demo widget extension",
  "permissions": [..., "widgets", ...]
  ...
}
```

There are exceptions:

*   Some APIs are integral parts of the platform (e.g.
            [runtime](http://developer.chrome.com/apps/runtime.html),
            [app.window](http://developer.chrome.com/apps/app.window.html), etc)
            and require no declaration.
*   Some older APIs use object permissions rather than strings, such as
            [fileSystem](http://developer.chrome.com/apps/fileSystem.html). This
            is deprecated. Use a manifest entry instead.

#### Tell the extensions platform about widgets

Firstly decide, *can your API be applied to any platform built on the web, or
does it only make sense for Chrome?* Examples of the former: storage, messaging.
Examples of the latter: browserAction, bookmarks. A good clue is whether you
need to #include anything from chrome.

*   If it applies to any platform, widgets should be implemented in the
            extensions module (src/extensions).
*   If it's Chrome only, widgets should be implemented in Chrome
            (src/chrome/common/extensions and src/chrome/browser/extensions,
            etc).
*   If in doubt, try to implement it in src/extensions.

From here, all files here are relative to either extensions/common/api or
chrome/common/extensions/api:

First, add an entry in _api_features.json. This tells the extensions platform
about when your API should be available (anywhere? only in extension
processes?), and what they need to do to use it (do they need a permission? a
manifest entry?).

*   If you're using a **manifest entry**, use "manifest:widgets" as the
            dependency.
*   If you're using a **permission string**, use "permission:widgets" as
            the dependency.

Second, add an entry to either _manifest_feature.json or
_permission_features.json. This tells the platform how to interpret "widgets"
when it encounters it as either a manifest entry or a permission. What is it
available to (extensions? apps? both?), and importantly what channel is it
available in (dev? beta? stable?). **New extension APIs MUST start in dev**
(although if they're unimplemented then trunk is advisable).

*==New extension APIs MUST start in dev==* (just repeating it).

**Write a schema for widgets**

Extension APIs can be defined in either IDL (widgets.idl) or JSON Schema
(widgets.json). IDL is much more concise, but doesn't include some of the
advanced features supported by JSON Schema.

You probably want IDL, though be warned *IDL syntax errors occasionally cause
the compiler to never terminate*.

Fourth, list the schema in
[//extensions/common/api/schemas.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:extensions/common/api/schema.gni),
which tells the build system to generate a bunch of boilerplate for you in
&lt;build_dir&gt;/gen/extensions/common/api or
&lt;build_dir&gt;/gen/chrome/common/extensions/api: models for your API, and the
glue to hook into your implementation.

Finally, add some documentation:

## **Adding documentation**

Adding documentation is very simple:

1.  Write a summary for your API and put it in
            chrome/common/extensions/docs/templates/intros/myapi.html.
2.  Create the publicly accessible template(s) in
            chrome/common/extensions/docs/templates/public/{extension,apps}/myapi.html.
            Whichever of extensions and/or apps you add an HTML file in depends
            on which platform can access your API.
    *   Each will look something like:
                {{ '{{' }}+partials.standard_extensions_api api:apis.extensions.myapi
                intro:intros.myapi /{{ '}}' }} (or apps).

**C++ implementation**

The actual C++ implementation will typically live in
extensions/browser/api/myapi or chrome/browser/extensions/api/myapi (as
mentioned above, the magic glue is generated for you).

**Functions**

Extension APIs are implemented as subclasses of ExtensionFunction from
extensions/browser/extension_function.h.

*   Use DECLARE_EXTENSION_FUNCTION to declare the JavaScript identifier
            for the function.
*   Implement ExtensionFunction::Run. The comments on that method
            explain how to implement it.
*   Note: use EXTENSION_FUNCTION_VALIDATE to validate input arguments,
            which are placed in args_. Failing this check kills the renderer, so
            the idiom for this is catching bugs in the renderer (for example:
            schema validation errors).
*   Add your implementation files to
            [chrome/common/extensions/api/api_sources.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:chrome/common/extensions/api/api_sources.gni).

**Model generation**

Your C++ implementation **must live** in
extensions/browser/api/myapi/myapi_api.h/cc or
chrome/browser/extensions/api/myapi/myapi_api.h/cc (depending on where it was
declared).This is so that the code generator can find the header file defining
your extension function implementations. Remember to add your source files to
[//chrome/common/extensions/api/api_sources.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:chrome/common/extensions/api/api_sources.gni).

In your header file, include extensions/common/api/myapi.h or
chrome/common/extensions/api/myapi.h to use the generated model. This comes from
a code-generated file that lives under e.g.
out/Debug/gen/chrome/common/extensions/api. Let's say we have the following IDL
(or equivalent JSON schema):

```
// High-level description of your API. This will appear in various places in the
// docs.
namespace myapi {
  dictionary BazOptions {
    // Describes what the id argument means.
    long id;

    // Describes what the s argument means.
    DOMString s;
  };

  dictionary BazResult {
    long x;
    long y;
  };

  callback BazCallback = void (BazResult result);

  interface Functions {
    // An interesting comment describing what the baz operation is.
    // Note that this function can take multiple input arguments, including
    // things like long and DOMString, but they have been elided for simplicity.
    static void doBaz(BazOptions options, BazCallback callback);
  };
};
```

A simple C++ implementation might look like this:
```
namespace extensions {

// You must follow a naming convention which is ApiNameFunctionNameFunction,
// in this case MyapiDoBazFunction. This is so that the generated code
// can find your implementation.
class MyapiDoBazFunction : public AsyncExtensionFunction {
 public:
  virtual ~MyapiDoBazFunction () {}

 private:
  // The MYAPI_DOBAZ entry is an enum you add right before ENUM_BOUNDARY
  // in chrome/browser/extensions/extension_function_histogram_value.h
  DECLARE_EXTENSION_FUNCTION("myapi.doBaz", MYAPI_DOBAZ);

  virtual ResponseAction Run() OVERRIDE {
    // Args are passed in via the args_ member as a base::ListValue.
    // Use the convenience member of the glue class to easily parse it.
    std::unique_ptr<api::myapi::DoBaz::Params> params(
      api::myapi::DoBaz::Params::Create(*args_));
    EXTENSION_FUNCTION_VALIDATE(params.get());
    api::myapi::BazResult result;
    result.x = params->options.id;
    base::StringToInt(params->options.s, &result.y);

    // Responds to the caller right, but see comments on
    // ExtensionFunction::Run() for other ways to respond to messages.
    return RespondNow(ArgumentList(result.ToValue()));
  }
};

} // namespace extensions
```
ExtensionFunction is refcounted and instantiated once per call to that extension
function, so use base::Bind(this) to ensure it's kept alive (or use
AddRef...Release if that's not possible for some reason).

**Events**

Use ExtensionEventRouter (on the UI thread) to dispatch events to extensions.
Prefer the versions that allow you to pass in base::Value rather than a JSON
serialized format. Event names are auto-generated in the API file (e.g.
chrome/common/extensions/api/myapi.h). In the un-common case where an event is
not defined in IDL or json, the corresponding event name should be defined in
chrome/browser/extensions/event_names.h.

As with extension functions, it generates some C++ glue classes. Let's say we
have the following IDL (or equivalent JSON Schema):

```
namespace myapi {
  dictionary Foo {
    // This comment should describe what the id parameter is for.
    long id;

    // This comment should describe what the bar parameter is for.
    DOMString bar;
  };

  interface Events {
    // Fired when something interesting has happened.
    // `foo`: The details of the interesting event.
    static void onInterestingEvent(Foo foo);
  };
};
```
To use the generated glue in C++:
```
DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
api::myapi::Foo foo;
foo.id = 5;
foo.bar = "hello world";
ExtensionSystem::Get(profile)->event_router()->DispatchEventToExtension(
  extension_id,
  api::myapi::OnInterestingEvent::kEventName,
  api::myapi::OnInterestingEvent::Create(foo),
  profile,
  GURL());
```

**Permissions**

By default, extension APIs should require a permission named the same as the API
namespace.

New permissions are added in ExtensionAPIPermissions::GetAllPermissions() in
extensions/common/permissions/extensions_api_permissions.cc or in
ChromeAPIPermissions::GetAllPermissions() in
chrome/common/extensions/permissions/chrome_api_permissions.cc. You may also
need to modify api_permission.h and chrome_permission_message_rules.cc in those
directories; see how it's done for other permissions.

**Advanced Extension Functionality**

**Custom Bindings**

Custom JS bindings go in chrome/renderer/resources/extensions/\*.js.

These are necessary for doing anything special: synchronous API functions,
functions bound to types, anything renderer-side. (If it's a common enough
operation, please send us patches to do it automatically as part of the bindings
generation :).

**New Manifest Sections**

If your API requires a new manifest section:

1.  Add a schema for the manifest entry to
            chrome/common/extensions/api/manifest_types.json (e.g. Widgets).
    *   This is the JSON Schema format. See "Write a schema for
                widgets".
2.  Add the name of the manifest entry to
            chrome/common/extensions/api/_manifest_features.json and declare
            when it's available.
    *   See "Tell the extensions platform about widgets".
3.  Add a ManifestHandler implementation in
            chrome/common/extensions/manifest_handlers (e.g.
            chrome/common/extensions/widgets_handler.cc/h).
    *   Your implementation can use the model that has been generated
                from the schema in manifest_types.json
    *   And of course, test it.
4.  If your manifest section requires permission messages and/or custom
            permissions, add a ManifestPermission implementation. See
            "sockets_manifest_handler" for an example.

The code which handles the externally_connectable manifest key is a good place
to start.

**Testing Your Implementation**

Make sure it has tests. Like all of Chrome, we prefer unit tests to integration
tests.

*   There is a relatively new mini framework for unit tests in
            chrome/browser/extensions/extension_function_test_utils.h. Hopefully
            it meets your needs.
*   If not, there is the older [API
            tests](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/test/data/extensions/api_test/README.txt?view=markup)
            for integration tests.

**Iterating**

1.  Create an example extension that uses your API and add it to
            chrome/common/extensions/docs/examples
2.  Ask someone else (preferably someone in chrome-devrel@ or
            chrome-extensions-team@) to make a second example extension
3.  Iterate based on how those examples went
4.  Make sure that your API is functional on all of Chrome's platforms
            that have access to the web store and that all tests are enabled on
            all platforms
5.  Announce the dev channel API to the community by sending mail to
            chromium-extensions@chromium.org and a blog post if appropriate
6.  Encourage community feedback (ask chrome-extensions-team@ for ideas
            here)
7.  Iterate on the API based on community feedback
8.  In general, an API should be on dev channel for at least one full
            release cycle before moving on to the next phase

### Going to Stable

Follow the [Going Live
Phase](/developers/design-documents/extensions/proposed-changes/apis-under-development)
instructions.

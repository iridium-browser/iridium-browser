---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: profile-architecture
title: Profile Architecture
---

Chromium has lots of features that hook into a `Profile`, a bundle of data
about the current user and the current chrome session that can span multiple
browser windows. When Chromium first started, the profile had only a few moving
parts: the cookie jar, the history database, the bookmark database, and things
to do with user preferences.

As more and more features were added, the `Profile` class grew bigger and
bigger. In the early stages of the Chromium Project, it owned all of the
profile's state, leading to things like `Profile::GetInstantPromoCounter()` or
`Profile::GetHostContentSettingsMap()`. At one point, there were 58 getters in
`Profile`.

We also struggled with startup and teardown issues during this period. When
creating a new `Profile` object, we had to create all the services in the right
order. For instance, the Sync service has a dependency on the history,
bookmarks, and extensions modules. If we weren't careful, we could initialize
Sync too early or destroy it too late.

The design has evolved a lot since then. In 2012, we transitioned to the new
model described on this page. Rather than owning everything directly, `Profile`
is now a sort of handle object with minimal state. To prevent startup and
teardown issues, we use a `DependencyManager` and a two-phase shutdown model.

[TOC]

## Design Goals

*   **The `Profile` class must stay at a maintainable size.** It is tempting to
    put all of a profile's "state" in the same place, but is not sustainable in
    a codebase this size.
*   **Profile startup and shutdown must be robust.** When we started and only
    had a few objects hanging off of Profile, manual ordering was acceptable for
    destruction. Now, we have hundreds of services; we can not rely on manual
    ordering when we have so many components.
*   **We must allow features to be compiled in and out.** This is important for
    any multi-platform project, but it's especially important for iOS, where we
    use WebKit instead of Blink. [Layered
    components](https://www.chromium.org/developers/design-documents/layered-components-design/#layered-components)
    offer guidelines for writing code that works on iOS and desktop at the same
    time. This design also makes it easy for third-party vendors (e.g. Opera,
    Edge, Brave...) to add/remove features as they see fit.

## How to Add a New Service

### KeyedService

A `KeyedService` is basically a plain object, with the following differences:

* It has a virtual destructor.
* Its lifetime is managed by a `ProfileKeyedServiceFactory` or a
  `BrowserContextKeyedServiceFactory` singleton.
* It has an overridable `Shutdown()` method, which is always called before the
  destructor. This allows two-phase shutdown; see below.

Let's say we're working on a new feature called Foo, and we want to store some
state in the profile. Typically, we'll implement this as a `FooService` class
that has a separate instance for each profile. It is generally a good idea to
disable copy and assignment for this class.


```
class FooService : public KeyedService {
 public:
  explicit FooService(PrefService* profile_prefs, BarService* bar_service);
  virtual ~FooService() = default;

  FooService(const FooService&) = delete;
  FooService& operator=(const FooService&) = delete;

 private:
  PrefService* profile_prefs_;
  BarService* bar_service_;
};
```

### ProfileKeyedServiceFactory

Now that we have implemented `FooService`, we need to derive from
`ProfileKeyedServiceFactory` (PKSF).

`ProfileKeyedServiceFactory` derives from `BrowserContextKeyedServiceFactory`
(BCKSF) where most of the following is implemented. PKSF adds the service
selection logic per profile type. In the rest of the document, we will refer to
PKSF as it is the main type of factory to be constructed. Differences will be
noted in the `BrowserContextKeyedServiceFactory` section below.

Instead of having the `Profile` own `FooService`, we have a dedicated singleton
`FooServiceFactory`. This class takes care of creating and destroying
`FooService` per `Profile`. Here is a minimal example:

```
class FooServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static FooService* GetForProfile(Profile* profile);
  static FooServiceFactory* GetInstance();

 private:
  FooServiceFactory();
  virtual ~FooServiceFactory();

  // BrowserContextKeyedServiceFactory:
  virtual BrowserContextKeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};
```

We have a factory which performs most of the work of associating a profile with
an object provided by the `BuildServiceInstanceFor()` method. The
`BrowserContextKeyedServiceFactory` provides an interface to override
while managing the lifetime of your Service object in response to `Profile`
lifetime events and making sure your service is shut down before services it
depends on.

An absolutely minimal factory will supply the following methods:

*   A static `GetInstance()` method that refers to the factory as a
            singleton.
*   A `GetForProfile()` method that wraps PKSF, casting the result back to
            whatever type we need to return.
*   A `BuildServiceInstanceFor()` method which is called once by the framework
            for each profile. It must initialize and return a proper instance of
            our service.

In addition, PKSF provides these other knobs for controlling behavior:

*   PKSF provides control over the creation of services for different profile
            types via the constructor.
    *   Using PKSF, you can construct the structure `ProfileSelections` to
            control which profile type the service will be created for.
    *   By default, PKSF will return nullptr (no service constructed) for all
            non-Regular Profiles, e.g Incognito profile, Guest profile and
            System profile. Note: Ash internal profiles are of type Regular and
            will behave the same by default. You can change that behavior in the
            `ProfileSelections` constructor.
    *   PKSF allows you to control which Profile type the service will be
            constructed for via the structure `ProfileSelections` passed to the
            constructor, providing a value of `ProfileSelections` will change the
            default behavior.
    *   More information on `ProfileSelections` can be found [here](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/profiles/profile_keyed_service_factory.md;l=72)
*   `RegisterProfilePrefs()` is called once per profile during initialization
            and is where you can place any user pref registration.
*   By default, PKSF will lazily create your service. If you override
            `ServiceIsCreatedWithBrowserContext()` to return true, your service
            will be created alongside the profile.
*   PKSF gives you multiple ways to control behavior during unit tests.
            See the header for more details.
*   PKSF gives you a way to augment and tweak the shutdown and deallocation
            behavior.


Examples of PKSF factory initialization (constructor definition):

```
// Initialization of a PKSF factory with default Profile selection behavior.
ChromeDefaultKeyedServiceFactory()
    : ProfileKeyedServiceFactory("DefaultKeyedService") {}

// Initialization of a PKSF factor with customized Profile selection behavior
// for different profile types with different behaviors per type.
ChromeCustomizedKeyedServiceFactory()
    : ProfileKeyedServiceFactory("CustomizedKeyedService",
        ProfileSelections::Builder()
          .WithRegular(ProfileSelection::kOwnInstance))
          .WithGuest(ProfileSelection::kOffTheRecordOnly)
          .WithAshInternals(ProfileSelection::kNone)
          .Build()) {}
```

### BrowserContextKeyedServiceFactory

In some cases, creating a PKSF is not possible, PKSF can only be created under
chrome/ directory. Other factories/services will have to derive from BCKSF,
e.g implementing a factory under components/, weblayer/ or extensions/
(extensions mainly use a templated version `BrowserContextKeyedAPIFactory<T>`).
There are some differences to note when using a BCKSF:
*  The derived constructor should associate the factory with the
            `BrowserContextDependencyManager` singleton to make the
            `DependsOn()` declarations, in PKSF this is directly done in the
            PKSF parent constructor.
*   By default, BCKSF return nullptr when given an Incognito profile.
    *   You can change this behavior by overriding `GetBrowserContextToUse()`,
                based on the input `BrowserContext`.
    *   In PKSF, this behavior is controlled by the `ProfileSelections` which
                provides a better API and default behavior.

WARNING: handling all profile types correctly in BCKSF is very difficult as
there are a lot of edge cases (OTR and non-OTR system and guest profiles,
Ash-internal profiles, ...). It is recommended to use PKSF instead if possible
to handle this complexity.

```
// Initialization of a BKCSF factory.
NonChromeDefaultKeyedServiceFactory::
NonChromeDefaultKeyedServiceFactory()
      : BrowserContextKeyedServiceFactory(
            "DefaultBrowserContextKeyedServiceFactory",
            BrowserContextDependencyManager::GetInstance()) {}

// Overriding which `context` the service will be constructed for.
content::BrowserContext*
    NonChromeDefaultKeyedServiceFactory::GetBrowserContextToUse(
        content::BrowserContext* context) const override {
          // Create the service for all profiles (contexts), including off the
          // record profiles.
      return context;
}
```

### Java keyed services

On Android, C++ keyed services often have a corresponding Java object. In such
cases, the C++ part should own the Java one. You can find more details
[here](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/android_jni_ownership_best_practices.md).


### Use the Service

Instead of doing `profile.GetFooService()` (the old way), you should access
`FooService` via `FooServiceFactory::GetForProfile(profile)`.

## A Few Types of Factories

Not all objects have the same lifecycle and memory management. The previous
paragraph was a major simplification; there is a base class
`KeyedServiceBaseFactory` that defines the most general dependency stuff
while `BrowserContextKeyedServiceFactory` is a specialization for the content
layer. `ProfileKeyedServiceFactory` is another specialization that takes care
of the Profile selection for the services. There is a second
`RefcountedProfileKeyedServiceFactory` that gives slightly different semantics
and storage for `RefCountedThreadSafe` objects. Equivalent
`RefcountedBrowserContextKeyedServiceFactory` exists.

## Two-phase Shutdown

Shutdown behavior is a bit subtle. For historical reasons, we have a two-phase
deletion process:

1.  Every PKSF will first have its `Shutdown()` method called. Use this method
    to drop weak references to the `Profile` or other service objects.
2.  Every PKSF is deleted and its destructor is run. Minimal work should be
    done here. Attempts to call any `*ServiceFactory::GetForProfile()` will
    cause an assertion in debug mode.

The `Shutdown()` steps lets us drop weak references and observers *before* any
`KeyedService` is destroyed. This is useful for "cycles" , e.g. `FooService`
observes `BarService` **and** `BarService` observes `FooService`. Single-phase
shutdown cannot accommodate these cases.

## Dependency Management Overview

With that in mind, let's look at how dependency management works. There is a
single `BrowserContextDependencyManager` singleton, which is what is alerted to
`Profile` creation and destruction. All constructors of PKSF register and
unregister themselves with the `BrowserContextDependencyManager` automatically.
The job of the `BrowserContextDependencyManager` is to make sure that individual
services are created and destroyed in a safe ordering.

Consider the case of these three service factories:

```
AlphaServiceFactory::AlphaServiceFactory()
    : ProfileKeyedServiceFactory("AlphaService") {}

BetaServiceFactory::BetaServiceFactory()
    : ProfileKeyedServiceFactory("BetaService") {
  DependsOn(AlphaServiceFactory::GetInstance());
}

GammaServiceFactory::GammaServiceFactory()
    : ProfileKeyedServiceFactory("GammaService") {
  DependsOn(BetaServiceFactory::GetInstance());
}
```

The explicitly stated dependencies in this simplified graph mean that the only
valid creation order for services is \[Alpha, Beta, Gamma\] and the destruction
order is \[Gamma, Beta, Alpha\]. The above is all you, as a user of the
framework, have to do to specify dependencies.

Behind the scenes, `BrowserContextDependencyManager` takes the stated dependency
edges, performs a Kahn topological sort, and uses that in
`CreateBrowserContextServices()` and `DestroyBrowserContextServices()`.

## Debugging Tips

### Using the dependency visualizer

Chrome has a built in method to dump the profile dependency graph to a file in
[GraphViz](http://www.graphviz.org/) format. When you run chrome with the
command line flag --dump-browser-context-graph, chrome will write the dependency
information to your /path/to/profile/browser-context-dependencies.dot file. You
can then convert this text file with dot, which is part of GraphViz:

```
dot -Tpng /path/to/profile/browser-context-dependencies.dot > png-file.png
```

This will give you a visual graph like this (generated January 23rd, 2012, click
through for full size):

[<img alt="Graph as of Aug 15, 2012"
src="/developers/design-documents/profile-architecture/graph5.png" height=40
width=400>](/developers/design-documents/profile-architecture/graph5.png)

### Crashes at Shutdown

If you get a stack that looks like this:

```
BrowserContextDependencyManager::AssertProfileWasntDestroyed()
BrowserContextKeyedServiceFactory::GetServiceForProfile()
MyServiceFactory::GetForProfile()
... [Probably a bunch of frames] ...
OtherService::~OtherService()
BrowserContextKeyedServiceFactory::BrowserContextDestroyed()
BrowserContextDependencyManager::DestroyProfileServices()
ProfileImpl::~ProfileImpl()
```

The problem is that OtherService is improperly depending on MyService. The
framework asserts if you try to use a `Shutdown()`ed component.

---
breadcrumbs:
- - /developers
  - For Developers
page_name: recent-changes-credential-management-api
title: Recent Changes to the Credential Management API
---

## Introduction

Recently there have been multiple changes to the Credential Management API. This
doc aims to summarizes the changes and give developers advice to use the new
features while still being able to support older versions of Chromium.

## Changes in Details

The following developer facing changes have been made:

## The unmediated flag to CredentialRequestOptions is replaced by an mediation enum

This change extends the possible mediation options passed to
navigator.credentials.get. Instead of a boolean flag unmediated get now supports
a CredentialMediationRequirement enum with three different values: “silent”,
“optional” and “required”. Providing mediation: “silent” is equivalent to the
old unmediated: true, while mediation: “optional” is equivalent to unmediated:
false. The novel option is “required”, which always leads to user mediation,
even if auto-signin would be possible otherwise. Similarly as before, mediation:
“optional” is the default option and will be chosen if no other value is
specified.

### Polyfill

In order to migrate existing code that uses the boolean unmediated flag to make
use of the new enum the following patch to get will be necessary. Note that it
is impossible to use mediation: required while being backwards compatible, given
that this is a new feature.

var origNavigatorCredentialsGet = navigator.credentials.get;

navigator.credentials.get = function(options) {

if (!("mediation" in options)) {

options\["mediation"\] = options\["unmediated"\] ? "silent" : "optional";

}

return origNavigatorCredentialsGet(options);

}

## Introduction of navigator.credentials.create

In addition to get, store and requireUserMediation navigator.credentials now
supports a fourth method called create. It can be used to asynchronously create
Credential objects. Currently it expects an option dictionary with a single key
which can be either “password” or “federated”. In case of “password” the value
will be used to instantiate a PasswordCredential object, while in case of
“federated” the key’s value will be used to instantiate a FederatedCredential.
create() returns a Promise, which resolves to a Credential object if successful
and rejects otherwise. However, it is still possible to create Credentials using
the PasswordCredential or FederatedCredential constructor. This continues to be
the preferred way for synchronous creation.

### Polyfill

In order to make use of the new Promise based creation method the following
polyfill is needed in old browsers that do not support it natively:

if (!("create" in navigator.credentials)) {

navigator.credentials.create = function(options) {

return new Promise(function(resolve, reject) {

if (Object.keys(options).length !== 1) {

reject("NotSupportedError: Only 'password' and 'federated' " +

"credential types are currently supported.");

}

if ("password" in options) {

try {

resolve(new PasswordCredential(options\["password"\]));

} catch (e) {

reject(e);

}

}

if ("federated" in options) {

try {

resolve(new FederatedCredential(options\["federated"\]));

} catch (e) {

reject(e);

}

}

reject("NotSupportedError: Only 'password' and 'federated' credential " +

"types are currently supported.");

});

};

}

## Renaming of requireUserMediation to preventSilentAccess

In order to address concerns that navigator.credentials.requireUserMediation can
be confusing depending on the context it has been renamed to
navigator.credentials.preventSilentAccess. However, there is no change in
functionality.

### Polyfill

Given that this is simply a rename the polyfill for old browsers is very short:

if (!("preventSilentAccess" in navigator.credentials)) {

navigator.credentials.preventSilentAccess =

navigator.credentials.requireUserMediation;

}

## Exposing of passwords to JavaScript and deprecation of the custom fetch infrastructure

Perhaps the biggest change is that PasswordCredential.password is now directly
accessible from JavaScript. The reason for this change is that the current
security model of the Credential Management API is not as effective as desired
(see e.g.
[here](https://github.com/w3c/webappsec-credential-management/issues/58)) and
the current fetch based infrastructure has reported usability issues (e.g. it's
impossible to send JSON or apply a hash to the password before sending). While
exposing passwords in JavaScript does not improve the security situation, it
does not significantly worsen it and it also improves greatly the usability of
the API.

Deprecation of the custom fetch infrastructure entails that it is no longer
needed to pass a PasswordCredential object to fetch’s RequestInit object and
that PasswordCredential attributes idName, passwordName and additionalData will
be deprecated and removed later this year.

### Polyfill

If possible, new code should simply access the new password property instead of
relying on the deprecated fetch infrastructure.

If this is not possible, existing code should be migrated in the following way.
Given the following legacy code that passes a Credential object to fetch:

c.idName = "id";

c.passwordName = "password";

c.additionalData = formData;

fetch(url, { credentials: c, method: "POST" });

The same can now be achieved using the following updated code, setting the
necessary attributes in the formData body directly:

formData.set("id", c.id);

formData.set("password", c.password);

fetch(url, { body: formData, method: "POST" });

A polyfill that patches fetch to do this automatically is provided below and
provides compatibility between old and new browsers:

var origFetch = fetch;

fetch = function(input, init) {

if (init && init\["credentials"\] instanceof Credential) {

var credential = init\["credentials"\];

if ("password" in credential) {

var body = credential.additionalData || {};

body\[credential.idName\] = credential.id;

body\[credential.passwordName\] = credential.password;

init\["body"\] = body;

init\["credentials"\] = "include";

}

}

origFetch(input, init);

}

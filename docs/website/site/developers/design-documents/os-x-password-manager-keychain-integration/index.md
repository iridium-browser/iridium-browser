---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: os-x-password-manager-keychain-integration
title: OS X Password Manager/Keychain Integration
---

Note: As of version 45, the password manager is [no longer integrated with
Keychain](https://code.google.com/p/chromium/issues/detail?id=466638), since the
interoperability goal discussed in the Background section is no longer possible.
This document is here for historical purposes only.

---

### Background

The Mac Keychain provides a shared, secure storage implementation that includes
API specifically for storing various types of web passwords (web forms, HTTP
auth, etc.). The platform expectation is that we should store our passwords
there, providing interoperability with other browsers (with the exception of
Firefox), rather than simply using it a private secure-storage mechanism.

### Storage

Because the Keychain schema is limited, and is not extensible, we cannot use it
as the sole storage mechanism—most notably we would lose the "action" field,
without which we have dramatically different user experience for security
reasons. This means we need our own (non-secure) storage for extra information.
Because Keychain entries don't have a stable, unique identifier, and don't have
a field we can use for adding an identifier for our own record, they must be
paired by matching across multiple fields.

The fields that will be stored in each location, in PasswordForm terminology:

> **Keychain**

    *   scheme
        *   kSecAuthenticationTypeItemAttr (direct equivalents exist for
                    each of the SCHEME_\* types)
    *   signon_realm
        *   for SCHEME_HTML: kSecProtocolItemAttr + kSecServerItemAttr +
                    kSecPortItemAttr
        *   for SCHEME_BASIC and SCHEME_DIGEST: as above, with the
                    addition of kSecSecurityDomain
        *   for proxies: TBD (format is "proxy-host/auth-realm"; need to
                    see what other browsers store, and how "origin" is handled
                    on the Chromium side)
    *   origin
        *   signon_realm components + kSecPathItemAttr
    *   username_value
        *   kSecAccountItemAttr
    *   password_value
        *   Keychain data
    *   date_created
        *   kSecCreationDateItemAttr

> **Internal Database** (using LoginDatabase)

> *   scheme
> *   signon_realm
> *   origin
> *   username_value
>     *   The above is duplicate information necessary to match our
                  entries with Keychain entries. They should be sufficient to
                  map uniquely to a Keychain entry, given Keychain's
                  non-duplication enforcement.
> *   action
>     *   This is the most important aspect of our metadata, since the
                  UE changes if we don't have it (see discussion below).
> *   submit_element
> *   username_element
> *   password_element
>     *   Used only for minor scoring changes.
> *   ssl_valid
>     *   This gives us an extra security indicator for passwords we
                  store; for passwords stored by other browsers we must infer it
                  from kSecProtocolItemAttr, similar to how imported passwords
                  are handled on other platforms.
> *   preferred
> *   blacklisted_by_user
> *   date_created
>     *   Primarily for blacklisted_by_user here, which has no
                  corresponding keychain entry

Note: In the PasswordStore implementations for other platforms, the
PasswordStore doesn't need to understand anything about the format of
signon_realm, so it can be free-form, which is not the case here. However, if we
ever need to add a new format that cannot be expressed in Keychain terms, we can
fall back to storing more metadata ourselves and using a generic application
Keychain item for the password.

### Saving, Updating, Retrieving, and Deleting

**Saving** is relatively straightforward, except for a wrinkle with collisions:

1.  Store the keychain entry
    *   If this fails as a "duplicate item", attempt to set the password
                (since the other primary information must already match). If
                that succeeds—which is not guaranteed, since the user may deny
                us access to modify the item—count storage as a success.
2.  If (1) succeeds, store our metadata.

This means that it is possible for us to have multiple entries in our database
(different only by the \*_element fields) mapping to the same keychain entry.

**Updating** is essentially the same:

1.  Store/update the Keychain, exactly as with Saving
2.  If we successfully create or update a Keychain item, update our
            metadata (or if we had no metadata, store some).

**Retrieving (single)** is trickier:

1.  Find all the matching entries in our database, and convert them to
            PasswordForms.
2.  Find all the matching Keychain entries, and convert them to
            PassswordForms.
3.  Walk the database PasswordForm list, and for each item:
    *   If it's a blacklist item, add it to the merged list; otherwise
    *   try to find a matching entry from the Keychain list. If there is
                a match, merge the keychain PasswordForm into the database
                PasswordForm, then move the combined entry to the return list
                (and remember the keychain entry as having been used).
4.  Move everything left in the original keychain list that's not a
            blacklist item into the return list.
    *   These will be treated like imported passwords in the UI due to
                the lack of an action field.
5.  Delete the database entry for anything left in the database list; if
            it's still there the user has deleted the keychain item outside of
            Chromium and it is now useless.

**Retrieving (batch)** differs from single retrieval in handling of Keychain
entries (see Design Choices):

1.  Find all entries in our database, and convert them to PasswordForms.
2.  For each entry, find and merge in the corresponding Keychain entry.
    *   If there isn't one, delete the database entry, since it is now
                useless.

**Deleting (single)** is slightly tricky due to potential sharing of an item by
multiple database entries:

1.  Delete the database entry that we are trying to delete.
2.  Find the Keychain item corresponding to the entry to delete.
3.  If we created the Keychain item (see Design Choices below):
    1.  Find all remaining database items that would use that Keychain
                item.
    2.  If there aren't any, delete the Keychain item.

**Deleting (batch)** works much the same way (and is subject to some
limitations; see Design Choices below):

1.  Delete everything in our metadata database from the given time
            range.
2.  Find everything that's left in the database.
3.  Find everything in the Keychain created by Chromium.
4.  Merge the lists from those two steps, and delete any Keychain
            entries that aren't used.

### Issues/Limitations

*   Translation to PasswordForm format requires accessing the password,
            which will cause UI (from the OS) if the password was saved by
            another application or the Keychain is locked. This will be most
            noticeable for users visiting a site that we find a Keychain
            password for but no metadata of our own, because without an "action"
            we won't auto-fill without user interaction. Users will likely be
            confused/annoyed by being prompted for access to a password that
            doesn't immediately appear to be used.
    *   Some Camino users complained about this general issue, causing a
                change to load passwords lazily. That would be possible in
                Chromium, but would require making PasswordForm a class,
                incorporating at least some platform-specific knowledge.
*   Keychain enforces more uniqueness than Chromium does. Specifically,
            we cannot store two password entries that differ only by \*_element
            fields or by action.
    *   This does not seem likely to be a significant concern; in
                testing of Windows behavior getting into this state appeared to
                require "tricking" Chromium by first storing the new entry with
                a different username, then changing that username.
*   Unlike the other platoforms' password storage, we can encounter
            errors in normal usage (experience with Camino tells us that
            unexpected keychain problems will occur for some number of users).
            There is currently no error reporting, so failures will be largely
            silent, although there is console logging.

### Design Choices

*   Deleting all passwords won't actually prevent Chromium from having
            stored passwords that it may access. Complete batch-deletion will
            leave Chromium in the same state that it started in: having access
            to passwords it did not create, but not trusting them for auto-fill.
    *   This is the more conservative of the two options, and matches
                Camino's behavior; Safari, on the other hand, deletes all items
                it understands whether it created them or not (which runs a high
                risk of unexpected data loss by users who don't realize that
                there is a shared storage system for passwords).
*   Deleting individual password entries created by another browser will
            revert us to the untrusted state for that password, but will not
            delete it from the keychain.
    *   This is primarily to be consistent with batch delete, although
                the the same argument also applies here to a lesser extent.
*   Passwords from the Keychain that we don't yet trust will not be
            listed in the Preferences UI (batch retrieval).
    *   This is a consequence of the decisions about deletion; it would
                be extremely confusing to show entries that didn't go away when
                deleted. We'll need to watch for how much confusion there is
                about Chromium filling passwords (once they are typed) when they
                don't show up in the Preferences.
*   We do not attempt to share blacklist entries with other browsers. We
            don't store our blacklist entries in the keychain, or honor those
            stored by other browsers.
    *   It's not entirely clear that if a user tells Safari never to
                remember a password for a given site that they should be unable
                to store a password there in Chromium. (Camino does not
                currently use or honor blacklist entries in the Keychain.) We
                also run the risk of confusing other browsers that don't make
                use of path information for Keychain entries, so that a
                blacklist for an individual form on a site in Chromium could
                block password storage for the entire site in other browsers.

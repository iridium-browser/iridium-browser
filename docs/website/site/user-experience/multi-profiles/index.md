---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: multi-profiles
title: Multiple Chrome Users
---

**Overview**

The multiple users feature allows the user to associate a set of preferences
with a specific set of browser windows, rather than with an entire running
instance of Chrome. Allowing different windows to run as different Chrome
identities means that a user can have different open windows associated with
different Google accounts, and correspondingly different sets of preferences,
apps, bookmarks, and so on -- all those elements which are bound to a specific
user's identity. Having multiple users in the Chrome browser also makes it easy
to browse with separate identities without having to log in as separate users at
the operating system level.

****Sync and Multiple Users****

**Right now, Chrome provides a way of associating a Google account with a Chrome
profile through the sync mechanism -- if a user "syncs" their browser, the
user's apps, autofill data, bookmarks, extensions, passwords, preferences, and
themes will be associated with the logged-in Google account and stored
off-client, where they become available to the user on any browser on which they
connect to their synced account. Sync provides the basic mechanism for binding a
Google account to the state of a Chrome browser, but provides no special
consideration for different users: the model provides that a browser is either
synced to an account or not synced, and syncing adds data, instead of modifying
it. For example, if a user syncs to account A and then logs out and syncs to
account B, the browser will contain bookmarks from both accounts.**

**In the multiple users model, the idea of syncing will need to adapt to the
understanding that several distinct users (or, at least, identities) can be
associated with a single browser instance -- and that creating a personal
account for the browser need not necessarily establish an off-client repository
to save its state. There are two main issues here:**

**1. The user will be able to create a local account without enabling sync. This
model can be seen as something like starting multiple Chrome instances with
separate user-data-dir directories.**

**2. The user can have a local account with sync enabled; this state replaces the current binary "sync / not synced" model.**

******The act of syncing binds the synced browser window to an identity; thus
the "sync" state is naturally part of the multi-user model -- indeed, it's the
fully enabled state, where a user has a local account bound to a synced account
in the cloud. If a user is running a local account which is not connected with a
Google account profile and chooses to sync, they will be connected to their sync
account as their Chrome identity. If they choose to create a local account
without enabling sync, they will always have the option to enable sync in the
options menu.******

**The First Account and Adding a New User**

Users can take two actions to bind a browser window with a personalized account:

1.  They can add a new user. This creates a separate user account, with
            separate preferences and state.
2.  They can connect (or "sync") any user account to a synced Google
            account. This stores some of a user account's preferences and state
            in the cloud, where it can be synced to accounts on other machines.

**The First User Account:** When users runs Chrome without connecting to their
Google account, they are using a basic, un-synced browser -- the "normal" mode
for the vast majority of users now. When the user comes from this state and
connects to a Google account, the preferences, bookmarks, history, etc. in this
basic account at the time of creation will populate the initial user data
directory for the new account. That is, this basic, default account will now be
connected to the chosen Google account. If the user chooses to create a new
account, a fresh, empty account will be created. If the user chooses to sync
this account to an existing Google account, then the user's preferences from the
chosen account will populate the new account. In either case, the initial
account will remain accessible to the user from the profile menu as the "first
user."

\[*Side note*: right now, browser sync works differently from this. If a user
syncs to identity A, then stops syncing, and then syncs again with identity B,
the browser will end up with all the bookmarks from both A and B. This reflects
the basic binary sync model of "sync/don't sync." If instead we have a model of
multiple identities, following the sync model would mean that each new profile
created would take bookmarks and settings from whichever identity is currently
logged in -- leading to bloat, lack of control, and unwanted mixing of
"personalities."\]

**Sign-in**

The user will be able to add a new Chrome user from from the preferences/options
dialog:

[<img alt="image"
src="/user-experience/multi-profiles/signin_1.png">](/user-experience/multi-profiles/signin_1.png)

**Launching New Windows**

New windows will take their identity from the identity of the last active
window. Active identities persist across sessions; if a user quits Chrome and
then starts it again, the identity of the windows opened will be taken from the
last window open when the browser last ran Chrome. Tabs will not be allowed to
move across account boundaries; that is, one won't be able to drag a window from
the browser belonging to account Alice to a window belonging to account Bob.

**Incognito Mode**

Every user account will have its own incognito mode. If an incognito window is
launched, it takes its settings and preferences from the browser window which is
currently active. We may need a special incognito mode UI to indicate which user
the window is associated with; on the other hand, despite the fact that each
incognito mode draws settings from a specific account, displaying the name of an
account in an incognito browser window may give the wrong impression to users
that their activities in this secret space are being associated with their
identities.

**Security**

Because the data for all users which have been used for an instance of Chrome
are associated with a single operating system identity, there is no expectation
of special privacy. That is, there is no additional encryption of preferences
and settings on the local machine other than that which already exists for user
data directories in Chrome. Obviously, a password is needed to log in to a
specific Chrome identity on the browser. However, no additional protection for
the user data directories is planned. On Mac OS X, in fact, because passwords
are stored in the commonly accessible keychain, it will be possible for a user
in one account to access the passwords that have been stored on that machine by
a user with another account.

**Background Apps and Extensions**

In order to properly handle background apps, which can run even after all
browser windows have closed, Chrome will have to load all extensions for all
accounts associated with a given Chrome instance when Chrome is launched, and
launch background apps even if the particular user associated with them is not
currently loaded. Because background apps will be controllable from the
operating system, control of these applications can be safely and reasonably
given to the user at this level, outside of the browser. That is, if a
background app is loaded for Alice and only Bob is logged in, Bob will be able
to turn off the settings for this application from the OS, without having to log
in as Alice in the Chrome browser.

**Browser Window Identity UI**

Each browser window needs some visual tag to indicate that it is associated with
a particular identity. This is accomplished with a personalizable avatar badge,
in the spot usually occupied by the incognito spy. Once multiple users are set
up for an instance of Chrome, the "Users" field in the personal settings menu is
populated with a list of existing user accounts. Any user can add or delete
accounts using this interface, and the avatar image and account name can be
personalized through the settings page belonging to the account being edited.

[<img alt="image"
src="/user-experience/multi-profiles/cupcake_2.png">](/user-experience/multi-profiles/cupcake_2.png)

**User Launch Menu**

The identity tag on the browser frame provides the option to launch a new window
connected to an existing user account (any account which has been used in this
browser will be listed in a dropdown), as well as the option to create a new
user account. In addition, there will be the option of editing the account
belonging to this browser (this account will be tagged with a check mark in the
dropdown menu). If a user is signed in to a syncing Google account, that
account's name will be displayed in the dropdown menu.

[<img alt="image"
src="/user-experience/multi-profiles/profile_menu3.png">](/user-experience/multi-profiles/profile_menu3.png)

**Disconnecting**
Disconnecting from an attached account will not result in any changes to the
user's immediate browsing experience; the only change will be that alterations
in the profile will not be synchronized with the profile state stored
off-client.

For questions or comments, contact mirandac at google dot com.

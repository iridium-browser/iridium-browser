---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: profile-chooser-menu
title: Profile Chooser Menu
---

The profile chooser menu, also known as the avatar menu, is represented by
different, platform-dependent views and models. The AvatarMenu class is used to
retrieve items for the menu and responds to actions like switching, adding and
editing profiles.

[<img alt="image"
src="/developers/design-documents/profile-chooser-menu/Avatar%20Menu%20Classes.png">](/developers/design-documents/profile-chooser-menu/Avatar%20Menu%20Classes.png)

## **AvatarMenu**

AvatarMenu defines the Item struct and provides methods for retrieving these
items and interacting with an item's associated profile. Most of its functions
delegate tasks to either the ProfileList object or the AvatarMenuActions object,
both of which are created in AvatarMenu::AvatarMenu(). The various Views use
AvatarMenu to determine whether the menu should be shown, to access the menu
items, and to respond to clicks on menu items or links.

**ProfileList**

AvatarMenu delegates generation of the profile list to the ProfileList
interface, which currently has two implementations. On desktop Chrome,
ProfileListDesktop simply exposes all of the profiles in LocalState for
inclusion in the menu. ProfileListChromeOS restricts the listed profiles to
those that are associated with logged-in users, hiding the Default profile and
profiles for users who are not logged into the session (including managed
users); additionally, the menu items' icon fields are populated with user images
instead of profile avatar icons. Both implementations rely on the
ProfileInfoInterface (ProfileInfoCache), and ProfileListChromeOS also uses
UserManager.

**AvatarMenuActions**

The AvatarMenuActions interface controls behavior such as adding and editing
profiles, as well as determining whether these links should be shown. As of this
writing, AvatarMenuActionsDesktop allows for adding and editing profiles through
the Settings sub-page, as well as signing out of a profile, while
AvatarMenuActionsChromeOS disallows editing (i.e., returns false from
ShouldShowEditProfileLink) and only lets the Add Profile link be shown if the
right conditions are met with respect to the user's type and other users on the
machine. Currently, AvatarMenuActions implementations use the Browser object; in
the future, this dependency should be removed by delegating actions like
AddNewProfile and EditProfile.

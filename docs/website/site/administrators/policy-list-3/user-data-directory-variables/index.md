---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
- - /administrators/policy-list-3
  - Policy List
page_name: user-data-directory-variables
title: Supported Directory Variables
---

The policy for modifying the user data directory and other paths for Chrome to
use has support for several variables, so you don't need to set a hard-coded
path for all users. For example, if you want to store your profile data under
the user local application data on Windows, set the UserDataDir policy to :
"${local_app_data}\Chrome\Profile" Which on most Windows 7 / Vista
installations would resolve to :
C:\Users\CurrentUser\AppData\Chrome\Profile.

**Chrome profiles are _not_ backwards compatible, so storing the user profile
on a network drive and using it with different versions of Chrome can cause
crashes and data loss.** Please see
<http://www.chromium.org/administrators/common-problems-and-solutions> for more
details.

### Some hints about setting paths in policies

* All policies that concern paths where Chrome stores different data are
  platform-dependent. Some of these are only available on specific platforms
  but others can be used on all platforms.
* Paths should be absolute to avoid errors caused by applications starting in
  different location on different occasions.
* Every variable can occur **only once** in a path. For the majority of them,
  this is the only meaningful way of using them as they resolve to absolute
  paths.
* Almost all policies will create the path if it doesn't exist and if possible
* **Using network locations for some policies can lead to unexpected results.**
  For example, the user profile is not backwards-compatible, so running a
  different version/channel of Chrome with the same profile may corrupt your
  profile.

### List of supported path variables

#### ChromeOS only

**${google_drive}** - The root directory of Google Drive. \
(example resolution: "johndoe")

#### All platforms besides ChromeOS

**${user_name}** - The user that is running Chrome (respects suids). \
(example resolution: "johndoe")

**${machine_name}** - The machine name possibly with domain \
(example resolution : "johnny.cg1.cooldomain.org" or simply “johnnyspc”)

#### Windows only

**${documents}** - The "Documents" folder for the current user. \
(example resolution : "C:\Users\Administrator\Documents")

**${local_app_data}** - The Application Data folder for the current user. \
(example resolution : "C:\Users\Administrator\AppData\Local")

**${roaming_app_data}** - The Roamed Application Data folder for the current user. \
(example resolution : "C:\\Users\\Administrator\\AppData\\Roaming")

**${profile}** - The home folder for the current user. \
(example resolution : "C:\\Users\\Administrator")

**${global_app_data}** - The system-wide Application Data folder. \
(example resolution : "C:\AppData")

**${program_files}** - The "Program Files" folder for the current process.
Depends on whether it is 32 or 64 bit process. \
(example resolution : "C:\Program Files (x86)")

**${windows}** - The Windows folder
(example resolution : "C:\WINNT" or "C:\Windows")

**${client_name)** - The name of the client pc connected to an RDP or Citrix
session. Take into account that this variable is empty if used from a local
session. Therefore if used in paths you might want to prefix it with something
which is guaranteed to be non-empty. \
(example : C:\chrome_profiles\session_${client_name} - produces
c:\chrome_profiles\session_ for local sessions and
c:\chrome_profiles\session_somepcname for remote sessions.)

**${session_name}** - The name of the active session. Useful for distinguishing
multiple simultaneously connected remote sessions to a single user profile. \
(example resolution: WinSta0 for local desktop sessions)

#### MacOS only

**${users}** - The folder where users profiles are stored \
(example resolution : "/Users")

**${documents}** - The "Documents" folder of the current user. \
(example resolution : "/Users/johndoe/Documents")

---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/obsolete
  - Obsolete Documentation
page_name: portage-package-status
title: Portage Package Upgrade Initiative
---

### How to add/update a package

Read the ["Portage New & Upgrade Package
Process"](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/portage/package_upgrade_process.md).

**Note: The following material is old - we do not maintain this process
anymore.**

### **Purpose**

Many of the Portage packages that we use today as part of the Chromium OS image
are outdated with respect to the latest version available from upstream Gentoo
(Portage). Some are very outdated. This site serves as the coordination point
for an effort to upgrade our packages to recent versions.

To spread out the effort of upgrading hundreds of packages each package should
be associated with one owner from among Chromium OS contributors. Volunteer
owners are always welcome, but where they are not received packages are assigned
to owners through technical teams.

Package status and ownership is maintained on the [Portage Package Status
spreadsheet](https://spreadsheets1.google.com/a/chromium.org/spreadsheet/ccc?key=tJuuSuHmrEMqdL5b8dkgBIA).
Package owners can check that spreadsheet at any time to see if any of their
packages are out of date. The [second
sheet](https://spreadsheets.google.com/a/chromium.org/spreadsheet/ccc?pli=1&key=0AsXDKtaHikmcdEp1dVN1SG1yRU1xZEw1Yjhka2dCSUE#gid=1)
of the spreadsheet presents the data statistically and with pretty charts.

### **What you should do**

Check the [package status
spreadsheet](https://spreadsheets1.google.com/a/chromium.org/spreadsheet/ccc?key=tJuuSuHmrEMqdL5b8dkgBIA),
and claim ownership of packages that you think you should own (**all
chromium.org accounts can edit the document**). This is a team effort, and we
need everyone to do their part. If you think you are best positioned to
evaluate/test an upgrade, then you are the logical owner. Perhaps you work on
projects that use that package extensively. *Please only edit the "Owner" and
"Owner Notes" columns.* Edits in other columns will be overwritten by the update
process.

To complete ownership coverage beyond volunteers, packages are assigned to
technical leads who are then responsible for assigning individual ownership.
*The "Team/Lead" column is for this purpose.*

As a package owner, whenever you are ready you should upgrade the outdated
packages that you own. See the "How to update a package" section for details.

### Managing upgrade process

There is a "Tracker" column in the spreadsheet for linking to a Tracker issue
for a package that needs an upgrade. This column is managed by the
sync_package_status script. The script runs daily as part of the
[refresh-packages
builder](http://build.chromium.org/p/chromiumos/builders/refresh%20packages),
but only for teams that have opted in to this process. It can also be run by
team leads or package owners whenever they like. It is provided as a
convenience. Run the following from within your chroot to get started:

```none
sync_package_status --help
```

Most likely, you will want to run sync_package_status for either a team or for
an individual. Those examples are given below (again, run inside your chroot).
Remove the "--pretend" option if you are satisfied with the results:

<pre><code>
sync_package_status --pretend --team=build
sync_package_status --pretend --owner=<i>chromiumuser</i>
</code></pre>

To see statistics and charts about the upgrade progress of the entire team or
your group, see the
[Stats](https://docs.google.com/a/chromium.org/spreadsheet/ccc?key=0AsXDKtaHikmcdEp1dVN1SG1yRU1xZEw1Yjhka2dCSUE#gid=1)
worksheet of the same spreadsheet.

To disable the filing of Tracker issues for a single package, change the
"Tracker" column value for that package's row to "disable".

### **Spreadsheet Legend**

In the "State On &lt;arch&gt;" column:

*   "current" means the current version is the same as the **stable**
            upstream version, and is coming from the portage (or portage-stable)
            overlay.
*   "local only" means the package (presumably) is not from upstream in
            the first place.
*   "patched locally" means the package is not coming from the portage
            (or portage-stable) overlay, with a version different from anything
            upstream.
*   "unknown" means no upstream version was found but the package looks
            like it originally came from upstream.
*   "duplicated locally" means the package is not coming from the
            portage (or portage-stable) overlay, but the version is identical to
            one upstream. (Note that this status can appear even if the version
            is not the latest one upstream.)
*   "needs upgrade" means a more recent **stable** version is available
            upstream.
*   "needs upgrade and patched locally" is a combination of the two
            states.
*   "needs upgrade and duplicated locally" is another combination of two
            states.

In the "Package" column:

*   If the package name has a ":&lt;slotname&gt;" suffix, then we are
            using that package in more than one Portage "slot". We should try to
            avoid this as much as possible by consolidating our usage to one of
            them.

### **FAQ**

**Q:** *Where does the spreadsheet information come from?*

**A:** It comes from gathering package information for boards 'x86-generic' and
'arm-generic' over each of the following targets: 'virtual/target-os',
'virtual/target-os-dev', and 'virtual/target-os-test', as well as the chroot
host (amd64) over each of the following targets: 'world', and
'virtual/target-sdk'.

**Q:** *Who is maintaining the package status spreadsheet?*

**A:** The update process for the spreadsheet is maintained by the build team;
send questions to chromium-os-dev@chromium.org.

**Q:** *Who can access the spreadsheet?*

**A:** Use your chromium.org account to access the spreadsheet for automatic
"view and edit" privileges. If you don't have a chromium.org account, simply
request access to the document (you have the option to do so after trying to
access the document) and you will be given "view" privileges.

**Q**: *I got a Tracker issue asking me to upgrade package FOO, but we don't
want to upgrade package FOO. How do I exclude FOO from the automatic issue
filer?*

**A**: In the [packages
spreadsheet](https://spreadsheets1.google.com/a/chromium.org/spreadsheet/ccc?key=tJuuSuHmrEMqdL5b8dkgBIA),
find the row for package FOO and change the Tracker value for that row to be
"disable".

**Q:** *How often is the package status data in the spreadsheet updated?*

**A:** The update process is executed daily as part of the [refresh-packages
buildbot](http://build.chromium.org/p/chromiumos/builders/refresh%20packages).

**Q:** *How do I tell when the package status data in the spreadsheet was last
updated?*

**A:** Go to File -&gt; See revision history, the look for the last time the
"bugdroid" user edited the spreadsheet. Or look at the [refresh packages
builder](http://build.chromium.org/p/chromiumos/builders/refresh%20packages) to
see when it last ran to completion.

**Q:** *Why can't I access the spreadsheet, even when I use my chromium.org
account?*

**A:** This happened to at least a couple people, and the solution was to
convert their account to GA+. Contact Mark Larson.

**Q:** *Are there any free, open-source utilities that could help with this
effort?*

**A:** We are aware of [euscan](http://euscan.iksaif.net/). It was evaluated for
use with Chromium OS, and the results can be found in this
[doc](https://docs.google.com/a/chromium.org/document/d/1QejmQQokk1lKS9PGuS_Zs279Z1cATNwYc9IVPyL__ZQ/edit).

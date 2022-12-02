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
- - /developers/design-documents/extensions/proposed-changes/extension-system-changes
  - Extension System
page_name: install-dialog-2
title: Install Dialog v2
---

### **Problem**

The current extension install dialog displays a different string of text
depending on the permissions the extension requests. This has a few problems:

1.  It isn't extensible. We keep adding new permissions, but it's hard
            to update the text strings to deal with all possible combinations of
            permissions.
2.  It isn't clear. Users don't realize that the text is telling them
            something specific about the extension they're installing.
3.  Advanced users have complained that they would like to see the
            details of the exact permissions being requested, not the text form.
4.  Very advanced users have requested the ability to disable individual
            permissions during installation.

### **Proposal**

The current set of permissions are:

*   Data on a domain (eg www.google.com)
*   tabs system details (urls, titles, etc)
*   browsing history
*   geolocation
*   unlimited storage quota
*   desktop notifications

Some of these permissions are too granular to be useful to most users during
installation, and should be hidden by default. For example, most users won't
want to approve access to desktop notifications during installation.
Installation should grant that privilege automatically. Similar for extra
storage quota. Access to the tab system and to the history system are basically
the same kind of access, so those should be grouped.

Thus the user-visible permissions for the above would be simplified to:

*   Data on a domain
*   Browsing history (tabs, history, etc)
*   Geolocation

The new install dialog should show a list of high-level permissions by default,
but give advanced users a way to see the detailed list if they want.

So something like:

+=============================================+

| +------+ |

| | | |

| | icon | Install Gmail Checker? |

| | | |

| +------+ |

+---------------------------------------------+

| This extension will have access to: |

| |

| ! Your private data on mail.google.com, |

| www.google.com, and 3 other web sites. |

| |

| ! Your physical location on planet Earth. |

| |

| ! Your browsing history. |

| |

| \[more details\] |

| |

| |

| \[ ok \] \[cancel\] |

+=============================================+

When the user clicks "more details", the dialog expands and shows all the
permissions requested in as much detail possible. In the future, we might also
make it possible to turn off individual permissions in that area, but the
extension system doesn't support that yet.

### Text details for high-level permissions

**Site access**

Your private data on www.google.com.

Your private data on www.google.com and foo.com.

Your private data on www.google.com, foo.com, and 5 other sites.

Your private data on all web sites.

**Tabs and History**

Your browsing history.

**Geolocation**

Your physical location on planet Earth.

**NPAPI**

Full access to your computer and private data.

**Camera (future)**

Your computer's camera

**Microphone (future)**

Your computer's microphone

**Infinite Storage, Notifications**

&lt;none&gt;

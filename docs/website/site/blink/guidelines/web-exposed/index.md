---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/guidelines
  - Guidelines and Policies
page_name: web-exposed
title: Definition of a web-exposed change to Chromium
---

Web-exposed is defined as **affecting web API behavior to the point that
developers using that API need to be aware of it**.

Changes that are web-exposed:

*   Any change that adds a new API, or in general modifies any IDL file
            to change what APIs developers see.
*   Removal of some or all of an API.

Cases that are usually not web-exposed:

*   Fixing a bug in the implementation to conform better with a defined
            specification.

Cases that might or might not be web-exposed:

*   Fixing a broken implementation in a way that may break significant
            numbers of sites.

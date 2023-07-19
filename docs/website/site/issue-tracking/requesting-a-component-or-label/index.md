---
breadcrumbs:
- - /issue-tracking
  - Issue Tracking
page_name: requesting-a-component-or-label
title: Requesting a Component or Label
---

[TOC]

## Requesting a Component or Label

Project members can request the creation of new components or labels by simply
filing an issue:

*   [Component
            Request](https://bugs.chromium.org/p/chromium/issues/entry?template=Component%20Request)
            (Template)
*   [Label
            Request](https://bugs.chromium.org/p/chromium/issues/entry?template=Label%20Request)
            (Template)

## Component versus Label

*   Components are meant to track work on a specific feature/ function.
*   Labels generally are meant to track effort that cuts across multiple
            components (e.g. Proj(ects), Hotlist(s), etc...)

## Component Guidelines

*   Guideline 1 (Clarity): Component name should be descriptive beyond
            the core project team (i.e. please avoid using non-industry standard
            abbreviations, code words,project names, etc...)
*   Guideline 2 (Permanence): Component names should describe
            features/functions and not team names, code locations, etc..., which
            are more subject to change and make the hierarchy less predictable
            for people triaging issues.
*   Guideline 3 (Specific): Components are meant to explicitly track
            functional work areas. If you are trying to track a Proj(ect) or an
            on-going effort (e.g. Hotlist-Conops), please instead request a
            label for a (Proj- or Hotlist-)
*   Guideline 4 (Discoverable/ Predictable): Components should be
            parented where people would logically expect to find them (i.e.
            follow product decomposition when naming versus team decomposition).

## Label Guidelines

*   Guideline 1: Unless a hierarchy of labels is needed, please avoid
            creating new root parents.
*   Guideline 2: Proj(ects) labels should be used for efforts that have
            a clear start/ end
*   Guideline 3: Hotlist labels should be used for tracking on-going
            efforts across multiple components (e.g. Hotlist-GoodFirstBug)

### Description Best Practices (for components and labels):

*   Monorail supports scanning the descriptions of components and labels
            during auto-complete. This has some important implications.
    *   (Best practice) Adding abbreviations (e.g. JS, GC, I18N, L10N,
                WASM, etc...) into the description of your components will give
                you and your users a nice shorthand.
    *   (Best practice) If there are synonyms not currently in your
                description, please consider adding them.
    *   (Best practice) Avoid using negative descriptions (e.g. "Not
                foo") as they will appear in auto-complete for the subject being
                avoided (e.g. "foo").

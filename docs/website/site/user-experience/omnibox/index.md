---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: omnibox
title: Omnibox
---

[TOC]

## Introduction

Most modern browsers are equipped with a toolbar, featuring both a URL and a
search field. While straightforward, this UI configuration can lead to some
confusion and a general feeling of unnecessary complexity. The purpose of
Chromium's omnibox is to merge both location and search fields while offering
the user some highly relevant suggestions and / or early results.
There were two trains of thought on what the omnibox should offer. Some
reviewers felt that current browsers currently try to guess what the user is
typing and offer suggestions. These reviewers would like some better
accelerators and some more satisfying shortcuts, these two approaches are
detailed below:

> Accelerate my typing
> The omnibox should focus on augmenting the user's commands - all of the tools
> it provides should be oriented around making the user's input 'better', and
> sending them to a destination.
> Show me results
> If some highly likely results exist for the user action, these results should
> be visible in the omnibox. For example, we could show very probable results
> which are coming from web search or history.

Chromium focuses on the 'accelerate my typing' system - while showing results
was powerful for certain navigational tasks, the designs we tested generally had
too much cognitive overhead to be useful in normal situations.

Additional details on the philosophy, architecture and implementation details
[are available
here](https://docs.google.com/document/d/1Dk_U-zXiMFynKOYYKrS6VKUXrON_ta2mYnAyHa-Uvc0/edit).

## Design Principles

> ### Reduce cognitive overhead

> > An early motivator of combining the search and address bars was a conviction
> > that, even for users who understood the difference between the two, having
> > to make a choice of where to input text was a small (generally subconscious)
> > cognitive load. Combining the two does not just serve novice users, it
> > reduces cognitive load for more advanced users. Similarly, the rest of the
> > omnibox design seeks to minimize cognitive load.
> > One example of this is the graphical design of the dropdown. Items are
> > displayed very simply, with a minimum of iconography, bright colors, font
> > changes, and other eye-catching elements. This makes the dropdown less
> > distracting for cases where users are simply trying to enter text in the
> > edit field and get where they want to go, at the cost of conveying less
> > information about each item when users are carefully scanning their various
> > options. We believe that users ignore the dropdown most of the time, and so
> > this tradeoff is the right one. Even when users look at the dropdown, this
> > design makes it simple to rapidly scan the available choices rather than
> > having to carefully read each one.

> ### Stability and predictability

> > In a control with as many heuristics as the omnibox, users can easily feel
> > out-of-control. Accordingly, both the edit field and dropdown seek to be as
> > stable and predictable as possible. Inline autocomplete is designed to
> > operate synchronously to avoid flickering or race conditions, and triggers
> > on very simple conditions (for most sites, after typing the hostname once)
> > so that its behavior doesn't suddenly change after long usage. The
> > top-ranked item in the dropdown should not change as more results come in.
> > The dropdown itself is always open while users edit, and always has a
> > selected entry, so that the action Chromium takes on hitting &lt;enter&gt;
> > can always be predicted. Asynchronous background queries stop when users
> > begin arrowing through the dropdown so the results don't change as users
> > attempt to select desired items. A highlight marks the hovered row as users
> > mouse over the dropdown so it will be clear which selection will be used if
> > they click.

## Input Types and Examples

> The biggest challenge faced by the omnibox is figuring out what the user wants
> - the various input types are detailed below:

> ### Single word

> > Example (Search): cheese
> > Example (URL): localhost
> > Single-word searches are hard for us to distinguish from single-word URLs -
> > previous solutions have relied on synchronous DNS lookups to figure out if a
> > user was typing a single-word URL, but such lookups add an unreasonable
> > overhead to the search experience and don't always lead to an expected
> > result. For example, if you wish to look up what 'localhost' means, but you
> > have a local webserver running, the result can be infuriating.
> > In Chromium, we decided that consistency and speed was best, and given that
> > the range of 'single-word inputs meant as searches' dwarfs the number of
> > 'single-word inputs meant as URLs', we default to displaying web search
> > results while doing a background DNS lookup to figure out if a local host
> > exists - if it does, we display a "Did you mean http://input/" infobar as
> > show below:
> > <img alt="image" src="/user-experience/omnibox/cheese_results.png">
> > If a user accepts the 'did you mean' suggestion, the auto-complete system
> > will ensure that subsequent entries of the given term will go to that URL.
> > A user can override the search-first behavior by making the search term look
> > like a URL fragment - typically adding a slash to the end of the input is
> > the easiest way of accomplishing this (eg. "cheese/").
> > If a single-word input is in-progress and matches an URL in the user's
> > autocomplete database, we may autocomplete to that full URL.

> ### URL (or URL Fragment)

> > Example: www.google.com
> > Example: www.blues
> > Example: http://localhost
> > Example: pie/
> > Items that look like URLs will be treated as such. There is a range of logic
> > for determining whether something looks like a URL, as there are cases where
> > multiple-word input can still be intended as a URL (editing of query
> > parameters without using '+' or '%20' instead of spaces, for example).

> ### Multiple words

> > Example: where can I buy meat pies?
> > Example: XKCD meaning
> > Multiple-word input is generally treated as search input, unless the first
> > word matches one of the user's keywords.

> ### Search Keyword

> > Example: images.google.com ponies
> > Example: g fastidious
> > If a user has search keyword, the default action may be to activate that
> > keyword using the terms following the keyword as input.

## Tab to Search

> If a user is partway through typing something that auto-completes to a
> keyword, we allow the user to press **Tab** to jump to the end of the
> auto-completion and begin their keyword search term input. This is powerful
> when combined with our automatic keyword creation - a user who does a search
> on amazon.com in the course of their normal browsing will be presented with
> the option of pressing **Tab** to do an Amazon search the next time they type
> something that auto-completes to amazon.com, leading to the ability to type
> 'ama \[tab to complete\] pies'. We call this functionality 'tab to search'.
> [<img alt="image"
> src="/user-experience/omnibox/keyword.png">](/user-experience/omnibox/keyword.png)

## Result Types

> [<img alt="image"
> src="/user-experience/omnibox/omnibox_results.png">](/user-experience/omnibox/omnibox_results.png)

> ### Search for...

> This searches for the typed query - this will be the topmost and default item
> if the input looks like a search string.

> ### URL

> If the input (after auto-complete, if applicable) looks like a URL, this will
> repeat the user's input and be the default, top-most item.

> ### Previous URLs

> If the input matches a previous URL, those URLs will be shown here.

> ### Nav Suggest

> Uses the user's default search provider to suggest URLs based on the user's
> input.

> ### Search Suggest

> Uses the user's default search provider to suggest search terms based on the
> user's input.

> ### History Results

> Shows the number of entries in the user's history that match the given input -
> selecting this item will take the user to the history results page for their
> given search term.

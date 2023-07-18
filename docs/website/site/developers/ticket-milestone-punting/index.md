---
breadcrumbs:
- - /developers
  - For Developers
page_name: ticket-milestone-punting
title: Ticket Milestone Punting
---

# Overview

Chromium issue tickets get labelled for the earliest Chrome milestone they can
get a fix into (or hope to get a fix into). Eg: label "M-41". However, once the
target milestone reaches its branch date leading up to Beta Channel release
(splits off from trunk), the gateway for getting changes into the milestone
branch becomes much more controlled. This is an important part of the process to
get the branch stabilized before it hits Stable Channel.

As a result of this milestone branch state, we start to automatically push
(punt) open tickets that are marked for this milestone into the next one. We do
this automatically based on the labels that have been applied to the ticket.

The purpose of this page is to document the punting label logic, which defines
whether an issue fix meets priority requirements to still potentially make it
into the branched milestone.

**It is very important that we work together to keep post-branch changes/merges
to a minimum. We need to do the best job possible to stabilize the milestone
branches before they hit our users on stable channel. Please do not introduce
unnecessary change and risk by inaccurately labeling your tickets, in the hopes
of getting a merge.**

**Labels that will NOT get auto punted from the post-branch milestone**

*   Pri=0
*   Type=Bug-Regression
*   Type=Bug-Security
*   Type=PrivacyReview
*   Type=SecurityReview
*   Type=Launch-Accessibility
*   Type=Launch-Experiment
*   Type=Launch,Launch-OWP
*   Merge=Approved,Requested,Merged
*   label:merge-merged *(leave tickets that have already been merged to
            a branch "merge-merged-X")*
*   has:ReleaseBlock *(leave tickets that have any "ReleaseBlock-X"
            label)*
*   label:Blocks *(leave tickets that have a ChromeOS "Blocks-X" label)*
*   Review=Security,Privacy
*   iOS=Merge-Approved,Merge-Requested

(Note: tickets could still be manually rejected.)

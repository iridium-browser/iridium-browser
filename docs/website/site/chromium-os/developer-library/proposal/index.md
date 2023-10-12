---
breadcrumbs:
- - /chromium-os/developer-library
  - Chromium OS > Developer Library
page_name: proposal
title: CrOS Developer Library Proposal
---

# Context

## Objective

Consolidate ChromeOS developer documentation in a centralized public repository
which contains reference and training materials written for both internal and
external engineers, the aim of which is to increase proficiencies and
productivity. The content is well structured, highly curated, and easy to
augment.

## Background

The Q2 2023 EngSat CrOS developer survey revealed documentation to be a common
pain point for engineers with 50% of the org reporting that documentation does
not meet their needs and ‘Poor or missing docs‘ as the top factor hindering
productivity. Issues include not knowing where to find resources, missing
documentation, and confusing or poorly written content.

Without a high quality, centralized library, teams create bespoke documentation
which are variations on a theme (a quick search turns up over twenty Getting
Started guides). Not only are efforts duplicated, but these resources get out of
date and confuse other developers who find them via search.

External developers (e.g. OEM partner engineers) do not have access to internal
documentation and struggle with the limited documentation which is public.

## Goals

1. One authoritative location for CrOS developer documentation.
2. Documentation and training material for the critical tools, processes, and
   infrastructure of the development workflow.
3. Access for both internal and external developers.
4. Meaningful content structure to help users find what they are looking for.
5. Resources to enable scalable, high quality documentation creation.
6. Influencing org culture to prevent bespoke documentation and generally
   increase documentation quality and availability.

## Success Metrics

* EngSat: % of engineers reporting poor or missing docs down 10% (measuring 28%
  as of Q2 2023).
* EngSat: % of engineers with documentation not meeting needs down 10%
  (measuring 50% as of Q2 2023).
* Library page views increase Q/Q.

# Proposal

## CrOS Developer Library

The library is the authoritative developer reference containing well structured
and highly curated documentation and training materials hosted at
go/cros-dev-library (Goal 1). The creation of this library involves cataloging
the myriad existing resources spread across the org, moving these resources into
the library, and deprecating the existing sites. New documentation and training
materials are created to fill in knowledge gaps (Goal 2). Since the hosting site
is public, internal and external developers will have access and benefit from
these improvements (Goal 3).

## Content structure

The library is organized by topic in a tree-based hierarchy. Readers intuitively
drill down to find the resource they are looking for (Goal 4). Site search also
helps users find content. The top level content categories are:
* Getting Started - A guide to start from zero to landing code.
* References - Educational descriptions of tools, processes, and infrastructure.
* Guides - Step-by-step instructions for accomplishing tasks.
* Glossary - Definitions of relevant terms and acronyms.
* Index - A sitemap of the library's resources.
* Library Administration - Resources for maintaining and contributing to the
  library.

## Public access vs confidentiality

The guiding principle when determining which resources should be in the public
library is that any information relevant to the developer experience which is
non-confidential should be in the library. Confidential information should be in
a well-organized location at go/chromeos. Privileged information should be
caught in the review process and through automated tooling as part of the CL
upload process.

## CrOS technical writing resources

g/cros-dev-library-reviewers is a group of volunteers who provide reviews and
guidance on technical writing. Templates exist in the Library Administration
section of the site which help content authors quickly create high quality
material (Goal 5).

## Leadership buy-in

CrOS executive leadership approve the creation of the library and encourage
investments across the org, including carving out time in OKRs to improve
documentation and participate in documentation fix-its. Leads prioritize the
library cross-functionally, enabling roles like Tech Writers and DevRel to
invest in it. Leads encourage documentation efforts towards the library and
discourage creating bespoke resources (Goal 6).

## Feedback

A documentation survey is sent out quarterly to understand the needs of readers
including overall satisfaction and topics which are lacking adequate
documentation. Each document in the library has a link soliciting feedback. The
link leads to a buganizer template attached to the CrOS Documentation component.

# Plan of record

## Phase 0: Catalog

* Complete the CrOS Dev Documentation Catalog, listing existing documentation
  resources and topics covered.
* Gain approval from dev.chromium.org/chromium-os OWNERS.

## Phase 1: Consolidate

* Complete the chromiumos/docs migration as proposed in Future of
  chromiumos/docs/.
* Restructure the Getting Started guide on the library and deprecate the guides
  enumerated in the CrOS Dev Documentation Catalog.
* Create a well-named group <insert name here> of individuals who review
  documentation changes and provide tech writing guidance, document the groups
  purpose and expectations, and invite others to participate.
* Create the Infra > Documentation buganizer component against which feedback
  and documentation bugs are filed.
* Migrate the remaining docs in the CrOS Dev Documentation Catalog.

## Phase 2: Content curation and creation

* Create and publish content templates.
* Document the content review process and resources for authors at the Library
  Administration page.
* Survey chromeos-tech@ for documentation pain points (e.g., critical tools,
  processes, and infrastructure which are lacking high quality documentation)
  and fill the gaps.
* Host a Dev Documentation FixIt.


## Phase 3: Maintenance and feature

* Perform a monthly review of the library structure and content changes.
* Request feedback from developers to prioritize feature requests and bug fixes.

# Timeline

| Phase | Status | Timeline |
| ----- | ------ | -------- |
| Phase 0: Catalog | Completed | 08/2023 |
| Phase 1: Consolidate | In progress | 10/2023 |
| Phase 2: Content curation and creation | Not started | Q4 2023 |
| Phase 3: Maintenance and feature requests | Not started | Indefinite |
---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: test-dependencies-in-dynamic-suites
title: Test Dependencies in Dynamic Suites
---

[TOC]

****Not all tests can be run on all machines. Currently, the dynamic suite infrastructure has no way to deal with this issue. Our intent is to use the existing autotest control file DEPENDENCIES field to express what a test needs in order to be able to run. Any label in use in the autotest lab is valid in a test’s DEPENDENCIES list.****

**# DEPENDENCIES in autotest control files today**

**Currently, it seems that DEPENDENCIES in control files are parsed in only when tests are imported for use via the web front end, and they (may be) honored when tests are scheduled via that method.**
**Dependencies can also be specified when running a test via the create_job() RPC exposed by the AFE.**

**# DEPENDENCIES in dynamic suites**

**There are two, related, issues here. One is how individual tests will declare the things they need from a host in order to run, and how the dynamic suite infrastructure will schedule these tests on appropriate machines. The second, more complex issue, is how the dynamic suites code will ensure that -- for a given suite -- it reimages a set of hosts that will enable all tests in the suite to be scheduled.**

**## Scheduling tests on appropriate hosts**

**As the dynamic suite code is already parsing each individual test control file it intends to run, it will not be too hard for it to add the contents of DEPENDENCIES to the metahost spec that it already builds up when scheduling tests with the create_job() RPC.**

**## Determining which machines to reimage for a suite**

**There are several parameters that will inform dynamic suites’ decision about which machines to reimage for a suite:**

1.  **the board being tested,**
2.  **the number of machines the suite asks to use, and**
3.  **the collective dependencies of all tests in the suite.**

**These criteria introduce some new potential failure modes for suites:**

1.  **there is at least one test with unsatisfiable dependencies**
    *   **We should just fast-fail the suite, because we're not going to
                be able to complete it.**
2.  **all dependencies are satisfiable, but not by the number of
            machines requested**
3.  **all dependencies can be satisfied by &lt; num machines, but none
            of the remaining machines satisfy any tests in the suite**
    *   **consider a bluetooth test suite that wants two machines, but
                we have only one machine with bluetooth**

**There are also some resource efficiency concerns here:**

1.  **Machines with rare labels, or label combos, should be avoided if
            possible**
    *   **The only i5 lumpy with a Y3400 modem and swedish keyboard
                should not be used when an i5 wifi-only lumpy from any region
                will do**
2.  **One test in a suite with a rare label should not lead to the suite
            monopolizing all hosts with that label**
    *   **a suite that asks for 4 machines and has 15 tests that will
                run anywhere and one that requires GOBI2K should grab 1 GOBI2K
                device and 3 wifi-only ones.**

**## The naive algorithm (in broad strokes)**

**There’s a set of machines in the lab, each of which is described by a set of labels There’s also a suite (read: set) of tests, each of which is described by a set of labels (read: dependencies). For a given N, we need to find a set of N machines such that the dependencies of each test in the suite are satisfied by some machine in the set.**
**The difficulty is that we want to use ‘rare’ machines only if we really need them. If we weight hosts somehow, this should allow us to build a scoring function that we can use in a simulated annealing or hill-climbing kind of approximation algorithm.**

**### No, it’s not a set-cover problem**

**At first blush, this problem felt to me like a set-cover problem. Merge all the dependencies of all tests in the suite into a single, de-dup’d list, and then find the smallest number of devices whose label sets covered the whole she-bang. The problem is that this algorithm could provide a solution that leaves some tests unsatisfied. Consider the following example:**

**<table>**
**<tr>**
**<td>Test Dependencies</td>**
**<td>Host Labels</td>**
**</tr>**
**<tr>**
**<td>\[RPM, BT, GOBI3K\]</td>**
**<td>\[RPM, BT, GOBI3K\]</td>**
**</tr>**
**<tr>**
**<td>\[RPM, BT, i5\]</td>**
**<td>\[RPM, i5\]</td>**
**</tr>**
**</table>**

**This algorithm would merge the test deps into \[RPM, BT, GOBI3K, i5\], and determine that choosing both hosts would successfully cover. The problem is that the second test cannot be run in this testbed, as no machine meets all its dependencies.**

**## A dead-stupid first pass (read: what we've implemented so far!)**

**Since we only have tests with a single dependency these days, we’re going to
start by gathering all the deps of all the tests in the suite, de-duping them,
and finding &lt;= N machines to satisfy those deps. We may try to be smart about
how we pad out the machines to reach N, if we can. If we have 10 Gobi3K tests
and 2 Gobi2K tests, and N == 3, then we might try to get 2 Gobi3K devices and 1
Gobi2K, for example. If we can satisfy all tests with &lt; N devices, and cannot
pad to reach N total devices, we’ll WARN, but continue to run the suite.**

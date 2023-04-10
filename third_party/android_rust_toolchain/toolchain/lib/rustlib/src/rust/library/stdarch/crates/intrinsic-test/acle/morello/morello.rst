..
   Copyright (c) 2018-2020, Arm Limited and its affiliates.  All rights reserved.
   CC-BY-SA-4.0 AND Apache-Patent-License
   See LICENSE file for details

.. |release| replace:: 01alpha
.. |date-of-issue| replace:: 02 July 2021
.. |copyright-date| replace:: 2020-2021
.. |footer| replace:: Copyright © |copyright-date|, Arm Limited and its
                      affiliates. All rights reserved.

.. _ACLE-morello: http://github.com/arm-software/acle/morello
.. _ACLE: https://developer.arm.com/documentation/101028/latest
.. _CHERI: https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-947.pdf

***************************************************
Morello Supplement to the Arm C Language Extensions
***************************************************

.. class:: version

|release|

.. class:: issued

Date of Issue: |date-of-issue|

.. class:: logo

.. image:: Arm_logo_blue_RGB.svg
   :scale: 30%

.. section-numbering::

.. raw:: pdf

   PageBreak oneColumn

=========================


Preamble
========

Abstract
--------

This document specifies the Arm C Language Extensions to enable C/C++
programmers to use the Morello architecture with minimal restrictions
on source code portability.

Keywords
--------

Predefined macros, built-in functions

Latest release and defects report
---------------------------------

For the latest release of this document, see the `ACLE project on
GitHub <https://github.com/ARM-software/acle>`_.

Please report defects in this specification to the `issue tracker page
on GitHub <https://github.com/ARM-software/acle/issues>`_.

License
-------

This work is licensed under the Creative Commons
Attribution-ShareAlike 4.0 International License. To view a copy of
this license, visit http://creativecommons.org/licenses/by-sa/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA
94042, USA.

Grant of Patent License. Subject to the terms and conditions of this
license (both the Public License and this Patent License), each
Licensor hereby grants to You a perpetual, worldwide, non-exclusive,
no-charge, royalty-free, irrevocable (except as stated in this
section) patent license to make, have made, use, offer to sell, sell,
import, and otherwise transfer the Licensed Material, where such
license applies only to those patent claims licensable by such
Licensor that are necessarily infringed by their contribution(s) alone
or by combination of their contribution(s) with the Licensed Material
to which such contribution(s) was submitted. If You institute patent
litigation against any entity (including a cross-claim or counterclaim
in a lawsuit) alleging that the Licensed Material or a contribution
incorporated within the Licensed Material constitutes direct or
contributory patent infringement, then any licenses granted to You
under this license for that Licensed Material shall terminate as of
the date such litigation is filed.

About the license
-----------------

As identified more fully in the License_ section, this project
is licensed under CC-BY-SA-4.0 along with an additional patent
license.  The language in the additional patent license is largely
identical to that in Apache-2.0 (specifically, Section 3 of Apache-2.0
as reflected at https://www.apache.org/licenses/LICENSE-2.0) with two
exceptions.

First, several changes were made related to the defined terms so as to
reflect the fact that such defined terms need to align with the
terminology in CC-BY-SA-4.0 rather than Apache-2.0 (e.g., changing
“Work” to “Licensed Material”).

Second, the defensive termination clause was changed such that the
scope of defensive termination applies to “any licenses granted to
You” (rather than “any patent licenses granted to You”).  This change
is intended to help maintain a healthy ecosystem by providing
additional protection to the community against patent litigation
claims.

Contributions
-------------

Contributions to this project are licensed under an inbound=outbound
model such that any such contributions are licensed by the contributor
under the same terms as those in the LICENSE file.

Trademark notice
----------------

The text of and illustrations in this document are licensed by Arm
under a Creative Commons Attribution–Share Alike 4.0 International
license ("CC-BY-SA-4.0”), with an additional clause on patents.
The Arm trademarks featured here are registered trademarks or
trademarks of Arm Limited (or its subsidiaries) in the US and/or
elsewhere. All rights reserved. Please visit
https://www.arm.com/company/policies/trademarks for more information
about Arm’s trademarks.

Copyright
---------

Copyright (c) |copyright-date|, Arm Limited and its affiliates.  All rights
reserved.

About This Document
===================

Change Control
--------------

Current Status and Anticipated Changes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following support level definitions are used by the ACLE specifications:

**Release**
   Arm considers this specification to have enough implementations, which have
   received sufficient testing, to verify that it is correct. The details of these
   criteria are dependent on the scale and complexity of the change over previous
   versions: small, simple changes might only require one implementation, but more
   complex changes require multiple independent implementations, which have been
   rigorously tested for cross-compatibility. Arm anticipates that future changes
   to this specification will be limited to typographical corrections,
   clarifications and compatible extensions.

**Beta**
   Arm considers this specification to be complete, but existing
   implementations do not meet the requirements for confidence in its release
   quality. Arm may need to make incompatible changes if issues emerge from its
   implementation.

**Alpha**
   The content of this specification is a draft, and Arm considers the
   likelihood of future incompatible changes to be significant.

All content in this document is at the **Alpha** quality level.

Change History
^^^^^^^^^^^^^^

.. table:: :align: left

    +-----------+---------------------+--------------------------------+
    | Issue     | Date                | Change                         |
    +-----------+---------------------+--------------------------------+
    | 00alpha   | 30th September 2020 | Alpha release                  |
    +-----------+---------------------+--------------------------------+
    | |release| | |date-of-issue|     | Open source release. NFCI.     |
    +-----------+---------------------+--------------------------------+

References
----------

This document refers to, or is referred to by, the following documents.

.. class:: morello-table-references

.. table:: :align: left

    +---------------+---------------------------------------------------------+-----------------------------------------------------+
    | Ref           | URL or other reference                                  | Title                                               |
    +---------------+---------------------------------------------------------+-----------------------------------------------------+
    | ACLE-morello_ | This document                                           | Morello Supplement to the Arm C Language Extensions |
    +---------------+---------------------------------------------------------+-----------------------------------------------------+
    | ACLE_         | Document number: 101028                                 | Arm C Language Extensions                           |
    +---------------+---------------------------------------------------------+-----------------------------------------------------+
    | CHERI_        | UCAM-CL-TR-947, SSN 1476-2986                           | CHERI C/C++ Programming Guide                       |
    +---------------+---------------------------------------------------------+-----------------------------------------------------+

Terms & Abbreviations
---------------------

Capability
   The capability data type is an unforgeable token of authority which provides
   a foundation for fine grained memory protection and strong compartmentalisation.

Permissions
   The permissions mask controls how the capability can be used - for example, by
   authorizing the loading and storing of data and/or capabilities.

Deriving a capability
   A capability value CV2 is said to be derived from a capability value CV1
   when CV2 is a copy of CV1 with optionally removed permissions and/or
   optionally narrowed bounds (base increased or limit reduced).

Sealing a capability
   When a capability is sealed it cannot be modified or dereferenced,
   but it can be used to implement opaque pointer types.


Scope
=====

The Morello Supplement to the Arm C Language Extensions highlights the language
features added on top of the CHERI programming language to further exploit the
Morello architecture. We recommend reading the CHERI_ Pure-Capability
C/C++ Programming Guide as preliminary material.

Predefined macros
=================

ACLE introduces several predefined macros that define how the C/C++
implementation uses the Morello architecture.

``__ARM_FEATURE_C64``
---------------------
This macro indicates that the code is being compiled for the C64 ISA.

Capability Permissions
----------------------

The following macros indicate capability permissions:

.. table:: :align: left

   +---------------------------------------------+-----------+
   |               **Name**                      | **Value** |
   +---------------------------------------------+-----------+
   |``__ARM_CAP_PERMISSION_EXECUTIVE__``         | 2         |
   +---------------------------------------------+-----------+
   |``__ARM_CAP_PERMISSION_MUTABLE_LOAD__``      | 64        |
   +---------------------------------------------+-----------+
   |``__ARM_CAP_PERMISSION_COMPARTMENT_ID__``    | 128       |
   +---------------------------------------------+-----------+
   |``__ARM_CAP_PERMISSION_BRANCH_SEALED_PAIR__``| 256       |
   +---------------------------------------------+-----------+

Those can be used to form a bitmask that is acceptable for ``cheri_perms_and()``
and ``cheri_perms_clear()``. The value of each macro corresponds to the permission
bit as it appears in the architecture documentation.

Deviation from CHERI
--------------------
The macro ``__CHERI_CAP_PERMISSION_PERMIT_CCALL__`` is not available
on the Morello architecture.


Builtin functions
=================


ACLE standardizes builtin functions to access the Morello architecture.
These are the following:


Check subset and conditionally unseal or return null
----------------------------------------------------

.. code:: c

  void* __capability
  __builtin_morello_subset_test_unseal_or_null(const void* __capability a,
                                               const void* __capability b)

Assuming two valid capabilities ``a`` and ``b``, with the former being sealed
and the latter being unsealed, if ``a`` can be derived from ``b``, then it
unseals ``a`` and returns it, otherwise it returns a null capability.


Check subset and conditionally unseal
-------------------------------------

.. code:: c

  void* __capability
  __builtin_morello_chkssu(const void* __capability a,
                           const void* __capability b)

Assuming two valid capabilities ``a`` and ``b``, with the former being sealed
and the latter being unsealed, if ``a`` can be derived from ``b``, then it
unseals ``a`` and returns it, otherwise it just returns ``a``.


Convert pointer to capability offset (zeroing form)
---------------------------------------------------

.. code:: c

  void* __capability
  __builtin_morello_cvtz(const void* __capability a, size_t b)

If the specified offset ``b`` is zero, then it returns a null capability,
otherwise it sets the offset of capability ``a`` to ``b`` and returns ``a``.
If capability ``a`` is sealed then the returned capability is marked invalid.


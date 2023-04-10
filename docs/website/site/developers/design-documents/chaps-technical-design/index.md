---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: chaps-technical-design
title: Chaps Technical Design
---

[TOC]

## Objective

Chaps is a PKCS #11 implementation that provides trusted platform module (TPM)
backed cryptographic services. It replaces openCryptoki as the provider of
TPM-backed cryptographic services on Chrome OS. It aims to improve speed and
reliability of cryptographic token operations as well as to provide a simpler
and more flexible codebase for future enhancements. Chaps works with a TCG
Software Stack (TSS). On Chrome OS the TrouSerS TSS implementation is used, but
Chaps is not limited to working with TrouSerS. The name "Chaps" has no real
significance other than its fitness as a name for a layer above TrouSerS.

The following diagram illustrates where Chaps fits in the cryptographic services
stack.

[<img alt="image"
src="/developers/design-documents/chaps-technical-design/ChapsStack.png">](/developers/design-documents/chaps-technical-design/ChapsStack.png)

## Rationale

The motivation for creating Chaps and phasing out openCryptoki is threefold:

*   Stability - There have been numerous bugs and quirks that are
            difficult to find and sometimes messy to fix. We want a module that
            is designed to be highly testable and that lends itself to automated
            testing.
*   Performance - TPMs are notoriously slow, and we want to both
            leverage security and optimize performance.
*   Integration - We want a module that integrates well with Chrome OS
            and how Chrome OS uses TPM-backed cryptographic services.
            openCryptoki does not meet this criteria, and some future
            considerations, such as multi-token scenarios, are not possible with
            openCryptoki. We also want code that will be easy to enhance in the
            future.

## Audience

This document is for engineers, system integrators, and security analysts
interested in the details of Chrome OS security.

## Resources

The following resources provide useful background to this document:

*   [TrouSers PKCS #11 Interface
            Docs](http://trousers.sourceforge.net/pkcs11.html): OpenCryptoki is
            currently used with a plugin from the TrouSerS project to provide
            the TPM-specific implementation. This link provides details on the
            plugin and the model it implements.
*   [PKCS #11: Cryptographic Token Interface
            Standard](http://www.rsa.com/rsalabs/node.asp?id=2133)
*   [TCG Software Stack
            Specification](http://www.trustedcomputinggroup.org/files/resource_files/6479CD77-1D09-3519-AD89EAD1BC8C97F0/TSS_1_2_Errata_A-final.pdf)

## Overview

Chaps has two major components:

*   chapsd - A daemon that receives and handles PKCS #11 calls via the
            D-Bus. This daemon is always running and starts at boot.
*   libchaps.so - A library that exports the PKCS #11 interface and
            communicates with chapsd. An application loads and uses this library
            like any other PKCS #11 library.

Chaps reports two slots: (1) a system slot (not currently enabled) and (2) a
user slot. The system slot will always have a token available. The user slot
will have a token available while a user is logged in. chapsd receives
login/logout notifications from cryptohome. The login notification must provide
the user password (or some password-derived value) that will be used as
authorization data for TPM-protected keys. Cryptohome derives the authorization
data from the user password and a user-and-system-specific salt value using the
scrypt key derivation scheme.

This diagram shows the Chaps high-level architecture:

[<img alt="image"
src="/developers/design-documents/chaps-technical-design/ChapsArchitecture.png">](/developers/design-documents/chaps-technical-design/ChapsArchitecture.png)

### Staging

*   Phase 1 (enabled in Chrome OS version 18) - Introduce chapsd:
    *   Marshaling of all PKCS #11 calls from libchaps.so to chapsd.
    *   chapsd redirection to openCryptoki.
    *   Integration of libchaps.so with all PKCS #11 applications.
    *   chapsd runs as chronos to play nice with existing openCryptoki
                files and facilitate a rollback in the event of unexpected
                problems.
*   Phase 2 (enabled in Chrome OS version 20) - Replace openCryptoki:
    *   chapsd implementation (redirection to openCryptoki is removed).
    *   chapsd runs as a system user named "chaps."
    *   OpenCryptoki is removed from the system.
*   Phase 3 (optional) - Do more in software:
    *   TPM-encrypted key store for faster RSA operations. A
                TPM-protected symmetric key is used to encrypt a key store
                outside of the TPM. This symmetric key will be much faster to
                load than RSA keys and can be temporarily cached. The RSA keys
                are then stored in the key store and the cryptographic
                operations are performed in software.

## Infrastructures

### Inter-Process Communication (IPC)

All IPC with chapsd is performed using D-Bus. Both the Chaps client and daemon
use dbus-c++ to generate proxy and adaptor classes.

### Persistence

Object data is stored in /var/lib/chaps for the system token and in ~/.chaps for
each user. The object database is implemented using LevelDB.

### TPM Software Layers

The TrouSers library is used to access TPM services from the TSPI layer.

### Testing

Unit tests for Chaps use the gtest and gmock frameworks. End-to-end tests use
the autotest framework.

## Detailed Design

### TPM Interaction

#### Phase 1 - Introduce chapsd:

In Phase 1 all TPM interaction is still controlled by openCryptoki. The
introduction of chapsd does not change TPM interaction in any way.

#### Phase 2 - Replace openCryptoki:

All RSA private keys with the attribute CKA_TOKEN set to TRUE will be bound to
the TPM (wrapped by the Storage Root Key) and will not be extractable. All other
private data and non-RSA keys are stored encrypted by the User Encryption Key
which is protected by the TPM’s SRK but is extractable. This diagram depicts the
key hierarchy:

[<img alt="image"
src="/developers/design-documents/chaps-technical-design/KeyHierarchy.png">](/developers/design-documents/chaps-technical-design/KeyHierarchy.png)

##### Authorization Data

Authorization data for the User Authorization Key is derived from the user
password using the scrypt scheme. Salt is stored in a root-accessible file in
the user’s home directory and is unique per user account per system. This
ensures that authorization data for the same user on two different systems is
different. The scrypt parameters are tuned to perform the computation in about
100ms.

All other authorization data is randomly generated and stored encrypted with the
User Encryption Key. This authorization data is not directly encrypted with the
User Authorization Key for performance reasons. This does not reduce security
since any attack on the key would require access to the TPM (and so decryption
of authorization data by the TPM would be possible).

##### Migration

Existing TPM-protected keys cannot be migrated to this hierarchy. The existing
hierarchy is supported so that existing keys can continue to be used. All other
data including meta-data associated with TPM-protected keys that was previously
stored in the openCryptoki object store can be migrated to the new persistent
object store. This migration will take place when valid openCryptoki object data
is found in a user’s home directory and migration has not previously taken
place.

##### Multiple Users

In the future, simultaneous multiple user logins may need to be supported, and
this design must accommodate this scenario. Each slot maintains a distinct
object database, a distinct User Authorization Key and User Encryption Key, and
uses a distinct password. Currently, only the "system" and "user" slots exist,
but adding a new user would be as simple as adding another slot.

#### Phase 3 - Do more in software:

The key hierarchy for Phase 3 is the same as for Phase 2. The difference is that
in Phase 3 some RSA keys can be protected by the User Encryption Key rather than
being directly TPM-protected. Keys that require higher performance and do not
require the high security of direct TPM binding are candidates for this. There
is no migration of existing RSA keys because they are not extractable.

### Chaps Classes

Chaps is implemented in C++ and makes wide use of mockable interfaces. This
structure allows for a high level of unit test coverage. In general, all classes
inherit from an interface class, and when one class interacts with another, it
references the interface, not the implementation class.

*   ChapsInterface - This interface is the C++ equivalent of the IPC
            contract between the Chaps client and daemon. The actual interface
            used by dbus-c++ is described in XML. ChapsInterface is implemented
            on the client side by ChapsProxyImpl and on the daemon side by
            ChapsServiceRedirect (Phase 1) and ChapsServiceImpl (Phase 2). The
            following diagram shows the class hierarchy and how the classes
            interact in practice.

[<img alt="image"
src="/developers/design-documents/chaps-technical-design/ClassHierarchy.png">](/developers/design-documents/chaps-technical-design/ClassHierarchy.png)

*   ChapsProxyImpl - This class implements ChapsInterface and forwards
            all calls to an instance of the dbus-c++ generated proxy class.
*   ChapsAdaptor - This class implements the dbus-c++ generated adaptor
            interface and forwards all dbus calls to a ChapsInterface instance.
*   ChapsServiceRedirect - This class implements ChapsInterface and
            forwards all calls to a PKCS #11 library. In Phase 1 this class is
            hooked up to ChapsAdaptor and forwards incoming calls to
            openCryptoki. In Phase 2 this class will no longer be used.
*   ChapsServiceImpl - This class implements ChapsInterface and provides
            the actual Chaps implementation (in contrast to
            ChapsServiceRedirect). This class is hooked up to ChapsAdaptor (in
            Phase 2) and serves as the entry point of all calls to chapsd.
*   ChapsFactory - This class creates default instances of interface
            classes. In a test environment, a mock of this class can be
            configured to create the desired combination of real and mock
            objects.
*   SlotManager / SlotManagerImpl - This mockable class manages the
            state of all slots, the tokens in them, and maintains a list of
            sessions for each token.
*   Session / SessionImpl - This mockable class represents a session and
            manages all contextual information for a single session.
*   ObjectPolicy - An instance of this class is bound to each object; it
            enforces all policies specified in PKCS #11 for the particular
            object type. There is a subclass of this interface for each object
            type.
*   Object / ObjectImpl - This mockable class represents an object and
            is optionally persistent.
*   ObjectPool / ObjectPoolImpl - This mockable class represents an
            unordered group of objects. An ObjectPool may or may not be backed
            by an ObjectStore.
*   ObjectStore / ObjectStoreImpl - This mockable class implements a
            persistent object store. This is the only class that references the
            LevelDB API.
*   TPMUtility / TPMUtilityImpl - This mockable class interfaces with
            the TrouSerS library for interaction with the TPM.
*   ObjectImporter / OpencryptokiImporter - This mockable class imports
            objects from an external source. The openCryptoki-specific
            implementation imports objects from an openCryptoki database.

### **Marshalling**

All marshalling is performed by dbus-c++ with the exception of template
arguments (that is, CK_ATTRIBUTE lists), which are serialized using the protobuf
library.

### Schema

LevelDB is not relational, so a relational schema is not used. Objects are
serialized and assigned a unique identifier. The keys in LevelDB are the object
identifiers, and the values are serialized object blobs.

### Deployment

#### Phase 1

Phase 1 deployment has the following goals:

*   The PKCS #11 system is fully functional following each individual
            CL. This is necessary because multiple repositories are involved
            (for example, chaps, cryptohome, chrome, init, …).
*   A full rollback is possible at any time.

The following ordered deployment plan meets these goals:

1.  Both chapsd and libchaps.so are installed and fully functional.
            chapsd runs as chronos:pkcs11 in order to play nice with other
            applications loading openCryptoki directly.
2.  An upstart configuration file is added for chapsd so it starts
            before login and respawns automatically. It depends on tcsd and
            dbus.
3.  Chaps is added as a run-time dependency to the cryptohome ebuild.
4.  The cryptohome upstart configuration is modified to depend on
            chapsd.
5.  Cryptohome is modified to link against Chaps rather than
            openCryptoki.
6.  All other modules that use openCryptoki are modified (in any order)
            to use Chaps. This includes but is not limited to:
    1.  NSS in Chrome
    2.  vpn_manager
    3.  flimflam
    4.  wpa_supplicant
    5.  entd

#### Phase 2

Phase 2 deployment has the following goals:

*   The PKCS #11 system is fully functional at all times.
*   Migration is seamless to all users.
*   A partial rollback is possible for a limited time that will put a
            migrated user token back in the state it was in before migration.
            Modifications since migration will be lost; this works like
            restoring a backup.

The following deployment plan meets these goals:

1.  When a token is migrated, all existing files are left intact and a
            database record is created to indicate that migration has taken
            place. Upon rollback, openCryptoki simply keeps working with these
            files as it did before migration.
2.  The switch is in the chapsd upstart configuration: the target
            library argument is removed to enable Phase 2 logic and is re-added
            in order to roll back.
3.  Finally, once confidence is established in the Phase 2 logic,
            cleanup tasks can be performed that preclude future rollbacks. These
            include:
    1.  Remove all openCryptoki-related files in the user token
                directory.
    2.  Remove all openCryptoki-related logic from cryptohome code.
    3.  Remove openCryptoki from the build.

## Project Information

The Chaps code resides in a Chromium OS git repository at
<http://git.chromium.org/gitweb/?p=chromiumos/platform/chaps.git;a=summary>.

## Internationalization and Localization

The Chaps library returns the following strings which may need to be translated.
It is assumed that manufacturer ID strings do not require translation. If a
string does not appear in the Chrome OS user interface, it may not require
translation.

*   C_GetInfo returns a library description.
*   C_GetSlotInfo returns a slot description.
*   C_GetTokenInfo returns a token label, token model, and token
            description.

## Security Considerations

The following table lists some threats that are mitigated by the Chaps system
and provides details on how the threat is mitigated.

<table>
<tr>
<td>Threat</td>
<td>Mitigation</td>
</tr>
<tr>
<td>Attacker gains access to a user’s cryptographic token services (for example, creates an unauthorized digital signature).</td>
<td>chapsd only allows connections from authorized uids (chronos, root) using D-Bus policies.</td>
</tr>
<tr>
<td>Attacker owns D-Bus identifier and spoofs cryptographic services.</td>
<td>D-Bus policy only allows the "chaps" user to own the chapsd identifier.</td>
</tr>
<tr>
<td>Attacker gains access to private data and keys by accessing run-time memory of the chapsd process.</td>
<td>chapsd runs as the "chaps" user.</td>
</tr>
<tr>
<td>Attacker gains access to private data and keys by accessing persistent data stored by chapsd.</td>
<td>All private data and keys stored on disk are encrypted with the User Encryption Key and stored in the cryptohome partition.</td>
</tr>
<tr>
<td>Attacker gains access to a user’s TPM-protected resources.</td>
<td>TPM-protected resources are authorized by a secret derived from the user’s password and a root-accessible random salt value.</td>
</tr>
<tr>
<td>Attacker stages Chaps persistent data in order to leverage a vulnerability in chapsd to gain control of chapsd.</td>
<td>Chaps persistent data is modifiable only by the "chaps" user (<b>The data needs to be stored in a root-owned directory for this policy to be effective. See <a href="https://crbug.com/212630">crbug.com/212630</a>).</b> </td>
</tr>
</table>

If a user is logged in and an attacker gains chapsd privilege, then nothing
prevents this attacker from reading all private data and keys that are not
TPM-protected and non-extractable or from creating digital signatures using
TPM-protected, non-extractable keys. The following are TPM-protected:

*   RSA private keys (with modulus 2048 bits or less) that have
            CKA_TOKEN=TRUE.

All objects with CKA_PRIVATE=TRUE are encrypted on disk with a TPM-protected
key, even if they are not themselves TPM-protected. The following are not
themselves TPM-protected (other than the on-disk encryption):

*   RSA private keys with modulus larger than 2048 bits.
*   RSA private keys with CKA_TOKEN=FALSE.
*   All objects that are not RSA private keys (for example, AES keys,
            data objects).

### Key Storage Example

Suppose you have a certificate with an associated private key used to negotiate
802.1x and/or VPN connections. The following PKCS #11 objects would exist in the
Chaps database:

*   Certificate: This object is public and is not encrypted.
*   Public Key: This object is public and is not encrypted.
*   Private Key: This object is private and is encrypted with the User
            Encryption Key. It holds the following attributes:
    *   Authorization Data: This is random and specific to this key.
    *   Private Key Blob: This is a TPM-wrapped blob.
*   Encrypted User Encryption Key: This is encrypted by the User
            Authorization Key.
*   User Authorization Key Blob: This is a TPM-wrapped blob.

The following information would be in chapsd process memory while the token is
loaded:

*   User Encryption Key: This has been decrypted using the User
            Authorization Key and the password-derived secret as authorization
            data.
*   The Certificate, Public Key, and Private Key, as listed above. The
            Private Key has been decrypted using the User Encryption Key.

### Cryptography

This section lists each way in which Chaps uses cryptography and gives details
as to how it is used. In all cases where cryptography is used in software, Chaps
uses the openssl crypto library to provide the underlying algorithm
implementations.

#### Random Numbers

The openssl cryptographically strong pseudo-random-number-generator (PRNG) is
used as a source of random numbers. It is seeded using the default openssl
seeding mechanism (/dev/urandom on Chrome OS) in combination with 128 random
bytes from the TPM. This seeding occurs only once per chapsd process.

#### PKCS #11 Software Mechanisms

Chaps supports a number of mechanisms in software that are not supported by the
TPM (for example, SHA, HMAC, AES). The implementation of these mechanisms pass
through directly to openssl, including padding and cipher modes, with the
following exceptions:

*   The construction of RSA signing data. Chaps constructs the signing
            data itself and then employs the openssl RSA primitive. This
            approach guarantees signature compatibility between openssl and a
            TPM.
*   The generation of DES / DES3 keys. Chaps is still using openssl
            functions such as DES_is_weak_key() and the openssl PRNG.

#### Encryption / Authentication of Persistent Objects

Private object blobs are encrypted using AES-256-CBC with PKCS padding. An
initialization vector (IV) is randomly generated for each encryption operation;
it is appended to the encrypted blob. An HMAC-SHA512 mac is appended to each
encrypted blob. This mac is verified before the decryption is attempted.
Verification uses a version of memcmp that reads and compares all bytes
regardless of where a mismatch is found.

#### Authorization Data Hashes

Once authorization data is derived from the user password or similar using
scrypt, it is processed as follows:

*   An SHA-1 hash of the authorization data is computed and sent to the
            TPM. This hash is never stored on disk and is considered as
            sensitive as the user password.
*   An SHA-512 hash of the authorization data is computed and the first
            byte of the hash is stored in the token database. This allows Chaps
            to sanity-check incoming authorization data before sending it to the
            TPM. Only a single byte is stored because it allows a reasonable
            sanity check but it is not very useful for a brute-force attack
            against the authorization data or the user password.

## Logging Plan

All errors are logged using the facility provided by base/logging.h.

## Testing Plan

Chaps is tested using both unit and integration tests. All unit tests are run
from the ebuild test phase. The integration tests can be run manually or with
the autotest framework.

### Unit / Integration Tests

The classes generated by dbus-c++ are not suitable for stubbing / mocking for
two reasons:

*   We don’t want dbus code running in the unit test environment.
            Problems with this approach have been noted on other projects.
*   The classes are tightly coupled with types and conventions that are
            specific to dbus-c++.

A C++ interface is defined (ChapsInterface) that is very close to the generated
proxy and adaptor interfaces but suitable for using and mocking in a test
environment.

#### Client-side Unit Tests

The client-side code is unit tested by using a mock proxy and calling the PKCS
#11 C interface. Because the code being tested is called by external code, all
argument validation and internal state validation is tested thoroughly resulting
in a high level of code coverage.

#### Daemon-side Unit / Integration Tests

Phase 1: There are no unit tests per se for the chapsd code in Phase 1 because
each layer is either tightly coupled with D-Bus or tightly coupled with
openCryptoki. The chapsd code is tested by a suite that verifies basic
functionality of ChapsInterface methods. This test suite is then run on both a
ChapsServiceRedirect instance and on a ChapsProxyImpl instance. These tests
require a configured openCryptoki and, in the ChapsProxyImpl case, a running
chapsd instance. Because of this requirement, the tests must be run on a live
system, and they do not run as part of the ebuild test phase.

Phase 2: The tests from Phase 1 are reused and are run on a ChapsServiceImpl
instance and a ChapsProxyImpl instance. In the case of ChapsServiceImpl, a mock
persistence object can be used, and the suite can run as a unit test. In the
case of ChapsProxyImpl, the suite is run as an integration test and requires a
running chapsd instance as in Phase 1. In addition to this, unit tests which are
specific to a single class are run for the following classes:

*   SlotManager
*   Session
*   ObjectPolicy
*   Object
*   ObjectPool
*   ObjectStore
*   TPMUtility

For each of these unit tests, any references to other objects are mocks. The
following mocks are available:

*   ChapsFactoryMock
*   SlotManagerMock
*   SessionMock
*   ObjectPolicyMock
*   ObjectMock
*   ObjectPoolMock
*   ObjectStoreMock
*   ObjectImporterMock
*   TPMUtilityMock

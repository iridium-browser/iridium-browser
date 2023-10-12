---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: tpm-usage
title: TPM Usage
---

[TOC]

## Introduction

This document describes the usage of a Trusted Platform Module (TPM) in Chrome
devices (Chromebook or other form factors), including firmware, operating
system, and applications. It assumes some knowledge of Chrome OS concepts as
well as TPM functions. It may be of interest to Chromium OS developers who want
to use the TPM and to users who wish to understand the usage of the TPM in
Chrome OS.

Chrome OS uses the TPM for these tasks:

*   Preventing software and firmware version rollback
*   Maintaining information to detect transitions between normal and
            developer modes
*   Protecting user data encryption keys
*   Protecting certain user RSA keys (‘hardware-backed’ certificates)
*   Providing tamper evidence for installation attributes
*   Protecting stateful partition encryption keys
*   Attesting TPM-protected keys
*   Attesting device mode

The TPM is not directly available outside of Chrome OS for any purpose; that is,
no remote computer has access to the TPM.

Chrome OS does not use the TPM for the following:

*   Trusted boot - the TPM is not used as part of the Chrome OS verified
            boot solution.
*   Hardware-strength platform configuration reporting. See [Attesting
            Device
            Mode](/developers/design-documents/tpm-usage#TOC-Attesting-Device-Mode)
            for more details.
*   Whole-disk encryption or similar. In particular, the TPM is not used
            to unwrap an encryption key during the boot process.

The rest of this document first discusses the four different modes of operation
of Chrome devices; then it describes how Chrome OS controls TPM ownership; and
finally it presents each area of TPM usage in detail.

## Modes of Operation

A Chrome device can be booted in four different modes, corresponding to the
settings of two switches (physical or virtual) at power on. They are the
developer switch and the recovery switch. They may be physically present on the
device, or they may be virtual, in which case they are triggered by certain key
presses at power on. When both switches are off, the boot is called normal mode
boot. When the developer switch is on, it is called developer mode boot. When
the recovery switch is on, it is called recovery mode boot (and normal-recovery
or developer-recovery when there is a need to distinguish them).

These modes give users a choice between a high degree of security or complete
control over the device. In normal mode, the device is running a Google-provided
copy of Chrome OS, which cannot be altered (assuming the hardware has not been
tampered with). In developer mode, users can run a modified copy of Chromium OS
(or any other supported operating system), though without some of the Chrome OS
security defenses. The recovery modes allow for installation of Chrome OS or
Chromium OS from recovery media.

## TPM Ownership and Restrictions

Most of the TPM functionality becomes available after a TPM owner is
established. In normal mode, Chrome OS attempts to establish a TPM owner with a
random password, which is generated only after the owner of the Chrome device
starts using it. When the owner password is created, there is a period of time
in which the user can find out what it is and write it down. After this period,
the password is destroyed. However, knowledge of the owner password is not
necessary at any point in Chrome OS.

Under certain conditions, the TPM owner will be cleared, rendering keys
currently protected by the TPM useless (and therefore the data protected by
those keys unrecoverable). These conditions are as follows:

*   On the first power-on after switching to developer mode, the TPM is
            cleared by the firmware before the OS kernel begins booting.
*   On the first power-on after switching to normal mode, the TPM is
            also cleared by the firmware before the OS kernel begins booting.
*   During a recovery mode boot in normal mode, a Chrome OS recovery
            image will clear the TPM before the device is rebooted.
*   Some Chrome devices allow the system (or, if in developer mode, the
            user) to explicitly request that the TPM owner be cleared on the
            next reboot.

When a non-Chrome OS image is booted in developer mode, it is up to that
user-installed OS to decide whether or not to take ownership, or do anything at
all with the TPM. The user-installed system, for example, may create additional
TPM NVRAM spaces other than those that Chrome OS creates (e.g., see [Rollback
Prevention](/developers/design-documents/tpm-usage#TOC-Rollback-Prevention)
below).

Note the following developer mode restrictions on the TPM:

*   The TPM physical presence command is disabled by the read-write
            firmware on every boot. This means that physical presence cannot be
            asserted even by a custom OS.
*   The NVRAM firmware space (see [Rollback
            Prevention](/developers/design-documents/tpm-usage#TOC-Rollback-Prevention)
            below) cannot be removed.
*   The NVRAM kernel space (see [Rollback
            Prevention](/developers/design-documents/tpm-usage#TOC-Rollback-Prevention)
            below) can be removed, but doing so will result in the firmware
            forcing recovery mode at the next boot.

In the event that the NVRAM kernel space is removed, the device will only boot a
Google-provided recovery image, which will try to reconstruct that space. Chrome
OS recovery will aggressively destroy other spaces as needed to make room.

## Rollback Prevention

The normal boot process of a Chrome device follows a chain of trust, in the
following order:

1.  Read-only section of firmware (set during factory installation and
            unchangeable in software)
2.  Upgradable section of firmware (called read-write firmware)
3.  Kernel
4.  Programs and services that comprise the operating system

Each link in the chain is responsible for verifying that the next link has not
been tampered with before yielding control to it.

Read-write firmware and kernel can be updated, either manually (through a
“recovery” process) or automatically. For security, the automatic update process
does not allow updating to versions of the software that are older than the
current one. This prevents “remote rollback attacks,” in which a remote attacker
replaces newer software, through a hard to exploit vulnerability, with older
software, with a well-known and easy to exploit vulnerability.

To prevent remote rollback attacks, Chrome devices reserve two small NVRAM
spaces in the TPM to store version numbers and other information. These NVRAM
spaces are called “firmware space” and “kernel space.” They are created during
factory initialization and are never removed. (The kernel space can be removed
in developer mode, but the firmware space cannot. Also note that a Chrome OS
recovery image will try to recreate the kernel space, possibly removing other
spaces to make room.)

The firmware space contains the read-write firmware version number (among other
things). It is created with permission TPM_NV_PER_GLOBALLOCK |
TPM_NV_PER_PPWRITE, and is locked by setting the TPM bGlobalLock bit. On a
normal boot, this space is locked by the read-only firmware, possibly after
updating it to reflect the version of the verified read-write firmware
installed. As mentioned, this update may only replace the current version number
with a larger one.

The kernel space contains the kernel version number. It is created with
permission TPM_NV_PER_PPWRITE. On a normal boot it is locked by turning off and
locking physical presence in the read-write firmware. As in the case of the
firmware space, the read-write firmware will update this number to the version
number of the current kernel.

On a recovery boot (either in normal or developer mode), neither the firmware
nor the kernel space is locked. Therefore the recovery image can bypass the
anti-rollback mechanism. Bypassing the anti-rollback mechanism is allowed
because there are legitimate cases in which it is desirable to install older
versions of the system, as long as this rollback does not happen without the
knowledge of the user.

## Protecting User Data Encryption Keys

User data in Chrome OS is encrypted when stored on the disk using the eCryptfs
stacked filesystem. When a user logs in for the first time, two random 128-bit
AES keys are generated. One key is used by eCryptfs to encrypt file names, and
the other is used to encrypt file content. These keys are managed locally; they
are not escrowed outside of the Chrome device. This allows users to log in and
access their data while offline and also means that the keys never need to leave
the device. However, this feature also necessitates a strong method of
protecting these keys from disclosure as they are stored in a persistent file on
disk.

Chrome OS uses the TPM to make parallelized attacks and password brute-forcing
difficult. One feature and one characteristic of the TPM are exploited here.
First, the TPM provides secure key storage for RSA keys. This means that the
private key only exists in plain text while it resides on the TPM itself--it can
only be stored outside of the TPM in encrypted form. This feature makes
parallelizing difficult: decrypt operations involving that key must happen on
the TPM itself (unless a vulnerability exists whereby the attacker can obtain
the plain-text private key of a TPM-wrapped RSA key). Second, the TPM is a
relatively slow device. Private key operations can take over half a second to
complete; this provides a level of brute-force protection by effectively
throttling the rate at which guesses can be made.

To protect the user keys, Chrome OS creates a system-wide RSA key wrapped by the
TPM’s Storage Root Key (SRK) on first boot. When storing a particular user’s AES
keyset, Chrome OS encrypts the keyset using a random symmetric key. That
symmetric key is encrypted using the system-wide RSA key. To tie the decryption
to a user secret, 128 bits of this encrypted blob are encrypted a second time
using a key derived from the user’s login password. Since this step is done
without padding, any decryption must blindly decrypt those 128 bits and then
decrypt the entire blob on the TPM before knowing if the decryption process (and
therefore the password) was correct. Using this method (over per-user TPM keys
requiring authentication) has the benefit of avoiding the TPM’s dictionary
attack defense, which is overly aggressive for use during user login.

There are two important implications to the use of the TPM to protect user data:

*   If the TPM is cleared, user data protected by it cannot be
            recovered. In this case, Chrome OS removes user data.
*   If the TPM is somehow disabled or faults, user data cannot be
            accessed. Chrome OS will fail logins for existing users until the
            TPM is enabled.

## Protecting Certain User RSA Keys

Chrome OS implements TPM-backed cryptographic services via the Chaps PKCS #11
component. (More information about Chaps can be found in the [Chaps Technical
Design](/developers/design-documents/chaps-technical-design).) RSA private keys
generated or imported into a Chaps PKCS #11 token with a modulus size supported
by the TPM (typically up to 2048 bits) will be wrapped by the TPM Storage Root
Key (SRK). Each user account on the device has a distinct token. A user can
import a certificate and private key into his TPM-backed token by using the
‘Import and Bind’ operation available in Chrome’s certificate manager. Also, any
keys generated using the Webkit ‘keygen’ tag will use the TPM-backed token by
default.

RSA private keys that are not supported by the TPM and all other PKCS #11 data
(certificates, public keys, data objects, TPM-encrypted key blobs, etc.) are
encrypted with a symmetric key and stored in /home/chronos/user/.chaps. This
symmetric key is itself encrypted using a TPM-backed key which requires a value
partially derived from the user password for authorization. Thus, a user’s Chaps
token can only be decrypted if both the TPM and the user password are available.

## Tamper-Evident Installation Attributes

The first time a device is used, a set of installation attributes is stored on
the device and remains tamper-evident for the remainder of the install (i.e.,
until the device mode changes). If a device has been enterprise enrolled, as
evidenced by a ribbon with text like “This device is owned by yourcompany.com,”
then the installation attributes correspond to this enrollment. If not, the set
of attributes is empty. Either way, the tamper-evidence for the set of
attributes is implemented by computing a salted hash of the attributes and
storing the random salt and the hash in TPM NVRAM with TPM_NV_PER_WRITE_DEFINE
permissions (i.e. read-only until destroyed by the TPM owner). The hash can then
be verified at any time to ensure the attributes have not been modified. This
tamper-evident container is referred to as the “lockbox.”

## Stateful Partition Encryption

Some parts (e.g., /var, /home/chronos) of the stateful partition are encrypted
using dm-crypt. The encryption key is randomly chosen via the TPM’s RNG on first
boot and is encrypted by a TPM-held “system key” (read from TPM NVRAM during
startup). This system key is actually the random salt used for hashing
installation attributes.

## Attesting TPM-Protected Keys

If an RSA private key has been generated in the TPM and has always been
non-migratable, then the key may be certified by a key that has been verified as
an Attestation Identity Key (AIK). No key, including any AIK, is certified
unless the user or device-owner has consented to remote attestation of his or
her device. A certified key credential gives very strong assurance that the key
is protected by a Chrome Device TPM.

## Attesting Device Mode

At boot time, the read-only firmware extends TPM PCR0 with the status of the
developer and recovery mode switches. The value of PCR0 can later be quoted
using a key that has been verified as an Attestation Identity Key (AIK). The
quote, in combination with the AIK credential, gives assurance that the reported
PCR0 value is accurate. While assurance of the PCR0 value is very strong,
assurance that this correctly reflects the device mode is weaker because of the
reliance on read-only firmware to extend PCR0. It is nonetheless useful for
reporting policy compliance. This PCR0 quote is not available outside of Chrome
OS unless the user or device-owner has consented to remote attestation of the
device.

## Chrome OS-Specific TPM Configuration

Under Chrome OS, there are a few configuration settings for the TPM that may
differ from other operating systems:

*   TPM ownership, as discussed above, is automatic and random. While
            the user may choose to write the random owner password down, it is
            never used. Rather, TPM ownership is established merely to enable
            the cryptographic features (such as using keys wrapped directly or
            indirectly by the SRK) that Chrome OS uses in the TPM.
*   The Storage Root Key (SRK) is unrestricted so that it can be used
            without the owner password. Since the TPM is used as a generic
            cryptographic device, and Chrome OS manages clearing the TPM in
            firmware as necessary, unrestricted use of the SRK is allowable.
*   In Chrome OS, the TPM is only used locally on the Chrome device.
            Chrome OS uses TrouSerS for communication with the TPM, which has
            been patched to use a UNIX domain socket instead of a TCP socket.
*   Under Chrome OS, physical presence is disabled on every boot by the
            read-write firmware.

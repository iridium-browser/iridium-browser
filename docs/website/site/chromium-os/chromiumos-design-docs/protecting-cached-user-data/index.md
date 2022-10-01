---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: protecting-cached-user-data
title: Protecting Cached User Data
---

[TOC]

## Abstract

*   Chromium OS devices are intended to be both portable and safely
            shared. As a result, privacy protection for user data stored on the
            local disk is a requirement for a Chromium-based OS.
*   Privacy protection for user data stored on a local disk is
            accomplished via system-level encryption of users' home directories.
*   Chromium OS uses the [eCryptfs](https://launchpad.net/ecryptfs)
            stacked filesystem with per-user vault directories and keysets to
            separate and protect each user’s cached data.
*   Chromium OS devices may use the Trusted Platform Module (TPM) to
            protect against brute-force attempts to recover a user’s keyset (and
            therefore the data it protects).

## Problem: Multiple users, portable devices, locally stored data

Chromium OS should provide privacy protection for user data stored on the local
disk. In particular, some subset of a user's email, pictures, and even HTTP
cookies will be stored on the local drive to enhance the online experience and
ensure Chrome notebooks are useful even when an Internet connection isn't
available. Without this protection, anyone who has physical access to the
Chromium OS device would be able to access any data cached on it.

Normally, data stored in the cloud is protected by privacy-guarding mechanisms
provided by the servers that host it. When data is pulled out of the cloud, it
moves outside of the reach of those server-provided protections. Even though
this is the case today with most computers, there are two areas that make this
even more important to Chromium OS:

*   With a Chromium OS device, users shouldn't have to worry about
            whether other users of the same device can see their cached data.
            For example, if my friend wants to log in to my device to check her
            mail, it's not a big deal. She won't be able to access my data, and
            even though she used my device, I won't be able to access her data.
*   Chromium OS devices are portable. The risk with very portable
            devices is that they are easy to misplace or leave behind. We don't
            want the person who finds a device to instantly have access to every
            users’ websites, emails, pictures, and so on.

## Solution: Encryption

Since Chromium OS is built on Linux, we can leverage existing solutions for
[encrypting](http://en.wikipedia.org/wiki/Disk_encryption_theory) the user's
data at the underlying operating system level and make sure it is automatic and
mandatory. Given the available implementation choices, many possible approaches
were considered (see [Appendix
B](/chromium-os/chromiumos-design-docs/protecting-cached-user-data#Appendix_D_Designs_considered)
for details). Of those approaches, we chose a solution that provides file
system-level encryption of [home
directories](http://en.wikipedia.org/wiki/Home_directory) for each user on a
device.

Encrypting each user's home directory means that all of the data stored and
cached while browsing will be encrypted by default. In addition, data created
and maintained by plugins, like [Adobe
Flash](http://www.adobe.com/products/flash/), will be transparently encrypted
without developers or users needing to take any special action. The same goes
for any data stored by HTML5-enabled web applications.

Existing Operating Systems rely on file ownership and access permissions to
prevent two users of a system from accessing each other’s files. However, the
root or administrative user typically can still access any of these files, and
the files are really only protected as long as that Operating System instance is
booted. Mounting the drive as a secondary disk would allow full access.
Encryption is a better solution to this problem--even when the device is in the
hands of a malicious individual, that person would still have to recover the
encryption keys to be able to access the cached data.

## How encryption works

In a nutshell, each user gets a unique “vault” directory and keyset that is
created at her first login. The vault is used as the underlying encrypted
storage for her data. The keyset is tied to her login credentials and is
required to allow the system to both retrieve and store information in the
vault. The vault is opened transparently to the user at login. On logout or
reboot, the user's data is locked away again.

### Details

Each user is allocated a vault directory that contains the encrypted backing
storage for their home directory. The vault is located in a directory specific
to the user using a hash of the user name with a salt value, and is mounted
using eCryptfs at login to the user directory. A user-specific file encryption
key (FEK) and file name encryption key (FNEK) allow eCryptfs to transparently
encrypt and decrypt while the vault is mounted. A successful mount of the vault
is a required step in the login process of Chromium OS. On first login,
/etc/skel/ is copied to the new home directory, and then it's ready for use.

The FEK and FNEK (together, with some additional data, called the keyset) for a
user are created using the randomness generator provided by the kernel, which,
on Chromium OS devices, is given additional entropy from the TPM at boot. These
128-bit AES keys are used to encrypt the file contents and file names,
respectively. As a stacked filesystem, eCryptfs has a lower file (the encrypted,
persistent file located in the vault and stored on disk) for each upper file
(the plaintext file presented at the mount point, which is never stored directly
to disk). The keys exist in kernel memory for the lifetime of the mount.

A user’s keyset never changes unless their account is removed from the Chromium
OS device, or if the system is erased, such as when a recovery device is used.
It must be protected from disclosure and only available with valid login
credentials. To achieve this, Chromium OS uses one of two methods to store the
keyset, depending on whether a TPM is available and initialized. The preferred
method involves using the TPM, with the fallback method using the
[Scrypt](http://www.tarsnap.com/scrypt.html) encryption library. Both methods
tie the user’s vault keyset to their Google Accounts password, and are described
further below.

All of this is managed by one daemon, cryptohomed, which listens for incoming
requests from the login manager. If the request is to mount an encrypted vault,
the user name and weak password hash must be included. If the request is to
unmount the image, cryptohomed clears the keys stored by the kernel and unmounts
the vault.

### Performance concerns

Adding cryptography to a filesystem comes at a cost--encryption consumes both
CPU cycles and additional memory pages. The latter is primarily managed the the
Linux page cache, but the former can result in reads and writes being CPU-bound
rather than I/O-bound. Based on testing compared to I/O direct to the solid
state disk, we’ve found the current overhead to be acceptable, though there is
always room to be better. Chromium OS uses encryption to protect cached user
data, which does include some frequently used items such as the browser cache
and databases, but does not include shared Operating System files and programs.
So while there is a performance cost, it does not apply to all disk I/O.

## Reclaiming lost space

Being a stacked filesystem, eCryptfs has a side effect of having a single lower
file for every upper file. This allows for “passthrough”--where a file or
directory is created as plaintext in the underlying vault folder, and shows up
in the upper mount point. Generally, this is not a good idea for files (we
wouldn’t want to store plaintext data on disk), but for directories, it has a
nice benefit of allowing offline file cleanup.

Specifically, passthrough directories allow a plaintext directory name to show
up in the vault, but the directory contents (files, subdirectories, etc.) are
still encrypted. Chromium OS can create passthrough directories for discardable
information, such as the browser cache. Since the name is a standard location,
reclaiming disk space when it is low is simply a matter of removing the known
cache directory names from each vault that exists on the system. Without
passthrough directories, each vault would have to be mounted (requiring each
user’s credentials) to be able to identify the cache directory and delete it.

## Managing encryption keys

As mentioned earlier, the user’s vault keyset is ultimately protected by a
partial cryptographic hash of the user's passphrase. A partial cryptographic
hash is used because the user's Google Accounts password must not be exposed to
offline attack. Instead, we're willing to lose some of the entropy supplied by
the passphrase by performing a SHA-256 digest of a user-specific salt
concatenated with the user's passphrase. The first 128 bits of the digest are
used as the user's "weak password hash."

Often, user passwords will only contain anywhere from [18 bits to 30 bits of
entropy](http://en.wikipedia.org/wiki/Password_strength#Human-generated_passwords)
starting at 8 characters in length. Spreading that entropy over a hash and
halving it is not really that great. At the very least, adding a salt means that
it will be quite costly to precompute a dictionary of hashes, but that still
isn't perfect if an attacker has the time and access to the local salt in order
to attempt to brute force the local password. There are two other useful options
that can be pursued to help dissuade offline attacks. The first is that [key
strengthening](http://en.wikipedia.org/wiki/Key_strengthening) can be used to
increase the amount of time it takes to calculate a hash from a dictionary-based
brute force attack that uses repeated passes through a cryptographic hash. The
other option is to make use of a TPM device when it is present, such as in
Chromium OS devices. This allows the TPM to effectively rate limit brute force
attacks without the overhead of key strengthening.

As mentioned above, both Scrypt-based and TPM-based protection may be employed.
We’ll describe the Scrypt method first, being the simpler of the two. The
benefit of using Scrypt is that it uses a unique method of password
strengthening to derive a key for the encryption that is both CPU- and
memory-bound, making a parallelized brute-force attack more costly than a scheme
such as repeated hash rounds. Scrypt is used to directly protect the vault
keyset using a key based on the strengthened user’s password (actually the
user’s weak password hash). The encrypted keyset is stored on disk, and is
unlocked for use at login when valid credentials allow for it to be decrypted.

The preferred method of protecting the vault keyset involves the TPM to protect
against brute force attacks. The method takes advantage of the secure key
storage functionality of the TPM (and the relatively slow clock speed of the
chip) to protect the keyset.

When using the TPM, Chromium OS creates a system-wide RSA key using OpenSSL and
then wraps it with the TPM’s Storage Root Key on first boot. The original key is
destroyed, and we are left with an RSA key whose private key is known only
on-chip on the TPM. This means that, short of a vulnerability in the TPM, in
order to use this RSA key, the private key operation must happen on this
specific TPM, so mechanisms that use it are better protected against brute-force
attacks. Furthermore, as current TPMs can take around half a second for a
private key operation, even attacks which use the TPM are effectively rate
limited.

We use a system-wide key rather than per-user key for two important reasons:
first, key load into the TPM is costly (it must decrypt the private key
component). While 0.5s is a tolerable penalty to pay for the private key
operation, longer (which would be the case if we had to load a user-specific key
on each login attempt) is a noticable impact on user login. Using a system-wide
key allows one key to be loaded and generally left in the TPM while the system
is booted. Since it is protected by the chip, this is okay. Second, the typical
method of incorporating user passwords with keys on the TPM is to require
authentication for their use. This would be a good solution for Chromium OS, but
there is a downside. TPMs feature a vendor-specific dictionary attack defense
mode, which is easily entered into with too many failed TPM authentications. On
many chips, this mode is aggressive, and once entered, renders the chip useless
for some amount of time. Since failed passwords happen (both legitimately, and
by an attacker), too many failed passwords would leave the TPM disabled for a
time, and the vaults it protected unusable. Instead, we use a passwordless
system-wide key, but mix the user’s password in a unique way that achieves the
same benefits without the possibility of entering this mode.

Specifically, when a vault keyset is encrypted, the following process takes
place:

1.  A 256-bit AES vault keyset key (VKK below) is created randomly and
            used to encrypt the vault keyset. The encrypted vault keyset is
            stored to disk. This intermediate key is used to allow the vault
            keyset to be larger than a single RSA operation would allow for.
2.  The VKK is encrypted with the system-wide RSA key previously
            mentioned (TPM_CHK below), resulting in an intermediate encrypted
            vault keyset key (IEVKK).
3.  The final 128 bits of the encrypted blob from step 2 are encrypted
            in place using the user’s weak password hash which has first been
            hashed a keyset-specific salt. This is done using a single AES
            operation with no padding, allowing for no verification on decrypt.
            The result is the encrypted vault keyset key (EVKK), which is stored
            to disk.

Step 3 is critical to combining the user’s weak password hash (and therefore
preventing anyone with access to the TPM and system-wide RSA key from decrypting
the vault keyset). Unless the attacker acquires the user’s password (or guesses
it), the blob passed to the TPM to decrypt using the system-wide key is
effectively garbage--the TPM will fail to decrypt because is is effectively
corrupt until those final 128 bits are corrected. The TPM treats this as a
failed decrypt (rather than a failed authentication), and the vault keyset key
(and vault keyset it protects) cannot be recovered. This operation is
illustrated below:

[<img alt="image"
src="/chromium-os/chromiumos-design-docs/protecting-cached-user-data/ChromiumOS_TPM_Cryptohome.png">](/chromium-os/chromiumos-design-docs/protecting-cached-user-data/ChromiumOS_TPM_Cryptohome.png)

While these protection methods result in protecting the cached user data, what
they effectively address is preventing an attacker from offline brute-force
guessing a user’s Google Accounts password.

## Hashing the user name

The primary reason for hashing the user name is to make it safe for use on the
filesystem without writing a custom mapping of unsafe characters. A web-based
user name may contain characters that are not safe for use on the file system.
Using a strong hash will give a reasonably unique value that is safe for use on
the file system. In addition, if all references to user names outside of the
encrypted images are done using a HASH(salt||user@domain.com), then it becomes
quite difficult for any user, or attacker, to determine what user names are in
use on a given device without attempting a brute force attack. This is a nice
side effect, but not an explicit goal.

## **Password change and offline login**

Password change and offline login are both handled through cryptohomed.

### **Password change**

If the user changes her password from another browser, the vault keyset is still
tied to the previous password. Chromium OS allows password change only on a
successful online login. In that case, Google’s web service will authenticate
the user with her new credentials, returning a successful condition. Chromium OS
will attempt to mount her vault, but that will fail with a bad decrypt
condition. Since Chromium OS knows the password was in fact correct, it will
present the user with the option of entering her old password once (with which
it can recover the vault keyset and then re-protect with the new password), or
simply re-create the vault from scratch.

### **Offline login**

When a Chromium notebook is offline, it cannot authenticate a user with Google’s
web service. However, the encrypted vault keyset can be used for offline
credential verification. Put simply, if a given password results in a successful
recovery of the vault keyset, the password was correct. So, when Chromium OS
doesn’t detect a network connection, it will attempt to authenticate by mounting
the user’s vault directly (this only works if the user has previously logged in
while online, and so she has a vault present on the notebook).

## Encrypting swap and temporary data

At present, Chromium OS devices don't use a swap partition, and temporary data
is written to a tmpfs file system that is not backed by any permanent storage
(such as disk). If a swap device is needed in the future, we'll explore the
challenges associated with protecting that data satisfactorily.

### zram swap

While newer releases of Chrome OS have started including zram for swap purposes,
it is still not stored on disk. zram takes a chunk of memory and uses that for
swap. When pages are moved between that swap device, they are
compressed/uncompressed on the fly. So there still is no permanent storage in
play.

## Error conditions

You may be wondering what happens if an error occurs. Is the user locked out? If
a user authenticates successfully, but all attempts to create or access an
encrypted vault fail, then login may or may not fail. The answer is dependent on
the scenario. Chromium OS automatically detects unrecoverable failures, and
re-creates a user’s vault in the conditions. These conditions are typically
related to the TPM, such as happens if it is cleared (effectively making data
protected by it unrecoverable). Transient failures do not result in a fresh
vault, but both types of failures are rare and usually predictable (see our
document on TPM use for more information).

Regardless, the user can always log in to the device as a guest. While it does
not provide access to the encrypted user data, and the session history is not
stored, it does always allow use of the system.

## Disk speed and battery life

Simple benchmarks indicate that eCryptfs under common browsing patterns does not
significantly alter battery life. It's when sustained reads and writes occur
that more and more CPU is used, and hence more battery draw. A test with a
heavy, sustained write resulted in similar battery discharge rate as the heavy
writing used without encryption, but the encrypted large write took about twice
as long to complete.

In most use cases, disk encryption isn't noticeable. If AES acceleration reaches
additional processors, then the impact will be even lower.

## Directory structure

All metadata for this feature will live under the /home/.shadow directory. Each
user will have a subdirectory with a name based on the user name hash. That
directory will contain all data related to that user's image on the machine. For
example:

/home/.shadow/da39a3ee5e6b4b0d3255bfef95601890afd80709/vault

/home/.shadow/da39a3ee5e6b4b0d3255bfef95601890afd80709/master.0

The system-wide TPM RSA key is stored in:

/home/.shadow/cryptohome.key

## Appendix A: Formal requirements

The primary objective is to protect sensitive end user data from exposure if the
device or drive is lost.

*   User data **MUST NOT** be accessible when the device is powered
            down.
*   User data **MUST** be protected when the disk has been removed from
            a device.
*   User data **MAY** be protected when the device is suspended.
*   Google Accounts passphrases **MUST NOT** be exposed for direct
            offline attack.
*   User data **MUST** be available during offline login.
*   User data **MUST** be recoverable after an out-of-band password
            change.
*   User data **MUST** **NOT** be exposed across users on a multiuser
            device.
*   User data swapped out of memory **MAY BE** protected (depending on
            the requirement below).
*   Power and performance overhead **SHOULD BE** minimized. (Some
            concrete maximum acceptable overhead should be determined. It may
            end up being device specific, however. In particular, boot time,
            login time, and Chromium browser responsiveness issues are
            important.)
*   Users **MUST NOT** be able to opt out of encryption.

Non-goals:

*   Offline credential storage behavior may be a side effect of the
            chosen implementation.
*   A TPM device may be used, but it is a non-goal to create a
            TPM-dependent solution.
*   Root partition encryption/protection is not a goal.
*   It is a non-goal to provide a remote wipe mechanism via this
            feature, but it may make such a design easier.

## Appendix B: Designs considered

*   **Per-user block-based encryption**: This approach would use a
            random key to encrypt a partition per-user that is protected by the
            user's passphrase (and a Google-held backup key). This approach
            would ensure that a user's data will be accessible only when the
            user has logged in, and it allows for easy remote wipe (removing the
            key for the partition). This does add more pain when it comes to
            device management. Each data partition would need to be separated
            into preallocated regions for each simultaneously cached user stored
            on the system (that is, data_space / n = users_space).
    *Note:* This should not be visible to the user unless he tries to log in
    with the new passphrase while he is offline.
*   **Per-user file system-based encryption**: This approach would make
            use of encfs, cryptofs, or eCryptfs to add a layered encrypted file
            system on top of a preconfigured block device. Barring eCryptfs, the
            performance is quite painful with userland solutions like encfs and
            cryptofs. eCryptfs provides good performance, and allows for an
            approach similar to above (guaranteeing per-user privacy and offline
            defenses) without requiring preallocated block devices per user.
*   **Whole disk encryption with TPM**: This would use a TPM-protected
            key to unlock a drive. This would mean that with the whole device,
            the data would be exposed to OS-level attacks once booted, unless
            some form of pre-boot authentication was used. This is not the user
            experience we are looking for, and this would expose data trivially
            to a physical attacker. The disk would only be protected if removed
            from the device.
*   **Whole data partition encryption**: This would simplify the space
            utilization problem by putting all users on one encrypted partition.
            Normal access controls could keep users out of each other's data,
            but adding a new user to the encrypted drive without requiring
            another user to "authorize" the new user at login time is a
            challenge under this scheme. The reason this is an issue is because
            each user's passphrase-derived key is used to armor the disk
            encryption key. Even if a user authorizes another user to access the
            drive, the new user would be required to type the original
            passphrase immediately (or at next login), or a secondary key would
            need to be in play to allow for the encryption key to be decrypted
            and re-encrypted without the first user present. There are no clean
            ways around this.
    Another downside to this approach is that if any user is compromised and a
    privilege escalation vulnerability exists in the system, all user data would
    be compromised.
    Additionally, this approach would complicate any remote-wipe scheme we might
    implement. Whole-partition encryption would necessitate actual file removal
    to implement a per-user wipe, instead of just reinitializing the
    cryptographic file system or full data wipe. If we support only full data
    wipe, then that would probably be restricted to device owners only, but a
    per-user wipe would allow a user to clean his own data off a friend's
    machine he used once.
*   **No encryption**: Not only would this mean most enterprises can't
            use the device for work, it also would mean that many users would be
            unhappy. As is, most people don't want to lose their data if someone
            steals their device. And regardless of policies, people will use
            their personal devices at work. We'd like to think that providing
            simple, effective encryption to the platform is a long-term benefit
            to everyone.
*   **Chromium browser-only encryption**: Pushing encryption into the
            Chromium browser was considered. This approach would ensure that all
            cached data and all downloaded/accessed data is encrypted. This
            solution may be pursued in the long term. At present, providing the
            encryption at the OS level means that it will catch any data
            external to the Chromium browser (Adobe Flash local storage) and
            developer data.
*   **Other interesting side effects**: Both the whole data partition
            encryption and the per-user encryption come with the benefit of a
            free offline credential store. A weak hash of the password can be
            used to protect the stores which would then be checked on mount. In
            addition, the user can be mapped to a system account using this
            information. If a user is password slot 1, then he will be mapped to
            user_1, and so on.

## Appendix C: Threats

The following list includes the primary threats that are addressed by this
design:

*   An attacker acquires a powered down device with the drive in it and
            attempts to access all local user home directories.
*   An attacker acquires a device in suspend state with the drive in it
            and attempts to extract the encryption key with a cold boot attack.

*   An attacker with login privileges attempts to access another user's
            data using a known bug, vulnerability, or workaround.

Many threats are not dealt with through this design:

*   An attacker with a remote compromise will be able to access the
            currently logged in user's data.
*   An attacker will be able to access all user data if autologin is
            enabled and the device is in any state.
*   An attacker will be able to access all user data if the machine is
            suspended or logged in and screen locking is disabled.
*   An attacker will be able to dump all RAM from a suspended machine
            using a [cold boot
            attack](http://en.wikipedia.org/wiki/Cold_boot_attack) exposing any
            credentials in memory and the disk encryption key schedule until
            that can be fixed.
*   An attacker who steals a running machine with screen locking will be
            able to perform screen lock, usb, network, and any other runtime
            attacks to gain access unless the user is able to initiate a remote
            wipe while the machine is still Internet-connected.
*   An attacker who is able to modify the root partition will be able to
            compromise the runtime environment leading to data exposure when a
            user subsequently logs in. Our [verified boot
            process](/chromium-os/chromiumos-design-docs/verified-boot) should
            help combat this, however.

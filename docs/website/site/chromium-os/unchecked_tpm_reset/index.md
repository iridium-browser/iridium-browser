---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: unchecked_tpm_reset
title: Privilege escalation via unchecked TPM reset
---

Vulnerability description

This vulnerability is caused by a hardware design problem specific to a small
faction of device models resulting in the main CPU being able to trigger a reset
of the TPM while the main CPU doesn't reset. This falls short of the design
requirement that a TPM reset implies a reset of the main CPU as well. By
interacting in a certain way with the power management circuitry, the main CPU
can trigger a reset signal that will affect the TPM, but not the main CPU. After
a reset, the TPM starts out in a state where the main CPU can access a lot more
things in the TPM. The usual boot flow locks down the TPM as we progress through
the boot flow. However, since the reset took place while the OS is already
booted, the TPM is left in the state that allows access to privileged data and
commands which is intended to only be accessible to the firmware.

The vulnerability allows attackers with root privileges to trigger a TPM reset.
Afterwards, the following TPM functionality is available to the attacker:

    TPM physical presence - a special privileged mode to access the TPM that is
    intended for firmware use only. Allows access commands that are normally not
    accessible to the OS, or only accessible with TPM owner authorization.

    Platform Configuration Registers (PCR) reset to their initial state. These
    TPM hardware registers reflect the state of the system (e.g. boot mode:
    verified or developer) and can now be updated maliciously to claim an
    incorrect state that does not reflect the actual state.

# Impact

The following Chrome OS security features are affected by the bug:

    Rollback protection for the firmware and kernel.

    Remote attestation aka "Verified Access" statements regarding boot mode are
    no longer trustworthy.

    Device-wide disk encryption secrets can leak across normal mode / developer
    mode transitions.

    Devices may be forced into developer mode without explicit user consent.

    Lockbox / installation-time attributes lose their intended write-once
    property. Decisions that are supposed to be irreversible after initial
    device setup can be tampered with (most prominently, enterprise vs. consumer
    device mode).

Note that the bug only affects the TPM's operational state and it specifically
does not lead to compromise of secrets held by the TPM. Specifically, encrypted
user data or TPM-backed encryption keys are not at risk.

# Affected Versions

Chrome OS version 67 rolls out a firmware update (including a firmware version
bump to prevent downgrade to vulnerable firmware versions) that includes the
firmware mitigation described below.

Newly-produced devices incorporate a hardware change that further inhibits the
ability for the TPM to get reset in isolation at the electrical level.

# Affected Devices

The vast majority of Chrome OS devices deployed in the field are unaffected.
Only the following device models are affected (code names and product names):

    elm - "Acer Chromebook R13 (CB5-312T)"

    hana - "Lenovo 300e/N23 Yoga/Flex 11 Chromebook"

    hana - "Poin2 Chromebook 11C"

    hana - "Poin2 Chromebook 14"

Only elm and hana devices with Board IDs less than 5 is affected. The Board ID
of a device can be determined by checking the bios_info section displayed on
chrome://system.

# Mitigations

## Firmware

Via updated firmware, Chrome OS now blocks access to the offending parts of the
power management controller from the operating system. This prevents the OS from
triggering a TPM reset and thus fixes the problem.

## Hardware

As a defense-in-depth hardening measure, the schematics of the affected devices
have been changed to generate the TPM reset signal in a way that won't be
affected by the problematic power management controller interactions.
Newly-produced devices contain these changes.

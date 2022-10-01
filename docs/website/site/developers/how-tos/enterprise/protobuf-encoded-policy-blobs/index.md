---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/enterprise
  - Enterprise
page_name: protobuf-encoded-policy-blobs
title: Protobuf-encoded policy blobs
---

Cloud policy blobs encode policy settings in a protobuf format, protected with a
signature. The signature facilitates authenticity checks, the key pair is
typically created and owned by the entity managing the device or user. Policy
blobs are also used as the canonical format for device settings on Chromium OS.
They are stored on disk by session_manager, which also takes care of
authenticating incoming protobufs.

## Structure

The term "policy blob" usually refers to a binary-encoded PolicyFetchResponse
protobuf message as defined here:
<https://chromium.googlesource.com/chromium/src/components/policy/proto/+/HEAD/device_management_backend.proto>

This protobuf message is a nested structure and contains binary-encoded payload
protobufs. The layers are as follows:

1.  PolicyFetchResponse is the outer protobuf. It wraps the actual
            payload - a PolicyData protobuf message - in binary form and
            includes a signature that's computed on the payload binary. It also
            contains fields for shipping a new key in case of key rotation.
            Rotation requires the signature on the new key to verify against the
            old key.
2.  PolicyData hosts a fair amount of meta data about the policy, such
            as policy type, timestamps, intended receiver etc. These are used
            for further validity checks. The actual policy values are nested in
            the binary policy_value field. The field contains a binary-encoded
            protobuf, it's message type depends on the policy type.
3.  The CloudPolicySettings message type is used for user-level policy,
            which is indicated by type google/chromeos/user. The protobuf
            definition is generated at compile time from
            chrome/app/policy/policy_templates.json:
            <https://chromium.googlesource.com/chromium/src/+/HEAD/components/policy/resources/policy_templates.json>
4.  Chromium OS device policy is handled by ChromeDeviceSettingsProto
            defined in
            <https://chromium.googlesource.com/chromium/src/+/HEAD/components/policy/proto/chrome_device_policy.proto>
            and is applicable if the policy type is google/chromeos/device.

## Location of binary blobs

Binary policy blobs are stored on Chromium OS in these locations:

*   /var/lib/devicesettings/policy - device policy blob
*   /home/root/&lt;user-hash&gt;/session_manager/policy/policy - user
            policy blob

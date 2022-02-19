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
            <https://chromium.googlesource.com/chromium/src/chrome/browser/chromeos/policy/proto/+/HEAD/chrome_device_policy.proto>
            and is applicable if the policy type is google/chromeos/device.

## Manipulating binary blobs

Binary policy blobs are stored on Chromium OS in these locations:

*   /var/lib/whitelist/policy - device policy blob
*   /home/root/&lt;user-hash&gt;/session_manager/policy/policy - user
            policy blob

To manipulate these files, the protoc compiler (part of the protobuf
distribution) comes in handy, as it is capable of decoding and encoding binary
protobuf messages to and from human-readable text format. We have some tools
that can help with this:

*   A pair of scripts, decode_policy_blob.sh and encode_policy_blob.sh,
            which breaks up the policy blob and decode the individual layers.
            Copies of the scripts are attached, you might have to adjust the
            PROTO_DIR variable depending on the environment you use the scripts
            in. It needs to point at a directory containing the proto definition
            files, which are typically installed in /usr/include/proto a
            Chromium OS build chroot. When run, the scripts operate on text
            files in the current directory containing the textual protobuf
            representations for the individual layers:
    *   policy_response.txt for PolicyFetchResponse
    *   policy_data.txt for PolicyData
    *   chrome_device_policy.txt for ChromeDeviceSettingsProto (note
                that user policy is currently not supported, but that should be
                straightforward to add)
    Note that re-encoding the policy uses a key for the signature that is
    hard-coded in the encoding script, so you might have to swap in the
    appropriate owner key.
*   A tool called policy_reader, which is installed on production images
            and dumps the current device policy blob in textual representation.

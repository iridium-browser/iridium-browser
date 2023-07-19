---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/enterprise
  - Enterprise
page_name: running-the-cloud-policy-test-server
title: Running the cloud policy test server
---

Chromium can pull down enterprise policy configuration from a cloud service. We
have a simplistic python implementation of the management service, so we are
able to test features without relying on a full cloud policy server
implementation. This page explains how to run it:

## Running the test server

1.  You need a Linux Chromium checkout that's in good shape for building
            the browser. See [Get the Code](/developers/how-tos/get-the-code)
2.  Make sure you have the protocol buffer source compiled (add the
            `device_policy_proto` to the line below if you're building for
            Chrome OS):

    ```none
    ninja -C out/Debug py_proto components/policy/core/common full_runtime_code_generate
    ```

3.  Start the test server. You can start testserver.py directly from the
            src/ directory of your Chrome source tree:

    ```none
    OUT_DIR=out/Debug \
    PYTHONPATH=third_party/tlslite:third_party/pywebsocket3/src:$OUT_DIR/pyproto:net/tools/testserver:third_party/protobuf/python:$OUT_DIR/pyproto/components/policy/proto:$OUT_DIR/pyproto/third_party/shell-encryption/src:$OUT_DIR/pyproto/third_party/private_membership/src \
    python components/policy/test_support/policy_testserver.py --data-dir ~/tmp/ --host 127.0.0.1 --port 8889
    ```

    Note: replace out/Debug with out/Release if appropriate, depending on your
    build configuration.
    If this fails with a Python error, try running inside an isolated Python
    environment via [virtualenv](https://virtualenv.pypa.io/en/stable/).
    If you want logging, add these flags:

    ```none
    --log-level DEBUG --log-to-console
    ```

    Notes on parameters:
    *   `--data-dir` specifies the directory from which the server will
                read the policy file (see below). Up to you where to place it.
    *   `--port` specifies the port the server should listen on, you can
                pick a port of your liking.
    *   `--host` The IP address the server should bind to. Note that if
                you want to test a Chromium OS image running on a Chromebook or
                in a Virtual Machine against the server, you need to specify the
                host name or IP address of the public interface in your machine.
                Note that the server only accepts connections from that IP. To
                work around these restrictions you can use a port forwarding
                (ssh is your friend) or local proxy server.
    *   --client-state specifies a file in which to persist current
                server state. This is useful if you want the server to remember
                registered clients and such across server restarts, for example
                when your tinkering with the python code in policy_testserver.py
                to create specific error conditions etc.
    *   --config-file specifies a file that contains server
                configuration. If not specified, the server will default to the
                device_management file in the data directory.
    *   --policy-key a PEM-encoded file containing a private RSA key
                used to sign policy blobs. More information on the policy blob
                format and signatures is
                [here](/developers/how-tos/enterprise/protobuf-encoded-policy-blobs).
                Note that for signing keys to be accepted by Chrome, a
                verification signature is required that certifies the signing
                key. These signatures are checked against a Google-owned key
                pair, the public half of which is [baked into the chrome
                binary](https://code.google.com/p/chromium/codesearch#chromium/src/components/policy/core/common/cloud/cloud_policy_constants.cc&l=65).
                The policy test server contains [hardcoded verification
                signatures](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/policy/test/policy_testserver.py&l=115)
                for a couple test domains. If you specify a policy key on the
                command line, the server will try to load a verification
                signature from the file that's named like the policy key file,
                but with ".sig" appended. Multiple policy keys may be specified
                in the command line in order to test key rotation.
4.  Check whether the server answers requests. Point your browser to
            <http://localhost:8889/test/ping> (or the public IP you've passed to
            the `--host` switch). The server should respond with a page saying
            "Policy server is up."
5.  Ready to roll!

## Setting up a configuration file

The configuration file is a JSON file containing server-global parameters.
Here's an example:

```none
{
  "managed_users": [ "*" ],
  "policy_user": "madmax@managedchrome.com",
  "current_key_index": 0,
  "service_account_identity": "",
  "robot_api_auth_code": "",
  "invalidation_source": 0,
  "invalidation_name": "",
  "device_state": {
    "management_domain": "managedchrome.com",
    "restore_mode": 2
  }
}
```

Notes on parameters:

*   `managed_users` specifies the list of clients the server is allowing
            to register. Each entry is an oauth token, or the "\*" wildcard
            which matches any client.
    (Note that going by OAuth token actually isn't very useful, we should either
    remove this parameter or give the server the ability to figure out the
    actual user)
*   `policy_user` is the user ID to put in policy responses to identify
            the target of the policy settings. This needs to match the user on
            the Chrome side or Chrome will reject the policy.
*   `current_key_index` is the index of the signing key to use when
            generating policy blob signatures.
*   `service_account_identity` is the email address of the service
            account. This is the account used on Chrome OS to enable Google
            cloud services that require authentication. Note that the test
            server can't create service accounts, so this parameter is likely
            only useful for testing (i.e. you have a way to create a service
            account separately and want to inject the proper service account
            name).
*   `robot_api_auth_code` specifies the authentication code the server
            should return when a Chrome OS client asks for one during enterprise
            enrollment. Since the server doesn't have the ability to create
            robot accounts, it can't satisfy these request. Leave this parameter
            empty unless you are testing robot auth setup and have a way to
            create robot accounts and obtain auth codes separately.
*   `invalidation_source` and invalidation_name are used in policy
            change push notifications. Change notifications are not supported by
            the test server. This parameter merely exists to facilitate testing
            using a source identifier obtained elsewhere.
*   `device_state` provides device state parameters requested by Chrome
            OS clients that have gone through a hardware reset and are
            performing a handshake with the server to discover their previous
            state. This is part of the forced re-enrollment and device disabling
            features. You should generally only need these parameters if you're
            specifically testing the aforementioned features.

## Setting up a policy file

The test server reads policy to supply to clients from the data directory
specified with the `--data-dir`. The directory contains text files containing
protobuf messages that supply the payload to return to the client when it asks
for policy. The files are named according to the type of policy requested and
the entity the policy is intended for.

### User policy

The file names are:

*   policy_google_android_user.txt
*   policy_google_chromeos_publicaccount_$PUBLICACCOUNTID.txt
*   policy_google_chromeos_user.txt
*   policy_google_chrome_user.txt
*   policy_google_ios_user.txt

The payload protocol buffer message is CloudPolicySettings. This is generated
from
[policy_templates.json](https://code.google.com/p/chromium/codesearch#search/&q=policy_templates.json&sq=package:chromium&type=cs)
and there is a message field with the name matching the policy name for each
supported policy. The value field within the nested message contains the policy
value. Here is an example with a few policy settings defined:

```none
HomepageLocation {
  value: "http://www.chromium.org"
}
ShowHomeButton {
  value: true
}
```

### Device policy

The file name is policy_google_chromeos_device.txt and the payload protocol
buffer is ChromeDeviceSettingsProto. Example contents:

```none
device_policy_refresh_rate {
  device_policy_refresh_rate: 60
}
user_allowlist {
  user_allowlist: "*@managedchrome.com"
  user_allowlist: "*@gmail.com"
}
device_local_accounts {
  account {
    account_id: "publicsession@managedchrome.com"
    type: ACCOUNT_TYPE_PUBLIC_SESSION
  }
}
```

## Configuring Chromium OS to talk to the test server

In order to do something useful with the test server, you can configure Chromium
built for Chromium OS to talk to the test server for device- and user-level
policy. Here is what you need to do:

1.  Get a root shell on the VM or Chromebook that you want to talk to
            the test server.
2.  Make sure you have a writable root file system. Try `mount -o
            remount,rw /` if you don't, if that fails, you're likely on an
            actual device with enabled root file system protection, in that case
            check out `/usr/share/vboot/bin/make_dev_ssd.sh
            --remove_rootfs_verification`
3.  Edit `/etc/chrome_dev.conf`. Add the following flags:

    ```none
    --device-management-url=http://<your-ip>:<yourport>
    --enterprise-enrollment-skip-robot-auth
    ```

    This points the device at your test server and instructs it to skip robot
    auth setup, which avoids an error during enrollment due to the test server
    not being able to create robot accounts.
4.  On a shell, say

    ```none
    restart ui
    ```

5.  You're now set up to fetch policy from the test server!

## Configuring Chromium to talk to the test server

Pass the following command line flag to chrome:

```none
--device-management-url=http://<your-ip>:<yourport>
```

## User policy

To test some user policy setting, configure the policy file as desired and then
just log in. The browser should automatically pull policy. You can verify that
the policy is correctly pulled down from the server by inspecting
[chrome://policy](javascript:void(0);). To test policy changes, you can also
just update the policy in the file, and use the "Reload policies" button on
[chrome://policy](javascript:void(0);) to refresh policy at runtime.

## Device policy

For devices to receive device policy, they need to be enrolled for enterprise
management at device setup time. There are some requirements for that to
succeed:

*   The device's TPM needs to be clear. In particular, running
            `cryptohome --action=tpm_status` should indicate that the TPM is not
            yet owned. If you have an owned TPM, do the following:

    ```none
    crossystem clear_tpm_owner_request=1
    echo "fast keepimg" > /mnt/stateful_partition/factory_install_reset
    reboot
    ```

    The system will reboot, do a powerwash and reboot again. The device should
    have a clear TPM and be in enrollable state afterwards.
*   The device may not have a consumer owner already, i.e. you shouldn't
            have logged in previously. Ownership is mainly indicated through
            files in `/var/lib/whitelist`, which you can clear like this

    ```none
    stop ui
    rm -rf /var/lib/whitelist/*
    start ui
    ```

    This works well in a VM, note that you probably need a TPM reset an actual
    hardware (see above).

To perform the actual enrollment, hit `Ctrl+Alt+E` on the sign in screen.
Provide credentials (note that in case of the test server, you must match the
"policy_user" field in your JSON config file) and speak a short prayer. If you
get lucky, the device will enroll. Log in and check
[chrome://policy](javascript:void(0);) for whether it says device policy is
present.

---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/telemetry
  - 'Telemetry: Introduction'
page_name: telemetry-mac-keychain-setup
title: Telemetry Mac Keychain Setup
---

    These commands need to be run as the user that will be running the tests.

    These commands assume that the login keychain is unlocked (default on OSX)

    The password was generated via 16 random bytes, base 64 encoded. For test
    bots, I imagine the randomness doesn't matter, so I just chose a specific,
    arbitrary password. If the tests use official builds of Chrome, you'll need
    to replace "Chromium" with "Chrome" in the following instructions.

    ```none
    security delete-generic-password -s "Chromium Safe Storage" login.keychainsecurity add-generic-password -a Chromium -w "+NTclOvR4wLMgRlLIL9bHQ==" -s "Chromium Safe Storage" -A login.keychain
    ```

    If the default keychain settings have been changed, the following lines need
    to be run. Unfortunately, they require the user's password in plaintext, as
    well as knowledge of the current keychain password. (As long as no one has
    mucked with the keychain on the devices, these lines shouldn't be necessary)

    ```none
    security set-keychain-password -o [old_keychain_password] -p [user_login_password] login.keychainsecurity unlock-keychain -p [user_login_password] login.keychain
    ```

#!/bin/bash
#
# Copyright 2012 Google Inc. All Rights Reserved.
# Author: mnissler@google.com (Mattias Nissler)

PROTO_DIR=/build/x86-generic/usr/include/proto
OWNER_KEY="-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAwpLFa4ecSAAf+SC+IZQd0v/WMdMn0gfFHHd2fxdsyOT4QhPb
rdaXpNCCx+41lYZXzPXh3y15iU4lSoUYaGVpbSBTw+BBL/FyZbmL8h3VClnNO033
2B718z2fVbiOGxeCHfN3xotZumoZ74ko7sWnucPT8dvNaTU+c3lZpVNwjIpadKA/
FWOJuYhdO8ujcXar7H66L+dr0m7Hg82uhTzxK6nT5yurjhQsGpK11pIjXGV6uJdq
VSEzja+Rrt5vpBB60QGIOSBkRbOyzNWomh8oUOcqPUq5GDOoPo/3tZzr/2PHj6XN
R4UjOBX+qnzr2SS7Iz5QawT4oU9HN5elj+hSKwIDAQABAoIBAQCgLbPYkgtWOsQX
k5zyh70FtxfebLabcUoT5UTn26DywYye2TpAIik0xXLkpHX4YmBlmwYXdJhZMLwC
XQ964gGolLRgzHzduycyF03eRDDeFI+gAs/GW7aeSFyjdQuHwhKcFZLFIHL9w9sW
FxRbfNxXUZ9pvEmeEvcWmQ/zyn0dNFYz7OY5EaXAysqCWvLVkmOuIiH5bhHAObfh
xtHxoghrb8nDZgdoY6EhPYoJq/8QaoQpj5p0ZdbYE0MtJWVLPzDOL3wRy2ZAVpcn
Vhp/ybdB362OaM7ZA0xIhI6JOvZ6rFxww42QYyKcfXs7JuHQPjoDS9RieByu5mCY
C5o3V4wBAoGBAP4S1DxX56k9FImtGV9/LjDSVakguqAbih4ZLducmwdg8L0K9KbA
wMsJ+3UXVpoOEMRCvzdz7gqAUI4SVyDtZNpaxnW3/KEXRE7B4oDSM5j1/R/bPHUn
b3TPILYYfSEWNXXmbVZrime9FjkrAbX+nlAhr80INY0CfjxgcP4PNguTAoGBAMQM
cui/WDNNdK8NwmlfR0xVWZQd+qLNT5f591dZDNX9K6B8oipjvtX1HUXgE7aSKEc4
Gq9eUEKSLpmgSKmWAPqXLaltD8HpNjAfsFZxgQMATz5ll/7/Qb8SIDDYmZGQpUA2
lRgEcOytj5obaNovZxcvFKBRvrjffD4rXRPmzK4JAoGAT8zKLEnP0TAGC1f66Cuh
7mOh1AUbmL4Nm3Z9GMUPTDn+YuHWBan049C20ggKg0h3q6zrMhePZGz44CaShx0I
2Cw6uS6YgmA0bCgpZByhaCGa5y6Mxp8kOqPzuj3mz0WSdP1yyfns9rhFCp+fYfIe
9zwdY2B4sVlfHMeNtb5BU1ECgYBT4eO0tFI/uS9oyyFYxpySC57FYkJgMCqTIy/y
XrbARI/LHiigrIb1sufwgtzMbCLxvg6k5FzA7x0jPFJ6xSTsE41FBdYNKQS3eIeR
pQUHTLWbRArR31O5Nj8xxyuF/fbGz9PhL91FV0mvLXUijc+1Or6/jdpl7bGSRCmS
H1mKSQKBgB9CP0v6kx1uZQX71qFkE8rmEgQoQ6aBqFTo6PhQTs73y+prJB9M7Rec
UOUPyRo2WvehDMW/xEUXBxAou+Kqx2Wjc1X8ksE4TTvWic0fnC43VTzq9BOw+X8o
jolEsTrkv6jPkizq4mYSxugWg8UTx0FzPfJqlt80FZ3JNgKI5vDA
-----END RSA PRIVATE KEY-----"

function quote_and_label() {
  echo -n "$1: \""
  hexdump -v -e '1/1 "q%03o"' | tr q '\\'
  echo "\""
}

# Encode ChromeDeviceSettingsProto.
echo 'Encoding ChromeDeviceSettingsProto.' 1>&2
sed -i -e '/^policy_value: ".*"$/ d' policy_data.txt
cat "chrome_device_policy.txt" | \
protoc --encode=enterprise_management.ChromeDeviceSettingsProto \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/chrome_device_policy.proto" | \
quote_and_label 'policy_value' \
>> policy_data.txt

# Encode PolicyData.
echo 'Encoding PolicyData.' 1>&2
cat "policy_data.txt" | \
protoc --encode=enterprise_management.PolicyData \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/device_management_backend.proto" | \
quote_and_label 'policy_data' \
> "policy_response.txt"

# Sign PolicyData and recreate PolicyFetchResponse.
echo 'Generating signature.' 1>&2
cat "policy_data.txt" | \
protoc --encode=enterprise_management.PolicyData \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/device_management_backend.proto" | \
openssl dgst -sha1 -sign <(echo "$OWNER_KEY") | \
quote_and_label 'policy_data_signature' \
>> "policy_response.txt"

echo -n "$OWNER_KEY" | \
openssl rsa -pubout -outform DER | \
quote_and_label 'new_public_key' \
>> "policy_response.txt"

# Encode PolicyFetchResponse.
echo 'Encoding PolicyFetchResponse.' 1>&2
cat "policy_response.txt" | \
protoc --encode=enterprise_management.PolicyFetchResponse \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/device_management_backend.proto"

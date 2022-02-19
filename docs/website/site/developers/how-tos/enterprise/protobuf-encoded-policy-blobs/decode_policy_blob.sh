#!/bin/bash
#
# Copyright 2012 Google Inc. All Rights Reserved.
# Author: mnissler@google.com (Mattias Nissler)

PROTO_DIR=/build/x86-generic/usr/include/proto

# Decode PolicyFetchResponse.
echo 'Decoding PolicyFetchResponse.' 1>&2
protoc --decode=enterprise_management.PolicyFetchResponse \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/device_management_backend.proto" \
> "policy_response.txt"

# Decode PolicyData.
echo 'Decoding PolicyData.' 1>&2
cat "policy_response.txt" | \
awk -v ORS= '/^policy_data: ".*"$/ { print substr($0, 15, length($0) - 15); }' | \
python -c "import sys; sys.stdout.write(sys.stdin.read().decode('string_escape'))" | \
protoc --decode=enterprise_management.PolicyData \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/device_management_backend.proto" \
> "policy_data.txt"

# Decode ChromeDeviceSettingsProto.
echo 'Decoding ChromeDeviceSettingsProto.' 1>&2
cat "policy_data.txt" | \
awk -v ORS= '/^policy_value: ".*"$/ { print substr($0, 16, length($0) - 16); }' | \
python -c "import sys; sys.stdout.write(sys.stdin.read().decode('string_escape'))" | \
protoc --decode=enterprise_management.ChromeDeviceSettingsProto \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/chrome_device_policy.proto" \
> "chrome_device_policy.txt"

# [libcast] Cast Streaming Control Protocol (CSCP)

## What is it?

CSCP is the standardized and modernized implement of the Castv2 Mirroring
Control Protocol, a legacy API implemented both inside of
[Chrome's Mirroring Service](https://source.chromium.org/chromium/chromium/src/+/master:components/mirroring/service/receiver_response.h;l=75?q=receiverresponse%20&ss=chromium%2Fchromium%2Fsrc)
and other Google products that communicate with
Chrome, such as Chromecast. This API handles session control messaging, such as
managing OFFER/ANSWER exchanges, getting status and capability information from
the receiver, and exchanging RPC messaging for handling media remoting.

## What's in this folder?

The `streaming_schema.json` file in this directory contains a
[JsonSchema](https://json-schema.org/) for the validation of control messaging
defined in the Cast Streaming Control Protocol. This includes comprehensive
rules for message definitions as well as valid values and ranges. Similarly,
the `receiver_schema.json` file contains a JsonSchema for validating receiver
control messages, such as `LAUNCH` and `GET_APP_AVAILABILITY`.

The `validate_examples.sh` runs the control protocol against a wide variety of
example files in this directory, one for each type of supported message in CSCP.
In order to see what kind of validation this library provides, you can modify
these example files and see what kind of errors this script presents.

NOTE: this script uses
[`yajsv`](https://github.com/neilpa/yajsv/releases/tag/v1.4.0),
which needs to be installed. See the [README.md](../../../README.md#).

For example, if we modify the launch.json to not have a language field:

```
-> % ./validate_examples.sh                                                                                                                                                                                        -1-
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/answer.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/capabilities_response.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/get_capabilities.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/get_status.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/offer.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/rpc.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/streaming_examples/status_response.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/get_app_availability.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/get_app_availability_response.json: pass
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/launch.json: fail: (root): Must validate "then" as "if" was valid
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/launch.json: fail: (root): language is required
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/launch.json: fail: (root): Must validate all the schemas (allOf)
1 of 1 failed validation
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/launch.json: fail: (root): Must validate "then" as "if" was valid
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/launch.json: fail: (root): language is required
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/launch.json: fail: (root): Must validate all the schemas (allOf)
/usr/local/src/openscreen/cast/protocol/castv2/receiver_examples/stop.json: pass
```

## Updating schemas

When updating the JSON schema files, take care to ensure consistant formatting.
Since `clang-format` doesn't support JSON files (currently only Python, C++,
and JavaScript), the JSON files here are instead formatted using
(json-stringify-pretty-compact)[https://github.com/lydell/json-stringify-pretty-compact]
with a max line length of 80 and a 2-character indent. Many IDEs have an
extension for this, such as VSCode's
(json-compact-prettifier)[https://marketplace.visualstudio.com/items?itemName=inadarei.json-compact-prettifier].


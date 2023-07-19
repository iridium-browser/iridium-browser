# Cast Streaming

This module contains an implementation of Cast Streaming, the real-time media
streaming protocol between Cast senders and Cast receivers.

Included are two applications, `cast_sender` and `cast_receiver` that
demonstrate how to send and receive media using a Cast Streaming session.

## Prerequisites

To run the `cast_sender` and `cast_receiver`, you first need to [install
external libraries](external_libraries.md).

## Compilation
NOTE: To use a developer-signed certificate, the cast sender and receiver
binaries must be built with the GN argument `cast_allow_developer_certificate`
set to true.

Putting this together with the [
external libraries](external_libraries.md) configuration, a complete GN
configuration might look like:

```python
is_debug=true
have_ffmpeg=true
have_libsdl2=true
have_libopus=true
have_libvpx=true
cast_allow_developer_certificate=true
```

This can be done on the command line as:
```bash
$ mkdir -p out/Default
$ gn gen --args="is_debug=true have_ffmpeg=true have_libsdl2=true have_libopus=true have_libvpx=true cast_allow_developer_certificate=true" out/Default
$ autoninja -C out/Default cast_sender cast_receiver
```

## Developer certificate generation and use

To use the sender and receiver application together, a valid Cast certificate is
required. The easiest way to generate the certificate is to just run the
`cast_receiver` with `-g`, and both should be written out to files:

```
$ ./out/Default/cast_receiver -g
    [INFO:../../cast/receiver/channel/static_credentials.cc(161):T0] Generated new private key for session: ./generated_root_cast_receiver.key
    [INFO:../../cast/receiver/channel/static_credentials.cc(169):T0] Generated new root certificate for session: ./generated_root_cast_receiver.crt
```

## Running

In addition to the certificate, to start a session you need a valid network
address and path to a video for the sender to play. Note that you may need to
enable permissions for the cast receiver to bind to the network socket.

```bash
$ ./out/Default/cast_receiver -d generated_root_cast_receiver.crt -p generated_root_cast_receiver.key lo0
$ ./out/Default/cast_sender -d generated_root_cast_receiver.crt -r 127.0.0.1 ~/video-1080-mp4.mp4
```

TODO(https://issuetracker.google.com/197659238): Fix discovery mode for `cast_sender`.

### Specifying the codec

To determine which video codec to use, the `-c` or `--codec` flag should be
passed. Currently supported values include `vp8`, `vp9`, and `av1`:

```bash
$ ./out/Default/cast_sender -d generated_root_cast_receiver.crt -r 127.0.0.1 -c av1 ~/video-1080-mp4.mp4
```

### Mac

When running on Mac OS X, also pass the `-x` flag to `cast_receiver` to disable
DNS-SD/mDNS, since Open Screen does not currently integrate with Bonjour.

# End-to-end tests

The script [`standalone_e2e.py`](../standalone_e2e.py) exercises the Cast Sender
sender and receiver through an end-to-end test.

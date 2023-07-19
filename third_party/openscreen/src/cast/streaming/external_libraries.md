# Standalone Sender and Receiver Dependencies

Currently, external libraries are used exclusively by the standalone sender and
receiver applications, for compiling in dependencies used for video decoding and
playback.

The decision to link external libraries is made manually by setting the GN args.
For example, a developer wanting to link the necessary libraries for the
standalone sender and receiver executables might add the following to `gn args
out/Default`:

```python
is_debug=true
have_ffmpeg=true
have_libsdl2=true
have_libopus=true
have_libvpx=true
```

Or on the command line as:
```bash
$ gn gen --args="is_debug=true have_ffmpeg=true have_libsdl2=true have_libopus=true have_libvpx=true" out/Default
```

On some versions of Debian, the following apt-get command will install all of
the necessary external libraries for Open Screen:

```sh
sudo apt-get install libsdl2-2.0 libsdl2-dev libavcodec libavcodec-dev
                     libavformat libavformat-dev libavutil libavutil-dev
                     libswresample libswresample-dev libopus0 libopus-dev
                     libvpx5 libvpx-dev
```

Similarly, on some versions of Raspian, the following command will install the
necessary external libraries, at least for the standalone receiver. Note that
this command is based off of the packages linked in the [sysroot](../../build/config/sysroot.gni):

```sh
sudo apt-get install libavcodec58=7:4.1.4* libavcodec-dev=7:4.1.4*
                     libsdl2-2.0-0=2.0.9* libsdl2-dev=2.0.9*
                     libavformat-dev=7:4.1.4*
```

Note: release of these operating systems may require slightly different
packages, so these `sh` commands are merely a potential starting point.

Finally, note that generally the headers for packages must also be installed.
In Debian Linux flavors, this usually means that the `*-dev` version of each
package must also be installed. In the example above, this looks like having
both `libavcodec` and `libavcodec-dev`.

## libaom

For AV1 support, it is advised that most Linux users compile and install
`libaom` from source, using the instructions at
https://aomedia.googlesource.com/aom/ Older versions found in many package
management systems are not compatible with the Open Screen Library because of
API compatibility and performance issues.

To to enable AV1 support, also add the following to your GN args:

```python
have_libaom=true
```

Note that AV1 support is configured separately from the other standalone
libraries and the `have_libaom` flag is not necessary to run the standalone
demo.

TODO(mfoltz): Describe how to set `libaom_lib_dirs` and `libaom_include_dirs`
for your libaom install.

## Standalone Sender

The standalone sender uses `ffmpeg`, `libopus`, and `libvpx` for encoding video
and audio for sending. When the build has determined that [have_external_libs](
../standalone_sender/BUILD.gn ) is set to true, meaning that all of these
libraries are installed, then the VP8 and Opus encoders are enabled and actual
video files can be sent to standalone receiver instances. Without these
dependencies, the standalone sender cannot properly function (contrasted with
the standalone receiver, which can use a dummy player).

## Standalone Receiver

The standalone receiver also uses `ffmpeg`, for decoding the video stream
encoded by the sender, and also uses `libsdl2` to create a surface for decoding
video.  Unlike the sender, the standalone receiver can work without having its
[have_external_libs](../standalone_receiver/BUILD.gn) set to true, through the
use of its [Dummy Player](../standalone_receiver/dummy_player.h).

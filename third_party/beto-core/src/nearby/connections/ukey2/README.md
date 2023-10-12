This is a general description of what is contained in each crate and how it should be used.

```ukey2```:
-
- Contains the state machine required to run the handshake
- A new state type is return on each message exchanged with this state machine
- Current status: Wire-compatible with existing implementations of UKEY2 using the HandshakeImplementation::PublicKeyInProtobuf value.
- TODO: Improve error handling

```ukey2-connections```:
-
- Convenient wrapper around the ```ukey2``` crate for running the handshake, located in ```d2d_handshake_context.rs```.
- Creates a connection context encoding/decoding messages to/from the peer, located in ```d2d_connection_context_v1.rs```.
- Current status: Fully wire-compatible with existing implementations of UKEY2.
- TODO: Improve error handling

```ukey2-jni```:
-
- Houses a JNI wrapper for the ````ukey2-connections```` crate
- Includes a small Java test applet to test throwing exceptions and an example of how to use the library
- Automatically uses the ```HandshakeImplementation::PublicKeyInProtobuf``` implementation for compatibility with existing implementations.
- Current status: Working with the driver code in ```ukey2-jni/java```

```ukey2-c-ffi```:
-
- Houses a C interface for the Rust library
- Includes a header that can be used to link against the library
- Automatically uses the ```HandshakeImplementation::PublicKeyInProtobuf``` implementation for compatibility with existing implementations.
- Current status: Handshake and message exchange working with the ```ukey2-c-ffi/cpp/``` test binary
- To build the test binary, first build the FFI library with Cargo (only works on Linux) and then from ukey2-c-ffi: ```bazel (or blaze) build //cpp:ukey2```
- TODO: Improve error handling

```ukey2-shell```:
-
- A small shell application written very similarly to the C++ one used for testing against the Java implementation
- Current status: Working, tested against the Java implementation.
# libipp

## What is this?

General C++ library for building and parsing IPP frames. IPP stands for Internet
Printing Protocol and is defined in several documents. This implementation
is based mainly on the following sources:
*   [RFC 8010](https://tools.ietf.org/html/rfc8010)
*   [RFC 8011](https://tools.ietf.org/html/rfc8011)
*   [IANA IPP registry](https://www.pwg.org/ipp/ipp-registrations.xml)
*   [CUPS Implementation of IPP](https://www.cups.org/doc/spec-ipp.html)

## How to use it?

All required C++ classes, types and functions are declared in `ipp` namespace.

IPP frames are sent/received as a payload of HTTP POST requests/responses.
This library helps to build and parse raw IPP frames,
but does not support the HTTP protocol.
You have to use some other library to process HTTP packages,
like *libbrillo* or *libcurl*.
You can also dump a raw IPP frame to a file and send it from the command line
with *curl*, e.g.:

```shell
curl -X POST "http://my.server:631/mypath" --header "Content-Type: application/ipp" --data-binary @ipp.frame
```

Then obtained response can be read from the file and parsed by *libipp*.

## Documentation conventions

In this documentation, the following typographical conventions are used:
*   **boldface** denotes entities defined in the IPP specifications (see the
    links mentioned above);
*   *italics* indicates a name of a library or a (shell) command;
*   `monospace` is used to mark entities from the source code.

## Link to documentation

*   [API specification](docs/api.md)

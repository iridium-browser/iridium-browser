---
breadcrumbs: []
page_name: quic
title: QUIC, a multiplexed transport over UDP
---

QUIC is a new multiplexed transport built on top of UDP. HTTP/3 is designed to
take advantage of QUIC's features, including lack of Head-Of-Line blocking
between streams.

The QUIC project started as an alternative to TCP+TLS+HTTP/2, with the goal of
improving user experience, particularly page load times. The [QUIC working
group](https://datatracker.ietf.org/wg/quic/about/) at the IETF defined a clear
boundary between the
transport([QUIC](https://datatracker.ietf.org/doc/html/rfc9000)) and
application(HTTP/3) layers, as well as migrating from QUIC Crypto to [TLS
1.3](https://datatracker.ietf.org/doc/html/rfc8446).

Because TCP is implemented in operating system kernels and middleboxes, widely
deploying significant changes to TCP is next to impossible. However, since QUIC
is built on top of UDP and the transport functionality is encrypted, it suffers
from no such limitations.

**Key features of QUIC and HTTP/3 over TCP+TLS and HTTP/2 include**

*   Reduced connection establishment time - 0 round trips in the common
            case
*   Improved congestion control feedback
*   Multiplexing without head of line blocking
*   Connection migration
*   Transport extensibility
*   Optional unreliable delivery

**IETF documents**

*   Version-Independent Properties of QUIC -
            [RFC8999](https://datatracker.ietf.org/doc/html/rfc8999)
*   QUIC: A UDP-Based Multiplexed and Secure Transport -
            [RFC9000](https://datatracker.ietf.org/doc/html/rfc9000)
*   Using TLS to Secure QUIC -
            [RFC9001](https://datatracker.ietf.org/doc/html/rfc9001)
*   Congestion Control and Loss Detection -
            [RFC9002](https://datatracker.ietf.org/doc/html/rfc9002)
*   HTTP/3 - IESG approved
            [draft](https://datatracker.ietf.org/doc/html/draft-ietf-quic-http)
*   QPACK - IESG approved
            [draft](https://datatracker.ietf.org/doc/html/draft-ietf-quic-qpack)

**Documentation**

*   [SIGCOMM 2017 Paper](https://research.google/pubs/pub46403/)
*   [QUIC
            overview](https://docs.google.com/document/d/1gY9-YNDNAB1eip-RTPbqphgySwSNSDHLq9D5Bty4FSU/edit?usp=sharing)
*   [QUIC FAQ](/quic/quic-faq)
*   [A Guide to Parsing QUIC Client Hellos for Network Middlebox
            Vendors](https://docs.google.com/document/d/1GV2j-PGl7YGFqmWbYvzu7-UNVIpFdbprtmN9tt6USG8/preview)
*   [Legacy QUIC wire specification (versions up to and including
            Q043)](https://docs.google.com/document/d/1WJvyZflAO2pq77yOLbp9NsGjC1CHetAXV8I0fQe-B_U/edit?usp=sharing)
*   [Legacy QUIC wire specification (versions from Q044 to Q048
            inclusive)](https://docs.google.com/document/d/1FcpCJGTDEMblAs-Bm5TYuqhHyUqeWpqrItw2vkMFsdY/edit?usp=sharing)
*   [Cleartext QUIC wire specification (versions Q049 and
            above)](https://tools.ietf.org/html/draft-ietf-quic-invariants-07)
*   [QUIC crypto design
            doc](https://docs.google.com/document/d/1g5nIXAIkN_Y-7XJW5K45IblHd_L2f5LTaDUDwvZ5L6g/edit?usp=sharing)
*   [Getting started with the QUIC toy client and
            server](/quic/playing-with-quic)
*   [QUIC tech talk](https://www.youtube.com/watch?v=hQZ-0mXFmk8)
*   [QUIC
            Discovery](https://docs.google.com/document/d/1i4m7DbrWGgXafHxwl8SwIusY2ELUe8WX258xt2LFxPM/edit?usp=sharing)
*   [QUIC FEC v1
            (deprecated)](https://docs.google.com/document/d/1Hg1SaLEl6T4rEU9j-isovCo8VEjjnuCPTcLNJewj7Nk/edit?usp=sharing)
*   [QUIC flow
            control](https://docs.google.com/document/d/1F2YfdDXKpy20WVKJueEf4abn_LVZHhMUMS5gX6Pgjl4/edit?usp=sharing)

**Pre-Working Group IETF Material**

*   Presentations from the IETF-96 (Berlin) BoF on QUIC
    *   [Introduction](https://datatracker.ietf.org/meeting/96/materials/slides-96-quic-0)
    *   [QUIC Protocol
                Overview](https://www.ietf.org/proceedings/96/slides/slides-96-quic-5.pdf)
*   Presentations from the IETF-93 (Prague) BarBoF on QUIC
            ([Video](http://recordings.conf.meetecho.com/Playout/watch.jsp?recording=IETF93_QUIC&chapter=BAR_BOF))
    *   [Introduction](https://docs.google.com/presentation/d/15bnWhEBVRVZDO5up7UTpZU-o6jPOfGU4fT5JlBZ7-Cs/edit?usp=sharing)
    *   [QUIC Protocol
                Overview](https://docs.google.com/presentation/d/15e1bLKYeN56GL1oTJSF9OZiUsI-rcxisLo9dEyDkWQs/edit?usp=sharing)
    *   [Implementing QUIC For Fun And
                Learning](https://docs.google.com/presentation/d/1BjPUowoOoG0ywmq5r8QNqnC9JPELUe02jvgyoOW3HFw/edit?usp=sharing)
                (Christian Huitema, Microsoft)
    *   [QUIC Congestion Control and Loss
                Recovery](https://docs.google.com/presentation/d/1T9GtMz1CvPpZtmF8g-W7j9XHZBOCp9cu1fW0sMsmpoo/edit?usp=sharing)
*   Initial Presentation at IETF-88
            ([Slides](https://www.ietf.org/proceedings/88/slides/slides-88-tsvarea-10.pdf))

Video

*   [QUIC Connection Migration
            demo](https://drive.google.com/file/d/1DlMI_3MOxnWarvEVfzKxFqmD7c-u1cYG/view?usp=sharing)

Code

*   [Chrome
            implementation](https://chromium.googlesource.com/chromium/src/+/HEAD/net/quic/)
*   [Standalone test server and
            client](https://chromium.googlesource.com/chromium/src/+/HEAD/net/tools/quic/)

**Mailing lists**

*   [proto-quic@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/proto-quic)
            - QUIC mailing list specific to Chromium
*   [quic@ietf.org](https://mailarchive.ietf.org/arch/browse/quic/) -
            QUIC working group mailing list

**Abridged QUIC Version History**
*   In 2012, Google started working on QUIC
*   In 2013, Chrome started small-scale experiments with the original versions of QUIC (those versions are now known as Google QUIC)
*   In 2014, Chrome started a wide-scale deployment of Google QUIC
*   In 2015, Google brought QUIC to the IETF
*   In 2017, the IETF started creating versions of QUIC that diverged from Google QUIC (those new versions were then called IETF QUIC)
*   In 2020, Chrome started wide-scale experiments with IETF QUIC
*   In 2021, the IETF officially published QUIC as RFC 9000. Chrome added support for RFC 9000 in Chrome 90 and default-enabled it for all users in Chrome 93

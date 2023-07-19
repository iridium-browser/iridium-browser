---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: time-sources
title: Time Sources
---

Reliably figuring out what time it is on a machine operating in an untrustworthy
environment is pretty difficult. Currently we use the following sources:

1.  The machine's realtime clock, if set
2.  htpdate(1) pointed at http://www.google.com

Unfortunately, at first boot, the realtime clock may not be set well, and there
exist platforms where the realtime clock does not persist across power cycles
because it is not battery-backed. We need to get the right time on these
machines, mainly to avoid attacks on TLS connections (by setting the date far
enough backwards, an attacker might render a TLS certificate that expired valid
again). Right now, htpdate(1) refuses to set the date to before the time it was
built. There could also be attacks that are opened up by pushing the system
clock far enough forward (e.g., causing various system daemons to see files on
disk with an apparent modification time far in the future, or causing
cryptohomes to not be expired as 'old' when they should be). Currently we have
no real protection against these (admittedly limited-scope) attacks. Worse,
without a reliable realtime clock, we're always fetching our system time from an
unauthenticated source. We have had problems with this in practice - e.g.,
captive portals that intercept HTTP traffic and return their own headers,
sometimes with wildly bogus times in them.
We should instead use the following sources:

1.  tlsdate(1) pointed at https://www.google.com
2.  A file stored in the stateful partition with the last known good
            time (from tlsdate) and the last known rtc time.
    *   *The file system itself has a "last modified" timestamp that
                might be more straightforward -- jrbarnette*
    *   *This can get tricky though as the mtime field could be updated
                for various reasons, some of which are susceptible to the
                aforementioned attack vectors. Having dedicated storage would be
                clear as to its meaning, and the micro optimization here
                probably doesn't gain much in trading off to complexity.*
3.  The machine's rtc

Of these sources, we can always consider 1 authoritative if we do get a
response, since we'll be doing SSL validation of the whole connection. For
situations where we can't do 1 (for example, if we're behind a captive portal
that is blocking our TLS connection), we can look at the machine's rtc and
compare it against the file mentioned in 2; if the rtc is ahead of the file by
some reasonable offset, we can trust the rtc and update the file. If the
machine's rtc isn't ahead of the file (or is too far ahead), we probably have no
reliable time sources and will have to fall back on setting our time to the last
good time stored in the file.

There are a couple of other sources we could try: on machines with a 3g modem,
we might have access to the network time, but doing this is likely to be fraught
with peril. There might be other pieces of hardware on the system (the TPM?)
that have a notion of what time it is.

There are some upsides of this crazy plan:

1.  Drop htpdate, which is about 850 lines of code that parses HTTP, in
            exchange for tlsdate, which is 500 lines of code that uses OpenSSL
            very carefully;
2.  Works on systems without rtcs

An open question is whether we should accept tlsdate updates if TLS
authentication of the connection fails. If we accept unauthenticated updates,
we're vulnerable to clock manipulation in the interval (build time, build time +
max offset), with max offset most likely being on the order of a few years, but
we have a decent chance of getting an at-least-roughly-accurate time. If we
reject unauthenticated updates, we may find ourselves in a situation where we
have no reliable notion of the time at all, which would force us to default to
the build time. Worse, having no reliable time might make it impossible to
authenticate to a given captive portal. For a better UX, it's probably wise to
just accept the untrusted time, clamping it to the boundaries as needed, since
otherwise users might find themselves mysteriously unable to authenticate to a
captive portal.

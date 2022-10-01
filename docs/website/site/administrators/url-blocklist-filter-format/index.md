---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: url-blocklist-filter-format
title: URL Blocklist filter format
---

The format of filters for the URLBlocklist and URLAllowlist policies, as of
Chrome 52, is:

\[scheme://\]\[.\]host\[:port\]\[/path\]\[@query\]

*   Scheme can be http, https, ftp, chrome, etc. This field is optional,
            and must be followed by '://'.
*   An optional '.' (dot) can prefix the host field to disable subdomain
            matching, see below for details.
*   The host field is required, and is a valid hostname or an IP
            address. It can also take the special '\*' value, see below for
            details.
*   An optional port can come after the host. It must be a valid port
            value from 1 to 65535.
*   An optional path can come after port. Any string can be used here.
*   An optional query can come in the end, which is a set of key-value
            and key-only tokens delimited by '&'. The key-value tokens are
            separated by '='. A query token can optionally end with a '\*' to
            indicate prefix match. Token order is ignored during matching.

The format is very similar to the URL format, with some exceptions:

*   user:pass fields can be included but will be ignored (e.g.
            http://user:pass@ftp.example.com/pub/bigfile.iso).
*   If a reference separator '#' is present, it is ignored along with
            everything that appears after it.
*   The host can be '\*'. It can also have a '.' as a prefix.
*   The host can have a '/' or '.' as suffix. If it is the case, that
            suffix is ignored.

The filter selected for a URL is the most specific match found:

1.  First, the filters with the longest host match will be selected;
2.  Among these, filters with a non-matching scheme or port are
            discarded;
3.  Among these, the filter with the longest matching path is selected;
4.  Among these, the filter with the longest set of query tokens are
            selected;
5.  If no valid filter is left at step 3, the host is reduced by
            removing the left-most subdomain, and trying again from step 1;
6.  If a filter is available at step 3, its decision (block or allow) is
            enforced. If no filter ever matches, the default is to allow the
            request.

The special '\*' host will be the last searched, and matches all hosts. When
both a blocklist and allowlist filter apply at step 4 (with the same path length
and number of query tokens), the allowlist filter takes precedence. If a filter
has a '.' (dot) prefixing the host, only exact host matches will be filtered:

*   "example.com" matches "example.com", "www.example.com" and
            "sub.www.example.com";
*   ".www.example.com" only matches exactly "www.example.com".

The scheme can be either a standard or a custom one. Supported standard schemes
are: about, blob, content, chrome, cid, data, file, filesystem, ftp, gopher,
http, https, javascript, mailto, ws, wss. All other schemes are treated as
custom schemes. As of Chrome 52, custom schemes are supported, but only the
patterns scheme:\* and scheme://\* are allowed. They match all URLs with that
scheme.

*   The patterns "custom://\*" or "custom:\*" are valid and match
            "custom:app".
*   The patterns "custom:app" or "custom://app" are invalid.

The scheme and as of Chrome 52 the host are case insensitive, while path and
query are case sensitive.

*   "http://example.com" matches "HTTP://Example.com",
            "http://example.COM" and "http://example.com";
*   "http://example.com/path?query=1" doesn't match
            "http://example.com/path?Query=1", "http://example.com/Path?query=1"
            but matches "http://Example.com/path?query=1";

Example of searching for a match for "http://mail.example.com/mail/inbox":

1.  First find filters for "mail.example.com", and go to step 2. If that
            fails, then try again with "example.com", "com" and finally "".
2.  Among the current filters, remove those that have a scheme which is
            not http.
3.  Among the current filters, remove those that have an exact port
            number and it not 80;
4.  Among the current filters, remove those that don't have
            "/mail/inbox" as a prefix of the path;
5.  Pick the filter with the longest path prefix, and apply it. If no
            such filter exists, go back to step 1 and try the next subdomain.

Some examples:

*   "example.com" blocks all requests to that domain and any subdomain;
*   "http://example.com" blocks all HTTP requests to that domain and any
            subdomain; Requests with other schemes (such as https, ftp, etc.)
            are still allowed;
*   "https://\*" blocks all HTTPS requests to any domain;
*   "mail.example.com" blocks this domain but not "www.example.com" nor
            "example.com";
*   ".example.com" blocks exactly "example.com", and won't block
            subdomains;
*   "\*" blocks all requests; only allowlisted URLs will be allowed;
*   "\*:8080" blocks all requests to port 8080;
*   "example.com/stuff" blocks all requests to any subdomain of
            "example.com" that have "/stuff" as a prefix of the path;
*   "192.168.1.2" blocks requests to this exact IP address;
*   Any request with the query "?video=100" is blocked by "\*?v\*",
            "\*?video\*", "\*?video=\*" and "\*?video=100\*";
*   "\*?a=1&b=2" blocks any request with the query "?b=2&a=1",
            "?a=1&b=2", "?a=1&c=3&b=2", ...;
*   For a blocklist any occurrence of the key-value pair is sufficient,
            i.e., blocklisting "youtube.com/watch?v=xyz" would block
            "youtube.com/watch?v=123&v=xyz".
*   For an allowlist every occurrence of the key should have a matching
            value, i.e., allowlisting "youtube.com/watch?v=V2" does not allow
            "youtube.com/watch?v=V1&v=V2", it allows
            "youtube.com/watch?v=V2&v=V2" though.

Example: allowing only a small set of sites:

*   Block "\*"
*   Allow selected sites: "mail.example.com", "wikipedia.org",
            "google.com"

Example: block all access to a domain, except to the mail server using HTTPS and
to the main page:

*   Block "example.com"
*   Allow "https://mail.example.com"
*   Allow ".example.com", and maybe ".www.example.com"

Example: block all access to youtube, except for selected videos.

*   Block "youtube.com"
*   Allow "youtube.com/watch?v=V1"
*   Allow "youtube.com/watch?v=V2"

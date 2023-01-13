---
breadcrumbs:
- - /flash-roadmap
  - Flash Roadmap
page_name: deprecating-wildcards
title: Deprecating wildcards
---

### Wildcard base and edge cases

Through enterprise policies:

Notes:

Currently, a wildcard port is
[serialized](https://source.chromium.org/chromium/chromium/src/+/HEAD:components/content_settings/core/common/content_settings_pattern_parser.cc;l=234-237;drc=456596a0b27623349d38e49d0e9812b24d47d5d8?originalUrl=https:%2F%2Fcs.chromium.org%2F)
as an empty port into prefs. Changing the semantics would require migration.

<table>
<tr>

<td>Pattern</td>

<td>Expected</td>
<td>behavior</td>

<td>Implemented behavior</td>

<td>Reason for implemented behavior</td>

</tr>
<tr>

<td>http://foo.com:80/</td>
<td>https://bar.com:443/</td>
<td>https://bar.com:8081/</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Everything specified.</td>

</tr>
<tr>

<td>http://foo.com/</td>
<td>https://bar.com/</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Concrete scheme, concrete host, empty path, unspecified port (implicit wildcard). Matches origins with any port.</td>

</tr>
<tr>

<td>http://www.foo.com:\*</td>
<td>https://www.foo.com:\*</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Concrete scheme, concrete host, empty path, explicit wildcard port. Matches origins with any port.</td>

</tr>
<tr>

<td>www.foo.com:80</td>
<td>\*:www.foo.com:80</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Wildcard or unspecified (=implicit wildcard) schemes are permitted</td>

</tr>
<tr>

<td>\*://www.foo.com</td>
<td>www.foo.com:\*</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Unspecified/wildcarded ports and schemes are permitted.</td>

</tr>
<tr>

<td>https://www.foo.com:443/\*</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Path wildcards are allowed. They are meaningless, as the pattern is always matched against an origin.</td>

</tr>
<tr>

<td>https://\[\*.\]foo.com:443</td>
<td>\[\*.\]foo.com</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>Disallowed because of subdomain wildcard in host.</td>

</tr>
<tr>

<td>https://\*:443</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>Disallowed because of a full wildcard in host.</td>

</tr>
<tr>

<td>\*</td>
<td>\*:\*</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>Scheme host port path all wildcard. Disallowed because of the host wildcard. Scheme/path/port wildcard would be fine.</td>

</tr>
<tr>

<td>https://\*</td>
<td>https://\*:\*</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>Concrete scheme, but host port path all wildcard. Disallowed because of the host wildcard.</td>

</tr>
</table>

Through extensions
([format](https://developer.chrome.com/extensions/match_patterns), note that
port cannot be specified, but path must be specified):

Notes:

    The omitted port maps to the default port (80 and 443), not the wildcard.

    The port can be optionally specified, and can be specified as “\*”, which
    then maps to the wildcard.

    The only allowed path for http/https is “/\*”, and that maps to the empty
    path, not a wildcard.

<table>
<tr>

<td>Pattern</td>

<td>Expected</td>

<td>behavior</td>

<td>Implemented behavior</td>

<td>Reason for implemented behavior</td>

</tr>
<tr>

<td>http://foo.com/\*</td>
<td>https://bar.com/\*</td>

<td>http://foo.com:80/\*</td>

<td>https://foo.com:80/\*</td>

<td>Allowed</td>

<td>Allowed.</td>

<td>The omitted port is assumed to be the default port, and /\* maps to the empty path.</td>

</tr>
<tr>

<td>https://foo.com:\*/\*</td>

<td>Allowed</td>

<td>Allowed.</td>

<td>Concrete scheme, host, empty path, wildcard port.</td>

</tr>
<tr>

<td>\*://www.foo.com/\*</td>

<td>Allowed</td>

<td>Allowed</td>

<td>Wildcard scheme is permitted.</td>

</tr>
<tr>

<td>&lt;all_urls&gt;</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>All-wildcard.</td>

</tr>
<tr>

<td>https://\*.foo.com/\*</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>Subdomain wildcard.</td>

</tr>
<tr>

<td><a href="javascript:void(0);">https://\*/\*</a></td>
<td>\*://\*/\*</td>

<td>Disallowed</td>

<td>Disallowed</td>

<td>Domain wildcard.</td>

</tr>
<tr>

<td>www.foo.com/\*</td>
<td>\*.foo.com/\*</td>

<td>Invalid</td>

<td>Invalid (The scheme must be present)</td>

</tr>
<tr>

<td>http://foo.com/path\*</td>

<td>www.foo.com/index.html</td>

<td>http://www.google.com/</td>

<td>http://www.google.com</td>

<td>Invalid</td>

<td>The only allowed path for http/https is “/\*”, and that maps to the empty path, not a wildcard.</td>

</tr>
<tr>

<td>http://\*foo/bar/\*</td>
<td>http://foo.\*.bar/baz/\*</td>
<td>https://\[\*.\]foo.com:443/\*</td>

<td>Invalid</td>

<td>Invalid ('\*' in the host can only be the first character and must be followed by ‘.’, and subdomain wildcards are not supported)</td>

</tr>
</table>

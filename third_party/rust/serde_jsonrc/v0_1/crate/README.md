# serde_jsonrc

This is a lenient JSON parser forked from the
[serde_json](https://crates.io/crates/serde_json) crate
that is that is designed to parse JSON written by humans
(e.g., JSON config files). This means that it supports:

- `/*` and `//` style comments.
- Trailing commas for object and array literals.
- `\v` and `\xDD` literal escapes (for vertical tab and
  two-digit hexadecimal characters)
- [planned] Unquoted object keys (precise spec TBD).

I am still playing with the idea of making it more lenient,
which could include taking more features from the
[JSON5](https://json5.org/) spec.

## Motivation

When I created this crate, my immediate goal was to create a fast parser
for a config file for a work project. I wanted a file format that would
be familiar to developers, but restrictive in what it accepted.
I had encountered this problem several times in my career, which always
faced the same set of tradeoffs:

- JSON: Not easy enough to use because of lack of support for comments
  and trailing commas.
- YAML: Input language has too many features.
- TOML: Use is not yet widespread enough that I would consider it
  "familiar to developers." Also, nested objects either end up
  being verbose or about the same as JSON.

When considering the relative downsides of each of these options, it
was clear that what I really wanted was a more lenient JSON.
The next question was how to get a lenient JSON parser (in Rust, in
my case). I considered the following options:

### Make serde_json lenient

This was my first choice, but [the maintainer wanted to keep the
scope of `serde_json` limited to strict JSON](https://github.com/dtolnay/request-for-implementation/issues/24),
so we respectfully agreed that forking was the way to go.

### json5 crate

The [json5 crate](https://crates.io/crates/json5) supports the superset
of JSON specified at https://json5.org/. In principle, the feature set
met my needs, but in practice, I discovered the implementation was not
nearly as performant as `serde_json`, even for small files.
(Also, it cannot parse streams: only strings.)

### serde-hjson crate

The [serde-hjson crate](https://crates.io/crates/serde-hjson) provies
a parser for a different superset of JSON named [Hjson](http://hjson.org/)
("JSON for humans"). I am not a fan of Hjson because the language
it accepts is not valid JavaScript, so it's not nearly intuitive as
JSON.

## Long-Term Goals

Ultimately, I would like to see a more lenient form of JSON
standardized that experiences the same level of ubiquity as JSON
today. I would like this crate to be a reference implementation
of that new, more lenient specification.

I suspect that being more conservative in adding new
features to the spec has the best chance of getting widespread
buy-in, which is why I am not immediately gravitating towards
implementing all of [JSON5](https://json5.org/). Instead,
I am starting with my [suggested improvements to JSON](http://bolinfest.com/essays/json.html)
from way back in 2011.

Finally, my gut feeling is that a new version of JSON should still
be valid JavaScript. For example, one other shortcoming of JSON today
is a lack of support for multiline strings. JSON5 addresses this by
allowing `\` for continued lines, but at this point, I think backtick
(`` ` ``) would be a more intuitive solution because that would be
consistent with ES6 (though string interpolation would not be supported).

## License

Because serde_jsonrc is a fork of serde_json, it maintains the original licence,
which means it is licensed under either of

- Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
  http://www.apache.org/licenses/LICENSE-2.0)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or
  http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in serde_jsonrc by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.

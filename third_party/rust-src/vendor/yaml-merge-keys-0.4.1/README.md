# Yaml merge keys

This crate implements support for the [Merge Key Language-Independent Type for
YAML][] draft specification for YAML documents from the `yaml-rust` and
`serde_yaml` (with the `serde_yaml` feature) crates.

When a mapping in a YAML document contains a `<<` key, its value should be
either a mapping or a sequence of mappings. For each mapping, it is merged
into the parent mapping where the parent mapping wins conflicts (so that it
may override keys from the merge set).

[Merge Key Language-Independent Type for YAML]: http://yaml.org/type/merge.html

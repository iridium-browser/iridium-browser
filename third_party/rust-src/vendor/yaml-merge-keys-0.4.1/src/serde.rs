// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use crates::serde_yaml::Value;
use crates::yaml_rust::Yaml;

use merge_keys;
use MergeKeyError;

struct YamlWrap(Yaml);

impl YamlWrap {
    fn into_yaml(self) -> Yaml {
        self.into()
    }
}

impl From<YamlWrap> for Yaml {
    fn from(yaml: YamlWrap) -> Self {
        yaml.0
    }
}

impl From<Yaml> for YamlWrap {
    fn from(yaml: Yaml) -> Self {
        YamlWrap(yaml)
    }
}

impl From<Value> for YamlWrap {
    fn from(yaml: Value) -> Self {
        YamlWrap(match yaml {
            Value::Number(n) => {
                if let Some(i) = n.as_i64() {
                    Yaml::Integer(i)
                } else {
                    Yaml::Real(format!("{}", n))
                }
            },
            Value::String(s) => Yaml::String(s),
            Value::Bool(s) => Yaml::Boolean(s),
            Value::Sequence(seq) => {
                Yaml::Array(
                    seq.into_iter()
                        .map(Into::into)
                        .map(Self::into_yaml)
                        .collect(),
                )
            },
            Value::Mapping(map) => {
                Yaml::Hash(
                    map.into_iter()
                        .map(|(k, v)| (Self::into_yaml(k.into()), Self::into_yaml(v.into())))
                        .collect(),
                )
            },
            Value::Null => Yaml::Null,
        })
    }
}

impl From<YamlWrap> for Value {
    fn from(yaml: YamlWrap) -> Self {
        match yaml.0 {
            Yaml::Real(f) => {
                match f.parse::<f64>() {
                    Ok(f) => Value::Number(f.into()),
                    Err(_) => Value::String(f),
                }
            },
            Yaml::Integer(i) => Value::Number(i.into()),
            Yaml::String(s) => Value::String(s),
            Yaml::Boolean(b) => Value::Bool(b),
            Yaml::Array(array) => {
                Value::Sequence(
                    array
                        .into_iter()
                        .map(|item| {
                            let wrap: YamlWrap = item.into();
                            wrap.into()
                        })
                        .collect(),
                )
            },
            Yaml::Hash(hash) => {
                Value::Mapping(
                    hash.into_iter()
                        .map(|(k, v)| {
                            let key: YamlWrap = k.into();
                            let value: YamlWrap = v.into();
                            (key.into(), value.into())
                        })
                        .collect(),
                )
            },
            Yaml::Alias(_) => unreachable!("alias unsupported"),
            Yaml::Null => Value::Null,
            Yaml::BadValue => unreachable!("bad value"),
        }
    }
}

/// Handle merge keys in a serde YAML document.
pub fn merge_keys_serde(doc: Value) -> Result<Value, MergeKeyError> {
    merge_keys(YamlWrap::into_yaml(doc.into()))
        .map(YamlWrap)
        .map(Into::into)
}

// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use serde::{Deserialize, Serialize};
use spdx_expression::SpdxExpression;

/// <https://spdx.github.io/spdx-spec/5-snippet-information/>
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, Clone, Default)]
pub struct Snippet {
    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#51-snippet-spdx-identifier>
    #[serde(rename = "SPDXID")]
    pub snippet_spdx_identifier: String,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#52-snippet-from-file-spdx-identifier>
    #[serde(rename = "snippetFromFile")]
    pub snippet_from_file_spdx_identifier: String,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#53-snippet-byte-range>
    pub ranges: Vec<Range>,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#55-snippet-concluded-license>
    #[serde(rename = "licenseConcluded")]
    pub snippet_concluded_license: SpdxExpression,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#56-license-information-in-snippet>
    #[serde(
        rename = "licenseInfoInSnippets",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub license_information_in_snippet: Vec<String>,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#57-snippet-comments-on-license>
    #[serde(
        rename = "licenseComments",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub snippet_comments_on_license: Option<String>,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#58-snippet-copyright-text>
    #[serde(rename = "copyrightText")]
    pub snippet_copyright_text: String,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#59-snippet-comment>
    #[serde(rename = "comment", skip_serializing_if = "Option::is_none", default)]
    pub snippet_comment: Option<String>,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#510-snippet-name>
    #[serde(rename = "name", skip_serializing_if = "Option::is_none", default)]
    pub snippet_name: Option<String>,

    /// <https://spdx.github.io/spdx-spec/5-snippet-information/#511-snippet-attribution-text>
    #[serde(
        rename = "attributionText",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub snippet_attribution_text: Option<String>,
}

/// <https://spdx.github.io/spdx-spec/5-snippet-information/#53-snippet-byte-range>
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Range {
    pub start_pointer: Pointer,
    pub end_pointer: Pointer,
}

impl Range {
    pub fn new(start_pointer: Pointer, end_pointer: Pointer) -> Self {
        Self {
            start_pointer,
            end_pointer,
        }
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, Clone)]
#[serde(untagged)]
pub enum Pointer {
    Byte {
        reference: Option<String>,
        offset: i32,
    },
    Line {
        reference: Option<String>,
        #[serde(rename = "lineNumber")]
        line_number: i32,
    },
}

impl Pointer {
    /// Create a new [`Pointer::Byte`].
    pub fn new_byte(reference: Option<String>, offset: i32) -> Self {
        Self::Byte { reference, offset }
    }

    /// Create a new [`Pointer::Line`].
    pub fn new_line(reference: Option<String>, line_number: i32) -> Self {
        Self::Line {
            reference,
            line_number,
        }
    }
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use crate::models::SPDX;

    use super::*;

    #[test]
    fn snippet_spdx_identifier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].snippet_spdx_identifier,
            "SPDXRef-Snippet".to_string()
        );
    }
    #[test]
    fn snippet_from_file_spdx_identifier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].snippet_from_file_spdx_identifier,
            "SPDXRef-DoapSource".to_string()
        );
    }
    #[test]
    fn ranges() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].ranges,
            vec![
                Range {
                    end_pointer: Pointer::Line {
                        line_number: 23,
                        reference: Some("SPDXRef-DoapSource".to_string()),
                    },
                    start_pointer: Pointer::Line {
                        line_number: 5,
                        reference: Some("SPDXRef-DoapSource".to_string()),
                    },
                },
                Range {
                    end_pointer: Pointer::Byte {
                        offset: 420,
                        reference: Some("SPDXRef-DoapSource".to_string()),
                    },
                    start_pointer: Pointer::Byte {
                        offset: 310,
                        reference: Some("SPDXRef-DoapSource".to_string()),
                    },
                },
            ]
        );
    }
    #[test]
    fn snippet_concluded_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].snippet_concluded_license,
            SpdxExpression::parse("GPL-2.0-only").unwrap()
        );
    }
    #[test]
    fn license_information_in_snippet() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].license_information_in_snippet,
            vec!["GPL-2.0-only".to_string()]
        );
    }
    #[test]
    fn snippet_comments_on_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
                    spdx.snippet_information[0].snippet_comments_on_license,
                    Some("The concluded license was taken from package xyz, from which the snippet was copied into the current file. The concluded license information was found in the COPYING.txt file in package xyz.".to_string())
                );
    }
    #[test]
    fn snippet_copyright_text() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].snippet_copyright_text,
            "Copyright 2008-2010 John Smith".to_string()
        );
    }
    #[test]
    fn snippet_comment() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
                    spdx.snippet_information[0].snippet_comment,
                    Some("This snippet was identified as significant and highlighted in this Apache-2.0 file, when a commercial scanner identified it as being derived from file foo.c in package xyz which is licensed under GPL-2.0.".to_string())
                );
    }
    #[test]
    fn snippet_name() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.snippet_information[0].snippet_name,
            Some("from linux kernel".to_string())
        );
    }
}

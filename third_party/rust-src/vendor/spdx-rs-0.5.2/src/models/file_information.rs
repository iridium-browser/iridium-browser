// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use serde::{Deserialize, Serialize};
use spdx_expression::{SimpleExpression, SpdxExpression};

use super::{Algorithm, Checksum};

/// ## File Information
///
/// SPDX's [File Information](https://spdx.github.io/spdx-spec/4-file-information/)
#[derive(Debug, Serialize, Deserialize, Clone, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct FileInformation {
    /// <https://spdx.github.io/spdx-spec/4-file-information/#41-file-name>
    pub file_name: String,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#42-file-spdx-identifier>
    #[serde(rename = "SPDXID")]
    pub file_spdx_identifier: String,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#43-file-type>
    #[serde(rename = "fileTypes", skip_serializing_if = "Vec::is_empty", default)]
    pub file_type: Vec<FileType>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#44-file-checksum>
    #[serde(rename = "checksums")]
    pub file_checksum: Vec<Checksum>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#45-concluded-license>
    #[serde(rename = "licenseConcluded")]
    pub concluded_license: SpdxExpression,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#46-license-information-in-file>
    #[serde(rename = "licenseInfoInFiles")]
    pub license_information_in_file: Vec<SimpleExpression>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#47-comments-on-license>
    #[serde(
        rename = "licenseComments",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub comments_on_license: Option<String>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#48-copyright-text>
    pub copyright_text: String,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#412-file-comment>
    #[serde(rename = "comment", skip_serializing_if = "Option::is_none", default)]
    pub file_comment: Option<String>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#413-file-notice>
    #[serde(
        rename = "noticeText",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub file_notice: Option<String>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#414-file-contributor>
    #[serde(
        rename = "fileContributors",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub file_contributor: Vec<String>,

    /// <https://spdx.github.io/spdx-spec/4-file-information/#415-file-attribution-text>
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub file_attribution_text: Option<Vec<String>>,
    // TODO: Snippet Information.
}

impl Default for FileInformation {
    fn default() -> Self {
        Self {
            file_name: "NOASSERTION".to_string(),
            file_spdx_identifier: "NOASSERTION".to_string(),
            file_type: Vec::new(),
            file_checksum: Vec::new(),
            concluded_license: SpdxExpression::parse("NOASSERTION").expect("will always succeed"),
            license_information_in_file: Vec::new(),
            comments_on_license: None,
            copyright_text: "NOASSERTION".to_string(),
            file_comment: None,
            file_notice: None,
            file_contributor: Vec::new(),
            file_attribution_text: None,
        }
    }
}

impl FileInformation {
    /// Create new file.
    pub fn new(name: &str, id: &mut i32) -> Self {
        *id += 1;
        Self {
            file_name: name.to_string(),
            file_spdx_identifier: format!("SPDXRef-{}", id),
            ..Self::default()
        }
    }

    /// Check if hash equals.
    pub fn equal_by_hash(&self, algorithm: Algorithm, value: &str) -> bool {
        let checksum = self
            .file_checksum
            .iter()
            .find(|&checksum| checksum.algorithm == algorithm);

        checksum.map_or(false, |checksum| {
            checksum.value.to_ascii_lowercase() == value.to_ascii_lowercase()
        })
    }

    /// Get checksum
    pub fn checksum(&self, algorithm: Algorithm) -> Option<&str> {
        let checksum = self
            .file_checksum
            .iter()
            .find(|&checksum| checksum.algorithm == algorithm);

        checksum.map(|checksum| checksum.value.as_str())
    }
}

/// <https://spdx.github.io/spdx-spec/4-file-information/#43-file-type>
#[derive(Debug, Serialize, Deserialize, PartialEq, Eq, PartialOrd, Clone, Copy)]
#[serde(rename_all = "UPPERCASE")]
pub enum FileType {
    Source,
    Binary,
    Archive,
    Application,
    Audio,
    Image,
    Text,
    Video,
    Documentation,
    SPDX,
    Other,
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use super::*;
    use crate::models::{Checksum, FileType, SPDX};

    #[test]
    fn checksum_equality() {
        let mut id = 1;
        let mut file_sha256 = FileInformation::new("sha256", &mut id);
        file_sha256
            .file_checksum
            .push(Checksum::new(Algorithm::SHA256, "test"));

        assert!(file_sha256.equal_by_hash(Algorithm::SHA256, "test"));
        assert!(!file_sha256.equal_by_hash(Algorithm::SHA256, "no_test"));

        let mut file_md5 = FileInformation::new("md5", &mut id);
        file_md5
            .file_checksum
            .push(Checksum::new(Algorithm::MD5, "test"));
        assert!(file_md5.equal_by_hash(Algorithm::MD5, "test"));
        assert!(!file_md5.equal_by_hash(Algorithm::MD5, "no_test"));
        assert!(!file_md5.equal_by_hash(Algorithm::SHA1, "test"));
    }

    #[test]
    fn get_checksum() {
        let mut id = 1;
        let mut file_sha256 = FileInformation::new("sha256", &mut id);
        file_sha256
            .file_checksum
            .push(Checksum::new(Algorithm::SHA256, "test"));

        assert_eq!(file_sha256.checksum(Algorithm::SHA256), Some("test"));
        assert_eq!(file_sha256.checksum(Algorithm::MD2), None);

        let mut file_md5 = FileInformation::new("md5", &mut id);
        file_md5
            .file_checksum
            .push(Checksum::new(Algorithm::MD5, "test"));

        assert_eq!(file_md5.checksum(Algorithm::MD5), Some("test"));
    }

    #[test]
    fn file_name() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[0].file_name,
            "./src/org/spdx/parser/DOAPProject.java"
        );
    }
    #[test]
    fn file_spdx_identifier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[0].file_spdx_identifier,
            "SPDXRef-DoapSource"
        );
    }
    #[test]
    fn file_type() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(spdx.file_information[0].file_type, vec![FileType::Source]);
    }
    #[test]
    fn file_checksum() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[0].file_checksum,
            vec![Checksum {
                algorithm: Algorithm::SHA1,
                value: "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12".to_string()
            }]
        );
    }
    #[test]
    fn concluded_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[0].concluded_license,
            SpdxExpression::parse("Apache-2.0").unwrap()
        );
    }
    #[test]
    fn license_information_in_file() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[0].license_information_in_file,
            vec![SimpleExpression::parse("Apache-2.0").unwrap()]
        );
    }
    #[test]
    fn comments_on_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[2].comments_on_license,
            Some("This license is used by Jena".to_string())
        );
    }
    #[test]
    fn copyright_text() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[0].copyright_text,
            "Copyright 2010, 2011 Source Auditor Inc.".to_string()
        );
    }
    #[test]
    fn file_comment() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[1].file_comment,
            Some("This file is used by Jena".to_string())
        );
    }
    #[test]
    fn file_notice() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
                    spdx.file_information[1].file_notice,
                    Some("Apache Commons Lang\nCopyright 2001-2011 The Apache Software Foundation\n\nThis product includes software developed by\nThe Apache Software Foundation (http://www.apache.org/).\n\nThis product includes software from the Spring Framework,\nunder the Apache License 2.0 (see: StringUtils.containsWhitespace())".to_string())
                );
    }
    #[test]
    fn file_contributor() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.file_information[1].file_contributor,
            vec!["Apache Software Foundation".to_string()]
        );
    }
}

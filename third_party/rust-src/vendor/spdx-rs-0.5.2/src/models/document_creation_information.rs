// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

use super::Checksum;

/// ## Document Creation Information
///
/// SPDX's [Document Creation Information](https://spdx.github.io/spdx-spec/2-document-creation-information/)
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct DocumentCreationInformation {
    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#21-spdx-version>
    pub spdx_version: String,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#22-data-license>
    pub data_license: String,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#23-spdx-identifier>
    #[serde(rename = "SPDXID")]
    pub spdx_identifier: String,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#24-document-name>
    #[serde(rename = "name")]
    pub document_name: String,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#25-spdx-document-namespace>
    #[serde(rename = "documentNamespace")]
    pub spdx_document_namespace: String,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#26-external-document-references>
    #[serde(
        rename = "externalDocumentRefs",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub external_document_references: Vec<ExternalDocumentReference>,

    pub creation_info: CreationInfo,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#211-document-comment>
    #[serde(rename = "comment", skip_serializing_if = "Option::is_none", default)]
    pub document_comment: Option<String>,

    /// Doesn't seem to be in spec, but the example contains it.
    /// <https://github.com/spdx/spdx-spec/issues/395>
    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub document_describes: Vec<String>,
}

impl Default for DocumentCreationInformation {
    fn default() -> Self {
        Self {
            // Current version is 2.2. Might need to support more verisons
            // in the future.
            spdx_version: "SPDX-2.2".to_string(),
            data_license: "CC0-1.0".to_string(),
            spdx_identifier: "SPDXRef-DOCUMENT".to_string(),
            document_name: "NOASSERTION".to_string(),
            spdx_document_namespace: "NOASSERTION".to_string(),
            external_document_references: Vec::new(),
            document_comment: None,
            creation_info: CreationInfo::default(),
            document_describes: Vec::new(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct CreationInfo {
    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#27-license-list-version>
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub license_list_version: Option<String>,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#28-creator>
    pub creators: Vec<String>,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#29-created>
    pub created: DateTime<Utc>,

    /// <https://spdx.github.io/spdx-spec/2-document-creation-information/#210-creator-comment>
    #[serde(skip_serializing_if = "Option::is_none", default)]
    #[serde(rename = "comment")]
    pub creator_comment: Option<String>,
}

impl Default for CreationInfo {
    fn default() -> Self {
        Self {
            license_list_version: None,
            creators: vec![
                "Person: Jane Doe ()".into(),
                "Organization: ExampleCodeInspect ()".into(),
                "Tool: LicenseFind-1.0".into(),
            ],
            created: chrono::offset::Utc::now(),
            creator_comment: None,
        }
    }
}

/// <https://spdx.github.io/spdx-spec/2-document-creation-information/#26-external-document-references>
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq, PartialOrd)]
pub struct ExternalDocumentReference {
    /// Unique ID string of the reference.
    #[serde(rename = "externalDocumentId")]
    pub id_string: String,

    /// Unique ID for the external document.
    #[serde(rename = "spdxDocument")]
    pub spdx_document_uri: String,

    /// Checksum of the external document following the checksum format defined
    /// in <https://spdx.github.io/spdx-spec/4-file-information/#44-file-checksum.>
    pub checksum: Checksum,
}

impl ExternalDocumentReference {
    pub const fn new(id_string: String, spdx_document_uri: String, checksum: Checksum) -> Self {
        Self {
            id_string,
            spdx_document_uri,
            checksum,
        }
    }
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use chrono::{TimeZone, Utc};

    use super::*;
    use crate::models::{Algorithm, SPDX};

    #[test]
    fn spdx_version() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();

        assert_eq!(
            spdx.document_creation_information.spdx_version,
            "SPDX-2.2".to_string()
        );
    }
    #[test]
    fn data_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();

        assert_eq!(spdx.document_creation_information.data_license, "CC0-1.0");
    }
    #[test]
    fn spdx_identifier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information.spdx_identifier,
            "SPDXRef-DOCUMENT".to_string()
        );
    }
    #[test]
    fn document_name() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information.document_name,
            "SPDX-Tools-v2.0".to_string()
        );
    }
    #[test]
    fn spdx_document_namespace() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information.spdx_document_namespace,
            "http://spdx.org/spdxdocs/spdx-example-444504E0-4F89-41D3-9A0C-0305E82C3301"
                .to_string()
        );
    }
    #[test]
    fn license_list_version() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information
                .creation_info
                .license_list_version,
            Some("3.9".to_string())
        );
    }
    #[test]
    fn creators() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert!(spdx
            .document_creation_information
            .creation_info
            .creators
            .contains(&"Tool: LicenseFind-1.0".to_string()));
        assert!(spdx
            .document_creation_information
            .creation_info
            .creators
            .contains(&"Organization: ExampleCodeInspect ()".to_string()));
        assert!(spdx
            .document_creation_information
            .creation_info
            .creators
            .contains(&"Person: Jane Doe ()".to_string()));
    }
    #[test]
    fn created() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information.creation_info.created,
            Utc.with_ymd_and_hms(2010, 1, 29, 18, 30, 22).unwrap()
        );
    }
    #[test]
    fn creator_comment() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information
                .creation_info
                .creator_comment,
            Some(
                r#"This package has been shipped in source and binary form.
The binaries were created with gcc 4.5.1 and expect to link to
compatible system run time libraries."#
                    .to_string()
            )
        );
    }
    #[test]
    fn document_comment() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.document_creation_information.document_comment,
            Some(
                "This document was created using SPDX 2.0 using licenses from the web site."
                    .to_string()
            )
        );
    }

    #[test]
    fn external_document_references() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert!(spdx
            .document_creation_information
            .external_document_references
            .contains(&ExternalDocumentReference {
                id_string: "DocumentRef-spdx-tool-1.2".to_string(),
                checksum: Checksum {
                    algorithm: Algorithm::SHA1,
                    value: "d6a770ba38583ed4bb4525bd96e50461655d2759".to_string()
                },
                spdx_document_uri:
                    "http://spdx.org/spdxdocs/spdx-tools-v1.2-3F2504E0-4F89-41D3-9A0C-0305E82C3301"
                        .to_string()
            }));
    }
}

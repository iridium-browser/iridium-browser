// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use serde::{Deserialize, Serialize};
use spdx_expression::SpdxExpression;

use super::Annotation;

use super::{Checksum, FileInformation};

/// ## Package Information
///
/// SPDX's [Package Information](https://spdx.github.io/spdx-spec/3-package-information/).
#[derive(Debug, Serialize, Deserialize, Clone, PartialEq, Eq)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PackageInformation {
    /// <https://spdx.github.io/spdx-spec/3-package-information/#31-package-name>
    #[serde(rename = "name")]
    pub package_name: String,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#32-package-spdx-identifier>
    #[serde(rename = "SPDXID")]
    pub package_spdx_identifier: String,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#33-package-version>
    #[serde(
        rename = "versionInfo",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub package_version: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#34-package-file-name>
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub package_file_name: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#35-package-supplier>
    #[serde(rename = "supplier", skip_serializing_if = "Option::is_none", default)]
    pub package_supplier: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#36-package-originator>
    #[serde(
        rename = "originator",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub package_originator: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#37-package-download-location>
    #[serde(rename = "downloadLocation")]
    pub package_download_location: String,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#38-files-analyzed>
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub files_analyzed: Option<bool>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#39-package-verification-code>
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub package_verification_code: Option<PackageVerificationCode>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#310-package-checksum>
    #[serde(rename = "checksums", skip_serializing_if = "Vec::is_empty", default)]
    pub package_checksum: Vec<Checksum>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#311-package-home-page>
    #[serde(rename = "homepage", skip_serializing_if = "Option::is_none", default)]
    pub package_home_page: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#312-source-information>
    #[serde(
        rename = "sourceInfo",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub source_information: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#313-concluded-license>
    #[serde(rename = "licenseConcluded")]
    pub concluded_license: SpdxExpression,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#314-all-licenses-information-from-files>
    #[serde(
        rename = "licenseInfoFromFiles",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub all_licenses_information_from_files: Vec<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#315-declared-license>
    #[serde(rename = "licenseDeclared")]
    pub declared_license: SpdxExpression,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#316-comments-on-license>
    #[serde(
        rename = "licenseComments",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub comments_on_license: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#317-copyright-text>
    pub copyright_text: String,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#318-package-summary-description>
    #[serde(rename = "summary", skip_serializing_if = "Option::is_none", default)]
    pub package_summary_description: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#319-package-detailed-description>
    #[serde(
        rename = "description",
        skip_serializing_if = "Option::is_none",
        default
    )]
    pub package_detailed_description: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#320-package-comment>
    #[serde(rename = "comment", skip_serializing_if = "Option::is_none", default)]
    pub package_comment: Option<String>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#321-external-reference>
    #[serde(
        rename = "externalRefs",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub external_reference: Vec<ExternalPackageReference>,

    /// <https://spdx.github.io/spdx-spec/3-package-information/#323-package-attribution-text>
    #[serde(
        rename = "attributionTexts",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub package_attribution_text: Vec<String>,

    /// List of "files in the package". Not sure which relationship type this maps to.
    /// Info: <https://github.com/spdx/spdx-spec/issues/487>
    // Valid SPDX?
    #[serde(rename = "hasFiles", skip_serializing_if = "Vec::is_empty", default)]
    pub files: Vec<String>,

    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub annotations: Vec<Annotation>,
}

impl Default for PackageInformation {
    fn default() -> Self {
        Self {
            package_name: "NOASSERTION".to_string(),
            package_spdx_identifier: "NOASSERTION".to_string(),
            package_version: None,
            package_file_name: None,
            package_supplier: None,
            package_originator: None,
            package_download_location: "NOASSERTION".to_string(),
            files_analyzed: None,
            package_verification_code: None,
            package_checksum: Vec::new(),
            package_home_page: None,
            source_information: None,
            concluded_license: SpdxExpression::parse("NONE").expect("will always succeed"),
            all_licenses_information_from_files: Vec::new(),
            declared_license: SpdxExpression::parse("NONE").expect("will always succeed"),
            comments_on_license: None,
            copyright_text: "NOASSERTION".to_string(),
            package_summary_description: None,
            package_detailed_description: None,
            package_comment: None,
            external_reference: Vec::new(),
            package_attribution_text: Vec::new(),
            files: Vec::new(),
            annotations: Vec::new(),
        }
    }
}

impl PackageInformation {
    /// Create new package.
    pub fn new(name: &str, id: &mut i32) -> Self {
        *id += 1;
        Self {
            package_name: name.to_string(),
            package_spdx_identifier: format!("SPDXRef-{}", id),
            ..Self::default()
        }
    }

    /// Find all files of the package.
    pub fn find_files_for_package<'a>(
        &'a self,
        files: &'a [FileInformation],
    ) -> Vec<&'a FileInformation> {
        self.files
            .iter()
            .filter_map(|file| {
                files
                    .iter()
                    .find(|file_information| &file_information.file_spdx_identifier == file)
            })
            .collect()
    }
}

/// <https://spdx.github.io/spdx-spec/3-package-information/#39-package-verification-code>
#[derive(Debug, Serialize, Deserialize, PartialEq, Eq, PartialOrd, Clone)]
pub struct PackageVerificationCode {
    /// Value of the verification code.
    #[serde(rename = "packageVerificationCodeValue")]
    pub value: String,

    /// Files that were excluded when calculating the verification code.
    #[serde(
        rename = "packageVerificationCodeExcludedFiles",
        skip_serializing_if = "Vec::is_empty",
        default
    )]
    pub excludes: Vec<String>,
}

impl PackageVerificationCode {
    pub fn new(value: String, excludes: Vec<String>) -> Self {
        Self { value, excludes }
    }
}

/// <https://spdx.github.io/spdx-spec/3-package-information/#321-external-reference>
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, PartialOrd, Clone)]
#[serde(rename_all = "camelCase")]
pub struct ExternalPackageReference {
    pub reference_category: ExternalPackageReferenceCategory,
    pub reference_type: String,
    pub reference_locator: String,
    #[serde(rename = "comment")]
    #[serde(skip_serializing_if = "Option::is_none")]
    #[serde(default)]
    pub reference_comment: Option<String>,
}

impl ExternalPackageReference {
    pub const fn new(
        reference_category: ExternalPackageReferenceCategory,
        reference_type: String,
        reference_locator: String,
        reference_comment: Option<String>,
    ) -> Self {
        Self {
            reference_category,
            reference_type,
            reference_locator,
            reference_comment,
        }
    }
}

/// <https://spdx.github.io/spdx-spec/3-package-information/#321-external-reference>
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, PartialOrd, Clone)]
#[serde(rename_all = "SCREAMING-KEBAB-CASE")]
pub enum ExternalPackageReferenceCategory {
    Security,
    PackageManager,
    PersistentID,
    Other,
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use crate::models::{Algorithm, SPDX};

    use super::*;

    #[test]
    fn all_packages_are_deserialized() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(spdx.package_information.len(), 4);
    }
    #[test]
    fn package_name() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_name,
            "glibc".to_string()
        );
    }
    #[test]
    fn package_spdx_identifier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_spdx_identifier,
            "SPDXRef-Package".to_string()
        );
    }
    #[test]
    fn package_version() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_version,
            Some("2.11.1".to_string())
        );
    }
    #[test]
    fn package_file_name() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_file_name,
            Some("glibc-2.11.1.tar.gz".to_string())
        );
    }
    #[test]
    fn package_supplier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_supplier,
            Some("Person: Jane Doe (jane.doe@example.com)".to_string())
        );
    }
    #[test]
    fn package_originator() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_originator,
            Some("Organization: ExampleCodeInspect (contact@example.com)".to_string())
        );
    }
    #[test]
    fn package_download_location() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_download_location,
            "http://ftp.gnu.org/gnu/glibc/glibc-ports-2.15.tar.gz".to_string()
        );
    }
    #[test]
    fn files_analyzed() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(spdx.package_information[0].files_analyzed, Some(true));
    }
    #[test]
    fn package_verification_code() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_verification_code,
            Some(PackageVerificationCode {
                value: "d6a770ba38583ed4bb4525bd96e50461655d2758".to_string(),
                excludes: vec!["./package.spdx".to_string()]
            })
        );
    }
    #[test]
    fn package_chekcsum() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert!(spdx.package_information[0]
            .package_checksum
            .contains(&Checksum::new(
                Algorithm::SHA1,
                "85ed0817af83a24ad8da68c2b5094de69833983c"
            )));
        assert!(spdx.package_information[0]
            .package_checksum
            .contains(&Checksum::new(
                Algorithm::MD5,
                "624c1abb3664f4b35547e7c73864ad24"
            )));
        assert!(spdx.package_information[0]
            .package_checksum
            .contains(&Checksum::new(
                Algorithm::SHA256,
                "11b6d3ee554eedf79299905a98f9b9a04e498210b59f15094c916c91d150efcd"
            )));
    }
    #[test]
    fn package_home_page() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_home_page,
            Some("http://ftp.gnu.org/gnu/glibc".to_string())
        );
    }
    #[test]
    fn source_information() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].source_information,
            Some("uses glibc-2_11-branch from git://sourceware.org/git/glibc.git.".to_string())
        );
    }
    #[test]
    fn concluded_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].concluded_license,
            SpdxExpression::parse("(LGPL-2.0-only OR LicenseRef-3)").unwrap()
        );
    }
    #[test]
    fn all_licenses_information_from_files() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert!(spdx.package_information[0]
            .all_licenses_information_from_files
            .contains(&"GPL-2.0-only".to_string()));
        assert!(spdx.package_information[0]
            .all_licenses_information_from_files
            .contains(&"LicenseRef-2".to_string()));
        assert!(spdx.package_information[0]
            .all_licenses_information_from_files
            .contains(&"LicenseRef-1".to_string()));
    }
    #[test]
    fn declared_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].declared_license,
            SpdxExpression::parse("(LGPL-2.0-only AND LicenseRef-3)").unwrap()
        );
    }
    #[test]
    fn comments_on_license() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
                    spdx.package_information[0].comments_on_license,
                    Some("The license for this project changed with the release of version x.y.  The version of the project included here post-dates the license change.".to_string())
                );
    }
    #[test]
    fn copyright_text() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].copyright_text,
            "Copyright 2008-2010 John Smith".to_string()
        );
    }
    #[test]
    fn package_summary_description() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[0].package_summary_description,
            Some("GNU C library.".to_string())
        );
    }
    #[test]
    fn package_detailed_description() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
                    spdx.package_information[0].package_detailed_description,
                    Some("The GNU C Library defines functions that are specified by the ISO C standard, as well as additional features specific to POSIX and other derivatives of the Unix operating system, and extensions specific to GNU systems.".to_string())
                );
    }
    #[test]
    fn package_comment() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.package_information[1].package_comment,
            Some("This package was converted from a DOAP Project by the same name".to_string())
        );
    }
    #[test]
    fn external_reference() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert!(
                    spdx.package_information[0].external_reference.contains(&ExternalPackageReference {
                        reference_comment: Some("This is the external ref for Acme".to_string()),
                        reference_category: ExternalPackageReferenceCategory::Other,
                        reference_locator: "acmecorp/acmenator/4.1.3-alpha".to_string(),
                        reference_type: "http://spdx.org/spdxdocs/spdx-example-444504E0-4F89-41D3-9A0C-0305E82C3301#LocationRef-acmeforge".to_string()
                    })
                );
        assert!(spdx.package_information[0].external_reference.contains(
            &ExternalPackageReference {
                reference_comment: None,
                reference_category: ExternalPackageReferenceCategory::Security,
                reference_locator:
                    "cpe:2.3:a:pivotal_software:spring_framework:4.1.0:*:*:*:*:*:*:*".to_string(),
                reference_type: "http://spdx.org/rdf/references/cpe23Type".to_string()
            }
        ));
    }
    #[test]
    fn package_attribution_text() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert!(
                    spdx.package_information[0].package_attribution_text.contains(&"The GNU C Library is free software.  See the file COPYING.LIB for copying conditions, and LICENSES for notices about a few contributions that require these additional notices to be distributed.  License copyright years may be listed using range notation, e.g., 1996-2015, indicating that every year in the range, inclusive, is a copyrightable year that would otherwise be listed individually.".to_string())
                );
    }
}

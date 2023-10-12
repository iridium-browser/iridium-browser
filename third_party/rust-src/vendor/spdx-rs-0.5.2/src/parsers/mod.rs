// SPDX-FileCopyrightText: 2021 HH Partners
//
// SPDX-License-Identifier: MIT

//! Parsers for deserializing [`SPDX`] from different data formats.
//!
//! The SPDX spec supports some data formats that are not supported by [Serde], so parsing from JSON
//! (and YAML) is achieved with the data format specific crates:
//!
//! ```rust
//! # use spdx_rs::error::SpdxError;
//! use spdx_rs::models::SPDX;
//! # fn main() -> Result<(), SpdxError> {
//!
//! let spdx_file = std::fs::read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json")?;
//! let spdx_document: SPDX = serde_json::from_str(&spdx_file).unwrap();
//!
//! assert_eq!(
//!     spdx_document.document_creation_information.document_name,
//!     "SPDX-Tools-v2.0"
//! );
//! # Ok(())
//! # }
//! ```
//!
//! [Serde]: https://serde.rs

use std::collections::HashSet;

use chrono::{DateTime, Utc};
use spdx_expression::{SimpleExpression, SpdxExpression};

use crate::{
    error::SpdxError,
    models::{
        Annotation, AnnotationType, DocumentCreationInformation, ExternalPackageReference,
        FileInformation, OtherLicensingInformationDetected, PackageInformation, Pointer, Range,
        Relationship, Snippet, SPDX,
    },
    parsers::tag_value::{atoms, Atom},
};

mod tag_value;

/// Parse a tag-value SPDX document to [`SPDX`].
///
/// # Usage
///
/// ```
/// # use spdx_rs::error::SpdxError;
/// use spdx_rs::parsers::spdx_from_tag_value;
/// # fn main() -> Result<(), SpdxError> {
///
/// let spdx_file = std::fs::read_to_string("tests/data/SPDXTagExample-v2.2.spdx")?;
/// let spdx_document = spdx_from_tag_value(&spdx_file)?;
///
/// assert_eq!(
///     spdx_document.document_creation_information.document_name,
///     "SPDX-Tools-v2.0"
/// );
/// # Ok(())
/// # }
/// ```
///
/// # Errors
///
/// - If parsing of the tag-value fails.
/// - If parsing of some of the values fail.
pub fn spdx_from_tag_value(input: &str) -> Result<SPDX, SpdxError> {
    let (_, atoms) = atoms(input).map_err(|err| SpdxError::TagValueParse(err.to_string()))?;

    let spdx = spdx_from_atoms(&atoms)?;

    Ok(spdx)
}

#[allow(clippy::cognitive_complexity, clippy::too_many_lines)]
fn spdx_from_atoms(atoms: &[Atom]) -> Result<SPDX, SpdxError> {
    let mut document_creation_information_in_progress =
        Some(DocumentCreationInformation::default());
    let mut document_creation_information_final: Option<DocumentCreationInformation> = None;

    let mut package_information: Vec<PackageInformation> = Vec::new();
    let mut package_in_progress: Option<PackageInformation> = None;
    let mut external_package_ref_in_progress: Option<ExternalPackageReference> = None;

    let mut other_licensing_information_detected: Vec<OtherLicensingInformationDetected> =
        Vec::new();
    let mut license_info_in_progress: Option<OtherLicensingInformationDetected> = None;

    let mut file_information: Vec<FileInformation> = Vec::new();
    let mut file_in_progress: Option<FileInformation> = None;

    let mut snippet_information: Vec<Snippet> = Vec::new();
    let mut snippet_in_progress: Option<Snippet> = None;

    let mut relationships: HashSet<Relationship> = HashSet::new();
    let mut relationship_in_progress: Option<Relationship> = None;

    let mut annotations: Vec<Annotation> = Vec::new();
    let mut annotation_in_progress = AnnotationInProgress::default();

    for atom in atoms {
        let document_creation_information = process_atom_for_document_creation_information(
            atom,
            &mut document_creation_information_in_progress,
        )?;
        if let Some(document_creation_information) = document_creation_information {
            document_creation_information_final = Some(document_creation_information);
            document_creation_information_in_progress = None;
        }
        process_atom_for_packages(
            atom,
            &mut package_information,
            &mut package_in_progress,
            &mut external_package_ref_in_progress,
        );
        process_atom_for_files(
            atom,
            &mut file_in_progress,
            &mut file_information,
            &package_in_progress,
            &mut relationships,
        );
        process_atom_for_snippets(atom, &mut snippet_information, &mut snippet_in_progress);
        process_atom_for_relationships(atom, &mut relationships, &mut relationship_in_progress);
        process_atom_for_annotations(atom, &mut annotations, &mut annotation_in_progress)?;
        process_atom_for_license_info(
            atom,
            &mut other_licensing_information_detected,
            &mut license_info_in_progress,
        )?;
    }
    if let Some(file) = file_in_progress {
        file_information.push(file);
    }
    if let Some(snippet) = &mut snippet_in_progress {
        snippet_information.push(snippet.clone());
    }

    if let Some(package) = package_in_progress {
        package_information.push(package);
    }

    if let Some(relationship) = relationship_in_progress {
        relationships.insert(relationship);
    }

    if let Some(license_info) = license_info_in_progress {
        other_licensing_information_detected.push(license_info);
    }

    if document_creation_information_in_progress.is_some() {
        document_creation_information_final = document_creation_information_in_progress;
    }

    process_annotation(&mut annotation_in_progress, &mut annotations);

    Ok(SPDX {
        document_creation_information: document_creation_information_final
            // TODO: Proper error handling
            .expect("If this doesn't exist, the document is not valid."),
        package_information,
        other_licensing_information_detected,
        file_information,
        snippet_information,
        relationships: relationships.into_iter().collect(),
        annotations,
        // TODO: This should probably be removed.
        spdx_ref_counter: 0,
    })
}

fn process_atom_for_document_creation_information(
    atom: &Atom,
    mut document_creation_information_in_progress: &mut Option<DocumentCreationInformation>,
) -> Result<Option<DocumentCreationInformation>, SpdxError> {
    // Get document creation information.
    let mut final_creation_information = None;
    match atom {
        Atom::SpdxVersion(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.spdx_version = value.to_string();
            }
        }
        Atom::DataLicense(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.data_license = value.to_string();
            }
        }
        Atom::SPDXID(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.spdx_identifier = value.to_string();
            }
        }
        Atom::DocumentName(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.document_name = value.to_string();
            }
        }
        Atom::DocumentNamespace(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.spdx_document_namespace = value.to_string();
            }
        }
        Atom::ExternalDocumentRef(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information
                    .external_document_references
                    .push(value.clone());
            }
        }
        Atom::LicenseListVersion(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information
                    .creation_info
                    .license_list_version = Some(value.to_string());
            }
        }
        Atom::Creator(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information
                    .creation_info
                    .creators
                    .push(value.to_string());
            }
        }
        Atom::Created(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.creation_info.created =
                    DateTime::parse_from_rfc3339(value)?.with_timezone(&Utc);
            }
        }
        Atom::CreatorComment(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.creation_info.creator_comment =
                    Some(value.to_string());
            }
        }
        Atom::DocumentComment(value) => {
            if let Some(document_creation_information) =
                &mut document_creation_information_in_progress
            {
                document_creation_information.document_comment = Some(value.to_string());
            }
        }
        Atom::TVComment(_) => {}
        _ => {
            if let Some(document_creation_information) = document_creation_information_in_progress {
                final_creation_information = Some(document_creation_information.clone());
            }
        }
    }
    Ok(final_creation_information)
}

#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn process_atom_for_packages(
    atom: &Atom,
    packages: &mut Vec<PackageInformation>,
    mut package_in_progress: &mut Option<PackageInformation>,
    mut external_package_ref_in_progress: &mut Option<ExternalPackageReference>,
) {
    match atom {
        Atom::PackageName(value) => {
            if let Some(package) = &mut package_in_progress {
                if let Some(pkg_ref) = &mut external_package_ref_in_progress {
                    package.external_reference.push(pkg_ref.clone());
                    *external_package_ref_in_progress = None;
                }
                packages.push(package.clone());
            }
            *package_in_progress = Some(PackageInformation::default());

            if let Some(package) = &mut package_in_progress {
                package.package_name = value.to_string();
            }
        }
        Atom::SPDXID(value) => {
            if let Some(package) = &mut package_in_progress {
                if package.package_spdx_identifier == "NOASSERTION" {
                    package.package_spdx_identifier = value.to_string();
                }
            }
        }
        Atom::PackageVersion(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_version = Some(value.to_string());
            }
        }
        Atom::PackageFileName(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_file_name = Some(value.to_string());
            }
        }
        Atom::PackageSupplier(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_supplier = Some(value.to_string());
            }
        }
        Atom::PackageOriginator(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_originator = Some(value.to_string());
            }
        }
        Atom::PackageDownloadLocation(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_download_location = value.to_string();
            }
        }
        Atom::PackageVerificationCode(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_verification_code = Some(value.clone());
            }
        }
        Atom::PackageChecksum(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_checksum.push(value.clone());
            }
        }
        Atom::PackageHomePage(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_home_page = Some(value.clone());
            }
        }
        Atom::PackageSourceInfo(value) => {
            if let Some(package) = &mut package_in_progress {
                package.source_information = Some(value.clone());
            }
        }
        Atom::PackageLicenseConcluded(value) => {
            if let Some(package) = &mut package_in_progress {
                package.concluded_license = SpdxExpression::parse(value).unwrap();
            }
        }
        Atom::PackageLicenseInfoFromFiles(value) => {
            if let Some(package) = &mut package_in_progress {
                package
                    .all_licenses_information_from_files
                    .push(value.clone());
            }
        }
        Atom::PackageLicenseDeclared(value) => {
            if let Some(package) = &mut package_in_progress {
                package.declared_license = SpdxExpression::parse(value).unwrap();
            }
        }
        Atom::PackageLicenseComments(value) => {
            if let Some(package) = &mut package_in_progress {
                package.comments_on_license = Some(value.clone());
            }
        }
        Atom::PackageCopyrightText(value) => {
            if let Some(package) = &mut package_in_progress {
                package.copyright_text = value.clone();
            }
        }
        Atom::PackageSummary(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_summary_description = Some(value.clone());
            }
        }
        Atom::PackageDescription(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_detailed_description = Some(value.clone());
            }
        }
        Atom::PackageAttributionText(value) => {
            if let Some(package) = &mut package_in_progress {
                package.package_attribution_text.push(value.clone());
            }
        }
        Atom::ExternalRef(value) => {
            if let Some(pkg_ref) = &mut external_package_ref_in_progress {
                if let Some(package) = &mut package_in_progress {
                    package.external_reference.push(pkg_ref.clone());
                }
            }
            *external_package_ref_in_progress = Some(value.clone());
        }
        Atom::ExternalRefComment(value) => {
            if let Some(pkg_ref) = &mut external_package_ref_in_progress {
                pkg_ref.reference_comment = Some(value.clone());
            }
        }
        _ => {}
    }
}

fn process_atom_for_files(
    atom: &Atom,
    mut file_in_progress: &mut Option<FileInformation>,
    files: &mut Vec<FileInformation>,
    package_in_progress: &Option<PackageInformation>,
    relationships: &mut HashSet<Relationship>,
) {
    match atom {
        Atom::PackageName(_) => {
            if let Some(file) = &mut file_in_progress {
                files.push(file.clone());
                *file_in_progress = None;
            }
        }
        Atom::FileName(value) => {
            if let Some(file) = &mut file_in_progress {
                files.push(file.clone());
            }
            *file_in_progress = Some(FileInformation::default());

            if let Some(file) = &mut file_in_progress {
                file.file_name = value.to_string();
            }
        }
        Atom::SPDXID(value) => {
            if let Some(file) = &mut file_in_progress {
                file.file_spdx_identifier = value.to_string();
                if let Some(package) = package_in_progress {
                    relationships.insert(Relationship::new(
                        &package.package_spdx_identifier,
                        value,
                        crate::models::RelationshipType::Contains,
                        None,
                    ));
                }
            }
        }
        Atom::FileComment(value) => {
            if let Some(file) = &mut file_in_progress {
                file.file_comment = Some(value.to_string());
            }
        }
        Atom::FileType(value) => {
            if let Some(file) = &mut file_in_progress {
                file.file_type.push(*value);
            }
        }
        Atom::FileChecksum(value) => {
            if let Some(file) = &mut file_in_progress {
                file.file_checksum.push(value.clone());
            }
        }
        Atom::LicenseConcluded(value) => {
            if let Some(file) = &mut file_in_progress {
                file.concluded_license = SpdxExpression::parse(value).unwrap();
            }
        }
        Atom::LicenseInfoInFile(value) => {
            if let Some(file) = &mut file_in_progress {
                file.license_information_in_file
                    .push(SimpleExpression::parse(value).unwrap());
            }
        }
        Atom::LicenseComments(value) => {
            if let Some(file) = &mut file_in_progress {
                file.comments_on_license = Some(value.clone());
            }
        }
        Atom::FileCopyrightText(value) => {
            if let Some(file) = &mut file_in_progress {
                file.copyright_text = value.clone();
            }
        }
        Atom::FileNotice(value) => {
            if let Some(file) = &mut file_in_progress {
                file.file_notice = Some(value.clone());
            }
        }
        Atom::FileContributor(value) => {
            if let Some(file) = &mut file_in_progress {
                file.file_contributor.push(value.clone());
            }
        }
        _ => {}
    }
}

fn process_atom_for_snippets(
    atom: &Atom,
    snippets: &mut Vec<Snippet>,
    mut snippet_in_progress: &mut Option<Snippet>,
) {
    match atom {
        Atom::SnippetSPDXID(value) => {
            if let Some(snippet) = &snippet_in_progress {
                snippets.push(snippet.clone());
            }

            *snippet_in_progress = Some(Snippet::default());
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_spdx_identifier = value.to_string();
            }
        }
        Atom::SnippetFromFileSPDXID(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_from_file_spdx_identifier = value.to_string();
            }
        }
        Atom::SnippetByteRange(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                let start_pointer = Pointer::new_byte(None, value.0);
                let end_pointer = Pointer::new_byte(None, value.1);
                let range = Range::new(start_pointer, end_pointer);
                snippet.ranges.push(range);
            }
        }
        Atom::SnippetLineRange(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                let start_pointer = Pointer::new_line(None, value.0);
                let end_pointer = Pointer::new_line(None, value.1);
                let range = Range::new(start_pointer, end_pointer);
                snippet.ranges.push(range);
            }
        }
        Atom::SnippetLicenseConcluded(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_concluded_license = SpdxExpression::parse(value).unwrap();
            }
        }
        Atom::LicenseInfoInSnippet(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet
                    .license_information_in_snippet
                    .push(value.to_string());
            }
        }
        Atom::SnippetLicenseComments(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_comments_on_license = Some(value.to_string());
            }
        }
        Atom::SnippetCopyrightText(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_copyright_text = value.to_string();
            }
        }
        Atom::SnippetComment(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_comment = Some(value.to_string());
            }
        }
        Atom::SnippetName(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_name = Some(value.to_string());
            }
        }
        Atom::SnippetAttributionText(value) => {
            if let Some(snippet) = &mut snippet_in_progress {
                snippet.snippet_attribution_text = Some(value.to_string());
            }
        }
        _ => {}
    }
}

#[allow(clippy::unnecessary_wraps)]
fn process_atom_for_relationships(
    atom: &Atom,
    relationships: &mut HashSet<Relationship>,
    mut relationship_in_progress: &mut Option<Relationship>,
) {
    match atom {
        Atom::Relationship(value) => {
            if let Some(relationship) = relationship_in_progress {
                relationships.insert(relationship.clone());
            }
            *relationship_in_progress = Some(value.clone());
        }
        Atom::RelationshipComment(value) => {
            if let Some(relationship) = &mut relationship_in_progress {
                relationship.comment = Some(value.to_string());
            }
        }
        _ => {}
    }
}

#[derive(Debug, Default)]
struct AnnotationInProgress {
    annotator_in_progress: Option<String>,
    date_in_progress: Option<DateTime<Utc>>,
    comment_in_progress: Option<String>,
    type_in_progress: Option<AnnotationType>,
    spdxref_in_progress: Option<String>,
}

fn process_annotation(
    mut annotation_in_progress: &mut AnnotationInProgress,

    annotations: &mut Vec<Annotation>,
) {
    if let AnnotationInProgress {
        annotator_in_progress: Some(annotator),
        date_in_progress: Some(date),
        comment_in_progress: Some(comment),
        type_in_progress: Some(annotation_type),
        spdxref_in_progress: Some(spdxref),
    } = &mut annotation_in_progress
    {
        let annotation = Annotation::new(
            annotator.clone(),
            *date,
            *annotation_type,
            Some(spdxref.clone()),
            comment.clone(),
        );
        *annotation_in_progress = AnnotationInProgress {
            annotator_in_progress: None,
            comment_in_progress: None,
            date_in_progress: None,
            spdxref_in_progress: None,
            type_in_progress: None,
        };
        annotations.push(annotation);
    }
}

fn process_atom_for_annotations(
    atom: &Atom,
    annotations: &mut Vec<Annotation>,
    mut annotation_in_progress: &mut AnnotationInProgress,
) -> Result<(), SpdxError> {
    process_annotation(annotation_in_progress, annotations);

    match atom {
        Atom::Annotator(value) => {
            annotation_in_progress.annotator_in_progress = Some(value.clone());
        }
        Atom::AnnotationDate(value) => {
            annotation_in_progress.date_in_progress =
                Some(DateTime::parse_from_rfc3339(value)?.with_timezone(&Utc));
        }
        Atom::AnnotationComment(value) => {
            annotation_in_progress.comment_in_progress = Some(value.clone());
        }
        Atom::AnnotationType(value) => {
            annotation_in_progress.type_in_progress = Some(*value);
        }
        Atom::SPDXREF(value) => {
            annotation_in_progress.spdxref_in_progress = Some(value.clone());
        }
        _ => {}
    }

    Ok(())
}

#[allow(clippy::unnecessary_wraps)]
fn process_atom_for_license_info(
    atom: &Atom,
    license_infos: &mut Vec<OtherLicensingInformationDetected>,
    mut license_info_in_progress: &mut Option<OtherLicensingInformationDetected>,
) -> Result<(), SpdxError> {
    match atom {
        Atom::LicenseID(value) => {
            if let Some(license_info) = &mut license_info_in_progress {
                license_infos.push(license_info.clone());
            }
            *license_info_in_progress = Some(OtherLicensingInformationDetected::default());

            if let Some(license_info) = &mut license_info_in_progress {
                license_info.license_identifier = value.to_string();
            }
        }
        Atom::ExtractedText(value) => {
            if let Some(license_info) = &mut license_info_in_progress {
                license_info.extracted_text = value.to_string();
            }
        }
        Atom::LicenseName(value) => {
            if let Some(license_info) = &mut license_info_in_progress {
                license_info.license_name = value.to_string();
            }
        }
        Atom::LicenseCrossReference(value) => {
            if let Some(license_info) = &mut license_info_in_progress {
                license_info.license_cross_reference.push(value.to_string());
            }
        }
        Atom::LicenseComment(value) => {
            if let Some(license_info) = &mut license_info_in_progress {
                license_info.license_comment = Some(value.to_string());
            }
        }
        _ => {}
    }

    Ok(())
}

#[cfg(test)]
#[allow(clippy::too_many_lines)]
mod test_super {
    use std::{fs::read_to_string, iter::FromIterator};

    use chrono::TimeZone;

    use crate::models::{
        Algorithm, Checksum, ExternalDocumentReference, ExternalPackageReferenceCategory, FileType,
    };

    use super::*;

    #[test]
    fn whole_spdx_is_parsed() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        assert_eq!(spdx.package_information.len(), 4);
        assert_eq!(spdx.file_information.len(), 4);
    }

    #[test]
    fn spdx_creation_info_is_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let document_creation_information = spdx.document_creation_information;
        assert_eq!(document_creation_information.spdx_version, "SPDX-2.2");
        assert_eq!(document_creation_information.data_license, "CC0-1.0");
        assert_eq!(
            document_creation_information.spdx_document_namespace,
            "http://spdx.org/spdxdocs/spdx-example-444504E0-4F89-41D3-9A0C-0305E82C3301"
        );
        assert_eq!(
            document_creation_information.document_name,
            "SPDX-Tools-v2.0"
        );
        assert_eq!(
            document_creation_information.spdx_identifier,
            "SPDXRef-DOCUMENT"
        );
        assert_eq!(
            document_creation_information.document_comment,
            Some(
                "This document was created using SPDX 2.0 using licenses from the web site."
                    .to_string()
            )
        );
        assert_eq!(
            document_creation_information.external_document_references,
            vec![ExternalDocumentReference::new(
                "spdx-tool-1.2".to_string(),
                "http://spdx.org/spdxdocs/spdx-tools-v1.2-3F2504E0-4F89-41D3-9A0C-0305E82C3301"
                    .to_string(),
                Checksum::new(Algorithm::SHA1, "d6a770ba38583ed4bb4525bd96e50461655d2759")
            )]
        );
        assert!(document_creation_information
            .creation_info
            .creators
            .contains(&"Tool: LicenseFind-1.0".to_string()));
        assert!(document_creation_information
            .creation_info
            .creators
            .contains(&"Organization: ExampleCodeInspect ()".to_string()));
        assert!(document_creation_information
            .creation_info
            .creators
            .contains(&"Person: Jane Doe ()".to_string()));
        assert_eq!(
            document_creation_information.creation_info.created,
            Utc.with_ymd_and_hms(2010, 1, 29, 18, 30, 22).unwrap()
        );
        assert_eq!(
            document_creation_information.creation_info.creator_comment,
            Some(
                "This package has been shipped in source and binary form.
The binaries were created with gcc 4.5.1 and expect to link to
compatible system run time libraries."
                    .to_string()
            )
        );
        assert_eq!(
            document_creation_information
                .creation_info
                .license_list_version,
            Some("3.9".to_string())
        );
    }

    #[test]
    fn package_info_is_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let packages = spdx.package_information;
        assert_eq!(packages.len(), 4);

        let glibc = packages.iter().find(|p| p.package_name == "glibc").unwrap();
        assert_eq!(glibc.package_spdx_identifier, "SPDXRef-Package");
        assert_eq!(glibc.package_version, Some("2.11.1".to_string()));
        assert_eq!(
            glibc.package_file_name,
            Some("glibc-2.11.1.tar.gz".to_string())
        );
        assert_eq!(
            glibc.package_supplier,
            Some("Person: Jane Doe (jane.doe@example.com)".to_string())
        );
        assert_eq!(
            glibc.package_originator,
            Some("Organization: ExampleCodeInspect (contact@example.com)".to_string())
        );
        assert_eq!(
            glibc.package_download_location,
            "http://ftp.gnu.org/gnu/glibc/glibc-ports-2.15.tar.gz".to_string()
        );
        assert_eq!(
            glibc.package_verification_code.as_ref().unwrap().value,
            "d6a770ba38583ed4bb4525bd96e50461655d2758".to_string()
        );
        assert_eq!(
            glibc.package_verification_code.as_ref().unwrap().excludes,
            vec!["./package.spdx"]
        );
        assert_eq!(
            glibc.package_checksum,
            vec![
                Checksum::new(Algorithm::MD5, "624c1abb3664f4b35547e7c73864ad24"),
                Checksum::new(Algorithm::SHA1, "85ed0817af83a24ad8da68c2b5094de69833983c"),
                Checksum::new(
                    Algorithm::SHA256,
                    "11b6d3ee554eedf79299905a98f9b9a04e498210b59f15094c916c91d150efcd"
                ),
            ]
        );
        assert_eq!(
            glibc.package_home_page,
            Some("http://ftp.gnu.org/gnu/glibc".to_string())
        );
        assert_eq!(
            glibc.source_information,
            Some("uses glibc-2_11-branch from git://sourceware.org/git/glibc.git.".to_string())
        );
        assert_eq!(
            glibc.concluded_license.identifiers(),
            HashSet::from_iter(["LGPL-2.0-only".to_string(), "LicenseRef-3".to_string()])
        );
        assert_eq!(
            glibc.all_licenses_information_from_files,
            vec!["GPL-2.0-only", "LicenseRef-2", "LicenseRef-1"]
        );
        assert_eq!(
            glibc.declared_license.identifiers(),
            HashSet::from_iter(["LGPL-2.0-only".to_string(), "LicenseRef-3".to_string()])
        );
        assert_eq!(glibc.comments_on_license, Some("The license for this project changed with the release of version x.y.  The version of the project included here post-dates the license change.".to_string()));
        assert_eq!(glibc.copyright_text, "Copyright 2008-2010 John Smith");
        assert_eq!(
            glibc.package_summary_description,
            Some("GNU C library.".to_string())
        );
        assert_eq!(glibc.package_detailed_description, Some("The GNU C Library defines functions that are specified by the ISO C standard, as well as additional features specific to POSIX and other derivatives of the Unix operating system, and extensions specific to GNU systems.".to_string()));
        assert_eq!(
            glibc.package_attribution_text,
            vec!["The GNU C Library is free software.  See the file COPYING.LIB for copying conditions, and LICENSES for notices about a few contributions that require these additional notices to be distributed.  License copyright years may be listed using range notation, e.g., 1996-2015, indicating that every year in the range, inclusive, is a copyrightable year that would otherwise be listed individually.".to_string()]
        );
        assert_eq!(
            glibc.external_reference,
            vec![
                ExternalPackageReference::new(
                    ExternalPackageReferenceCategory::Security,
                    "cpe23Type".to_string(),
                    "cpe:2.3:a:pivotal_software:spring_framework:4.1.0:*:*:*:*:*:*:*".to_string(),
                    None
                ),
                ExternalPackageReference::new(
                    ExternalPackageReferenceCategory::Other,
                    "LocationRef-acmeforge".to_string(),
                    "acmecorp/acmenator/4.1.3-alpha".to_string(),
                    Some("This is the external ref for Acme".to_string())
                ),
            ]
        );
        let jena = packages.iter().find(|p| p.package_name == "Jena").unwrap();
        assert_eq!(jena.package_spdx_identifier, "SPDXRef-fromDoap-0");
        assert_eq!(
            jena.external_reference,
            vec![ExternalPackageReference::new(
                ExternalPackageReferenceCategory::PackageManager,
                "purl".to_string(),
                "pkg:maven/org.apache.jena/apache-jena@3.12.0".to_string(),
                None
            ),]
        );
    }

    #[test]
    fn file_info_is_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let files = spdx.file_information;
        assert_eq!(files.len(), 4);

        let fooc = files
            .iter()
            .find(|p| p.file_name == "./package/foo.c")
            .unwrap();
        assert_eq!(fooc.file_spdx_identifier, "SPDXRef-File");
        assert_eq!(fooc.file_comment, Some("The concluded license was taken from the package level that the file was included in.
This information was found in the COPYING.txt file in the xyz directory.".to_string()));
        assert_eq!(fooc.file_type, vec![FileType::Source]);
        assert_eq!(
            fooc.file_checksum,
            vec![
                Checksum::new(Algorithm::SHA1, "d6a770ba38583ed4bb4525bd96e50461655d2758"),
                Checksum::new(Algorithm::MD5, "624c1abb3664f4b35547e7c73864ad24")
            ]
        );
        assert_eq!(
            fooc.concluded_license.identifiers(),
            HashSet::from_iter(["LGPL-2.0-only".to_string(), "LicenseRef-2".to_string(),])
        );
        assert_eq!(
            fooc.license_information_in_file,
            vec![
                SimpleExpression::parse("GPL-2.0-only").unwrap(),
                SimpleExpression::parse("LicenseRef-2").unwrap()
            ]
        );
        assert_eq!(fooc.comments_on_license, Some("The concluded license was taken from the package level that the file was included in.".to_string()));
        assert_eq!(
            fooc.copyright_text,
            "Copyright 2008-2010 John Smith".to_string()
        );
        assert_eq!(
            fooc.file_notice,
            Some("Copyright (c) 2001 Aaron Lehmann aaroni@vitelus.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the �Software�), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: 
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.".to_string())
        );
        assert_eq!(
            fooc.file_contributor,
            vec![
                "The Regents of the University of California".to_string(),
                "Modified by Paul Mundt lethal@linux-sh.org".to_string(),
                "IBM Corporation".to_string(),
            ]
        );
        let doap = files
            .iter()
            .find(|p| p.file_name == "./src/org/spdx/parser/DOAPProject.java")
            .unwrap();

        assert_eq!(doap.file_spdx_identifier, "SPDXRef-DoapSource");
    }

    #[test]
    fn snippet_info_is_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let snippets = spdx.snippet_information;
        assert_eq!(snippets.len(), 1);

        let snippet = snippets[0].clone();

        assert_eq!(snippet.snippet_spdx_identifier, "SPDXRef-Snippet");
        assert_eq!(
            snippet.snippet_from_file_spdx_identifier,
            "SPDXRef-DoapSource"
        );
        assert_eq!(snippet.ranges.len(), 2);
        assert!(snippet
            .ranges
            .iter()
            .any(|snip| snip.start_pointer == Pointer::new_byte(None, 310)));
        assert!(snippet
            .ranges
            .iter()
            .any(|snip| snip.end_pointer == Pointer::new_byte(None, 420)));
        assert!(snippet
            .ranges
            .iter()
            .any(|snip| snip.start_pointer == Pointer::new_line(None, 5)));
        assert!(snippet
            .ranges
            .iter()
            .any(|snip| snip.end_pointer == Pointer::new_line(None, 23)));
        assert_eq!(
            snippet.snippet_concluded_license,
            SpdxExpression::parse("GPL-2.0-only").unwrap()
        );
        assert_eq!(snippet.license_information_in_snippet, vec!["GPL-2.0-only"]);
        assert_eq!(snippet.snippet_comments_on_license, Some("The concluded license was taken from package xyz, from which the snippet was copied into the current file. The concluded license information was found in the COPYING.txt file in package xyz.".to_string()));
        assert_eq!(
            snippet.snippet_copyright_text,
            "Copyright 2008-2010 John Smith"
        );
        assert_eq!(snippet.snippet_comment, Some("This snippet was identified as significant and highlighted in this Apache-2.0 file, when a commercial scanner identified it as being derived from file foo.c in package xyz which is licensed under GPL-2.0.".to_string()));
        assert_eq!(snippet.snippet_name, Some("from linux kernel".to_string()));
    }

    #[test]
    fn relationships_are_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let relationships = spdx.relationships;
        assert_eq!(relationships.len(), 11);

        assert!(relationships.contains(&Relationship::new(
            "SPDXRef-DOCUMENT",
            "SPDXRef-Package",
            crate::models::RelationshipType::Contains,
            None
        )));
        assert!(relationships.contains(&Relationship::new(
            "SPDXRef-CommonsLangSrc",
            "NOASSERTION",
            crate::models::RelationshipType::GeneratedFrom,
            None
        )));

        // Implied relationship by the file following the package in tag-value.
        assert!(relationships.contains(&Relationship::new(
            "SPDXRef-Package",
            "SPDXRef-DoapSource",
            crate::models::RelationshipType::Contains,
            None
        )));
    }

    #[test]
    fn annotations_are_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let annotations = spdx.annotations;
        assert_eq!(annotations.len(), 5);

        assert_eq!(
            annotations[2],
            Annotation::new(
                "Person: Suzanne Reviewer".to_string(),
                Utc.with_ymd_and_hms(2011, 3, 13, 0, 0, 0).unwrap(),
                AnnotationType::Review,
                Some("SPDXRef-DOCUMENT".to_string()),
                "Another example reviewer.".to_string()
            )
        );
    }

    #[test]
    fn license_info_is_retrieved() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();
        let license_info = spdx.other_licensing_information_detected;
        assert_eq!(license_info.len(), 5);
        assert_eq!(license_info[1].license_identifier, "LicenseRef-2");
        assert_eq!(
            license_info[3].license_name,
            "Beer-Ware License (Version 42)"
        );
        assert_eq!(
            license_info[3].license_cross_reference,
            vec!["http://people.freebsd.org/~phk/"]
        );
        assert_eq!(
            license_info[3].license_comment,
            Some("The beerware license has a couple of other standard variants.".to_string())
        );
        assert!(license_info[3]
            .extracted_text
            .starts_with(r#""THE BEER-WARE"#));
        assert!(license_info[3]
            .extracted_text
            .ends_with("Poul-Henning Kamp"));
    }

    #[test]
    fn tag_value_is_parsed() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let spdx = spdx_from_tag_value(&file).unwrap();

        assert_eq!(spdx.package_information.len(), 4);
        assert_eq!(spdx.file_information.len(), 4);
        assert_eq!(spdx.snippet_information.len(), 1);
        assert_eq!(spdx.relationships.len(), 11);
        assert_eq!(spdx.annotations.len(), 5);
        assert_eq!(spdx.other_licensing_information_detected.len(), 5);
    }
}

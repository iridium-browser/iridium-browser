// SPDX-FileCopyrightText: 2021 HH Partners
//
// SPDX-License-Identifier: MIT

use std::{num::ParseIntError, str::FromStr};

use nom::{
    branch::alt,
    bytes::complete::{tag, take_until, take_while},
    character::complete::{alphanumeric0, char, digit1, multispace0, not_line_ending},
    combinator::{map, map_res, opt},
    error::{ParseError, VerboseError},
    multi::many0,
    sequence::{delimited, preceded, separated_pair, tuple},
    AsChar, IResult,
};

use crate::models::{
    Algorithm, AnnotationType, Checksum, ExternalDocumentReference, ExternalPackageReference,
    ExternalPackageReferenceCategory, FileType, PackageVerificationCode, Relationship,
    RelationshipType,
};

#[derive(Debug, Clone, PartialEq)]
#[allow(clippy::upper_case_acronyms)]
pub(super) enum Atom {
    // Document Creation Information
    SpdxVersion(String),
    DataLicense(String),
    SPDXID(String),
    DocumentName(String),
    DocumentNamespace(String),
    ExternalDocumentRef(ExternalDocumentReference),
    LicenseListVersion(String),
    Creator(String),
    Created(String),
    CreatorComment(String),
    DocumentComment(String),

    // Package Information
    PackageName(String),
    PackageVersion(String),
    PackageFileName(String),
    PackageSupplier(String),
    PackageOriginator(String),
    PackageDownloadLocation(String),
    FilesAnalyzed(String),
    PackageVerificationCode(PackageVerificationCode),
    PackageChecksum(Checksum),
    PackageHomePage(String),
    PackageSourceInfo(String),
    PackageLicenseConcluded(String),
    PackageLicenseInfoFromFiles(String),
    PackageLicenseDeclared(String),
    PackageLicenseComments(String),
    PackageCopyrightText(String),
    PackageSummary(String),
    PackageDescription(String),
    PackageComment(String),
    ExternalRef(ExternalPackageReference),
    ExternalRefComment(String),
    PackageAttributionText(String),

    // File Information
    FileName(String),
    FileType(FileType),
    FileChecksum(Checksum),
    LicenseConcluded(String),
    LicenseInfoInFile(String),
    LicenseComments(String),
    FileCopyrightText(String),
    FileComment(String),
    FileNotice(String),
    FileContributor(String),
    FileAttributionText(String),

    // Snippet Information
    SnippetSPDXID(String),
    SnippetFromFileSPDXID(String),
    SnippetByteRange((i32, i32)),
    SnippetLineRange((i32, i32)),
    SnippetLicenseConcluded(String),
    LicenseInfoInSnippet(String),
    SnippetLicenseComments(String),
    SnippetCopyrightText(String),
    SnippetComment(String),
    SnippetName(String),
    SnippetAttributionText(String),

    // Other Licensing Information Detected
    LicenseID(String),
    ExtractedText(String),
    LicenseName(String),
    LicenseCrossReference(String),
    LicenseComment(String),

    // Relationship
    Relationship(Relationship),
    RelationshipComment(String),

    // Annotation
    Annotator(String),
    AnnotationDate(String),
    AnnotationType(AnnotationType),
    SPDXREF(String),
    AnnotationComment(String),

    /// Comment in the document. Not part of the final SPDX.
    TVComment(String),
}

pub(super) fn atoms(i: &str) -> IResult<&str, Vec<Atom>, VerboseError<&str>> {
    many0(alt((ws(tv_comment), ws(tag_value_to_atom))))(i)
}

fn tag_value_to_atom(i: &str) -> IResult<&str, Atom, VerboseError<&str>> {
    let (i, key_value) = tag_value(i)?;
    match key_value.0 {
        // Document Creation Information
        "SPDXVersion" => Ok((i, Atom::SpdxVersion(key_value.1.to_string()))),
        "DataLicense" => Ok((i, Atom::DataLicense(key_value.1.to_string()))),
        "SPDXID" => Ok((i, Atom::SPDXID(key_value.1.to_string()))),
        "DocumentName" => Ok((i, Atom::DocumentName(key_value.1.to_string()))),
        "DocumentNamespace" => Ok((i, Atom::DocumentNamespace(key_value.1.to_string()))),
        "ExternalDocumentRef" => {
            let (_, value) = external_document_reference(key_value.1)?;
            Ok((i, Atom::ExternalDocumentRef(value)))
        }
        "LicenseListVersion" => Ok((i, Atom::LicenseListVersion(key_value.1.to_string()))),
        "Creator" => Ok((i, Atom::Creator(key_value.1.to_string()))),
        "Created" => Ok((i, Atom::Created(key_value.1.to_string()))),
        "CreatorComment" => Ok((i, Atom::CreatorComment(key_value.1.to_string()))),
        "DocumentComment" => Ok((i, Atom::DocumentComment(key_value.1.to_string()))),

        // Package Information
        "PackageName" => Ok((i, Atom::PackageName(key_value.1.to_string()))),
        "PackageVersion" => Ok((i, Atom::PackageVersion(key_value.1.to_string()))),
        "PackageFileName" => Ok((i, Atom::PackageFileName(key_value.1.to_string()))),
        "PackageSupplier" => Ok((i, Atom::PackageSupplier(key_value.1.to_string()))),
        "PackageOriginator" => Ok((i, Atom::PackageOriginator(key_value.1.to_string()))),
        "PackageDownloadLocation" => {
            Ok((i, Atom::PackageDownloadLocation(key_value.1.to_string())))
        }
        "FilesAnalyzed" => Ok((i, Atom::FilesAnalyzed(key_value.1.to_string()))),
        "PackageVerificationCode" => {
            let (_, value) = package_verification_code(key_value.1)?;
            Ok((i, Atom::PackageVerificationCode(value)))
        }
        "PackageChecksum" => Ok((i, Atom::PackageChecksum(checksum(key_value.1)?.1))),
        "PackageHomePage" => Ok((i, Atom::PackageHomePage(key_value.1.to_string()))),
        "PackageSourceInfo" => Ok((i, Atom::PackageSourceInfo(key_value.1.to_string()))),
        "PackageLicenseConcluded" => {
            Ok((i, Atom::PackageLicenseConcluded(key_value.1.to_string())))
        }
        "PackageLicenseInfoFromFiles" => Ok((
            i,
            Atom::PackageLicenseInfoFromFiles(key_value.1.to_string()),
        )),
        "PackageLicenseDeclared" => Ok((i, Atom::PackageLicenseDeclared(key_value.1.to_string()))),
        "PackageLicenseComments" => Ok((i, Atom::PackageLicenseComments(key_value.1.to_string()))),
        "PackageCopyrightText" => Ok((i, Atom::PackageCopyrightText(key_value.1.to_string()))),
        "PackageSummary" => Ok((i, Atom::PackageSummary(key_value.1.to_string()))),
        "PackageDescription" => Ok((i, Atom::PackageDescription(key_value.1.to_string()))),
        "PackageComment" => Ok((i, Atom::PackageComment(key_value.1.to_string()))),
        "ExternalRef" => Ok((
            i,
            Atom::ExternalRef(external_package_reference(key_value.1)?.1),
        )),
        "ExternalRefComment" => Ok((i, Atom::ExternalRefComment(key_value.1.to_string()))),
        "PackageAttributionText" => Ok((i, Atom::PackageAttributionText(key_value.1.to_string()))),

        // File Information
        "FileName" => Ok((i, Atom::FileName(key_value.1.to_string()))),
        "FileType" => Ok((i, Atom::FileType(file_type(key_value.1)?.1))),
        "FileChecksum" => Ok((i, Atom::FileChecksum(checksum(key_value.1)?.1))),
        "LicenseConcluded" => Ok((i, Atom::LicenseConcluded(key_value.1.to_string()))),
        "LicenseInfoInFile" => Ok((i, Atom::LicenseInfoInFile(key_value.1.to_string()))),
        "LicenseComments" => Ok((i, Atom::LicenseComments(key_value.1.to_string()))),
        "FileCopyrightText" => Ok((i, Atom::FileCopyrightText(key_value.1.to_string()))),
        "FileComment" => Ok((i, Atom::FileComment(key_value.1.to_string()))),
        "FileNotice" => Ok((i, Atom::FileNotice(key_value.1.to_string()))),
        "FileContributor" => Ok((i, Atom::FileContributor(key_value.1.to_string()))),
        "FileAttributionText" => Ok((i, Atom::FileAttributionText(key_value.1.to_string()))),

        // Snippet Information
        "SnippetSPDXID" => Ok((i, Atom::SnippetSPDXID(key_value.1.to_string()))),
        "SnippetFromFileSPDXID" => Ok((i, Atom::SnippetFromFileSPDXID(key_value.1.to_string()))),
        "SnippetByteRange" => Ok((i, Atom::SnippetByteRange(range(key_value.1)?.1))),
        "SnippetLineRange" => Ok((i, Atom::SnippetLineRange(range(key_value.1)?.1))),
        "SnippetLicenseConcluded" => {
            Ok((i, Atom::SnippetLicenseConcluded(key_value.1.to_string())))
        }
        "LicenseInfoInSnippet" => Ok((i, Atom::LicenseInfoInSnippet(key_value.1.to_string()))),
        "SnippetLicenseComments" => Ok((i, Atom::SnippetLicenseComments(key_value.1.to_string()))),
        "SnippetCopyrightText" => Ok((i, Atom::SnippetCopyrightText(key_value.1.to_string()))),
        "SnippetComment" => Ok((i, Atom::SnippetComment(key_value.1.to_string()))),
        "SnippetName" => Ok((i, Atom::SnippetName(key_value.1.to_string()))),
        "SnippetAttributionText" => Ok((i, Atom::SnippetAttributionText(key_value.1.to_string()))),

        // Other Licensing Information Detected
        "LicenseID" => Ok((i, Atom::LicenseID(key_value.1.to_string()))),
        "ExtractedText" => Ok((i, Atom::ExtractedText(key_value.1.to_string()))),
        "LicenseName" => Ok((i, Atom::LicenseName(key_value.1.to_string()))),
        "LicenseCrossReference" => Ok((i, Atom::LicenseCrossReference(key_value.1.to_string()))),
        "LicenseComment" => Ok((i, Atom::LicenseComment(key_value.1.to_string()))),

        // Relationship
        "Relationship" => Ok((i, Atom::Relationship(relationship(key_value.1)?.1))),
        "RelationshipComment" => Ok((i, Atom::RelationshipComment(key_value.1.to_string()))),

        // Annotation
        "Annotator" => Ok((i, Atom::Annotator(key_value.1.to_string()))),
        "AnnotationDate" => Ok((i, Atom::AnnotationDate(key_value.1.to_string()))),
        "AnnotationType" => Ok((i, Atom::AnnotationType(annotation_type(key_value.1)?.1))),
        "SPDXREF" => Ok((i, Atom::SPDXREF(key_value.1.to_string()))),
        "AnnotationComment" => Ok((i, Atom::AnnotationComment(key_value.1.to_string()))),
        v => {
            dbg!(v);
            unimplemented!()
        }
    }
}

fn external_document_reference(
    i: &str,
) -> IResult<&str, ExternalDocumentReference, VerboseError<&str>> {
    map(
        tuple((
            document_ref,
            ws(take_while(|c: char| !c.is_whitespace())),
            ws(checksum),
        )),
        |(id_string, spdx_document_uri, checksum)| {
            ExternalDocumentReference::new(
                id_string.to_string(),
                spdx_document_uri.to_string(),
                checksum,
            )
        },
    )(i)
}

fn annotation_type(i: &str) -> IResult<&str, AnnotationType, VerboseError<&str>> {
    match ws(not_line_ending)(i) {
        Ok((i, value)) => match value {
            "REVIEW" => Ok((i, AnnotationType::Review)),
            "OTHER" => Ok((i, AnnotationType::Other)),
            // Proper error
            _ => todo!(),
        },
        Err(err) => Err(err),
    }
}

fn file_type(i: &str) -> IResult<&str, FileType, VerboseError<&str>> {
    match ws(not_line_ending)(i) {
        Ok((i, value)) => match value {
            "SOURCE" => Ok((i, FileType::Source)),
            "BINARY" => Ok((i, FileType::Binary)),
            "ARCHIVE" => Ok((i, FileType::Archive)),
            "APPLICATION" => Ok((i, FileType::Application)),
            "AUDIO" => Ok((i, FileType::Audio)),
            "IMAGE" => Ok((i, FileType::Image)),
            "TEXT" => Ok((i, FileType::Text)),
            "VIDEO" => Ok((i, FileType::Video)),
            "DOCUMENTATION" => Ok((i, FileType::Documentation)),
            "SPDX" => Ok((i, FileType::SPDX)),
            "OTHER" => Ok((i, FileType::Other)),
            // Proper error
            _ => todo!(),
        },
        Err(err) => Err(err),
    }
}

fn document_ref(i: &str) -> IResult<&str, &str, VerboseError<&str>> {
    preceded(tag("DocumentRef-"), ws(idstring))(i)
}

fn relationship(i: &str) -> IResult<&str, Relationship, VerboseError<&str>> {
    map(
        tuple((
            ws(take_while(|c: char| !c.is_whitespace())),
            ws(take_while(|c: char| !c.is_whitespace())),
            ws(not_line_ending),
        )),
        |(item1, relationship_type, item2)| {
            let relationship_type = relationship_type.to_uppercase();
            let relationship_type = match relationship_type.as_str() {
                "DESCRIBES" => RelationshipType::Describes,
                "DESCRIBED_BY" => RelationshipType::DescribedBy,
                "CONTAINS" => RelationshipType::Contains,
                "CONTAINED_BY" => RelationshipType::ContainedBy,
                "DEPENDS_ON" => RelationshipType::DependsOn,
                "DEPENDENCY_OF" => RelationshipType::DependencyOf,
                "DEPENDENCY_MANIFEST_OF" => RelationshipType::DependencyManifestOf,
                "BUILD_DEPENDENCY_OF" => RelationshipType::BuildDependencyOf,
                "DEV_DEPENDENCY_OF" => RelationshipType::DevDependencyOf,
                "OPTIONAL_DEPENDENCY_OF" => RelationshipType::OptionalDependencyOf,
                "PROVIDED_DEPENDENCY_OF" => RelationshipType::ProvidedDependencyOf,
                "TEST_DEPENDENCY_OF" => RelationshipType::TestDependencyOf,
                "RUNTIME_DEPENDENCY_OF" => RelationshipType::RuntimeDependencyOf,
                "EXAMPLE_OF" => RelationshipType::ExampleOf,
                "GENERATES" => RelationshipType::Generates,
                "GENERATED_FROM" => RelationshipType::GeneratedFrom,
                "ANCESTOR_OF" => RelationshipType::AncestorOf,
                "DESCENDANT_OF" => RelationshipType::DescendantOf,
                "VARIANT_OF" => RelationshipType::VariantOf,
                "DISTRIBUTION_ARTIFACT" => RelationshipType::DistributionArtifact,
                "PATCH_FOR" => RelationshipType::PatchFor,
                "PATCH_APPLIED" => RelationshipType::PatchApplied,
                "COPY_OF" => RelationshipType::CopyOf,
                "FILE_ADDED" => RelationshipType::FileAdded,
                "FILE_DELETED" => RelationshipType::FileDeleted,
                "FILE_MODIFIED" => RelationshipType::FileModified,
                "EXPANDED_FROM_ARCHIVE" => RelationshipType::ExpandedFromArchive,
                "DYNAMIC_LINK" => RelationshipType::DynamicLink,
                "STATIC_LINK" => RelationshipType::StaticLink,
                "DATA_FILE_OF" => RelationshipType::DataFileOf,
                "TEST_CASE_OF" => RelationshipType::TestCaseOf,
                "BUILD_TOOL_OF" => RelationshipType::BuildToolOf,
                "DEV_TOOL_OF" => RelationshipType::DevToolOf,
                "TEST_OF" => RelationshipType::TestOf,
                "TEST_TOOL_OF" => RelationshipType::TestToolOf,
                "DOCUMENTATION_OF" => RelationshipType::DocumentationOf,
                "OPTIONAL_COMPONENT_OF" => RelationshipType::OptionalComponentOf,
                "METAFILE_OF" => RelationshipType::MetafileOf,
                "PACKAGE_OF" => RelationshipType::PackageOf,
                "AMENDS" => RelationshipType::Amends,
                "PREREQUISITE_FOR" => RelationshipType::PrerequisiteFor,
                "HAS_PREREQUISITE" => RelationshipType::HasPrerequisite,
                "OTHER" => RelationshipType::Other,
                // TODO: Proper error.
                _ => {
                    dbg!(relationship_type);
                    todo!()
                }
            };
            Relationship::new(item1, item2, relationship_type, None)
        },
    )(i)
}

fn external_package_reference(
    i: &str,
) -> IResult<&str, ExternalPackageReference, VerboseError<&str>> {
    map(
        tuple((
            ws(take_while(|c: char| !c.is_whitespace())),
            ws(take_while(|c: char| !c.is_whitespace())),
            ws(not_line_ending),
        )),
        |(category, ref_type, locator)| {
            let category = match category {
                "SECURITY" => ExternalPackageReferenceCategory::Security,
                "PACKAGE-MANAGER" => ExternalPackageReferenceCategory::PackageManager,
                "PERSISTENT-ID" => ExternalPackageReferenceCategory::PersistentID,
                "OTHER" => ExternalPackageReferenceCategory::Other,
                // TODO: Proper error handling
                _ => todo!(),
            };
            ExternalPackageReference::new(category, ref_type.to_string(), locator.to_string(), None)
        },
    )(i)
}

fn package_verification_code(
    i: &str,
) -> IResult<&str, PackageVerificationCode, VerboseError<&str>> {
    map(
        alt((
            separated_pair(
                ws(take_until("(excludes:")),
                ws(tag("(excludes:")),
                opt(take_until(")")),
            ),
            map(ws(not_line_ending), |v| (v, None)),
        )),
        |(value, exclude)| {
            #[allow(clippy::option_if_let_else)]
            let excludes = if let Some(exclude) = exclude {
                vec![exclude.to_string()]
            } else {
                Vec::new()
            };
            PackageVerificationCode::new(value.to_string(), excludes)
        },
    )(i)
}

fn range(i: &str) -> IResult<&str, (i32, i32), VerboseError<&str>> {
    map_res::<_, _, _, _, ParseIntError, _, _>(
        separated_pair(digit1, char(':'), digit1),
        |(left, right)| Ok((i32::from_str(left)?, i32::from_str(right)?)),
    )(i)
}

fn idstring(i: &str) -> IResult<&str, &str, VerboseError<&str>> {
    take_while(|c: char| c.is_alphanum() || c == '.' || c == '-' || c == '+')(i)
}

fn checksum(i: &str) -> IResult<&str, Checksum, VerboseError<&str>> {
    map(
        separated_pair(ws(take_until(":")), char(':'), ws(not_line_ending)),
        |(algorithm, value)| {
            let checksum_algorithm = match algorithm {
                "SHA1" => Algorithm::SHA1,
                "SHA224" => Algorithm::SHA224,
                "SHA256" => Algorithm::SHA256,
                "SHA384" => Algorithm::SHA384,
                "SHA512" => Algorithm::SHA512,
                "MD2" => Algorithm::MD2,
                "MD4" => Algorithm::MD4,
                "MD5" => Algorithm::MD5,
                "MD6" => Algorithm::MD6,
                // TODO: Use proper error.
                _ => todo!(),
            };
            Checksum::new(checksum_algorithm, value)
        },
    )(i)
}

fn tv_comment(i: &str) -> IResult<&str, Atom, VerboseError<&str>> {
    map(preceded(ws(tag("#")), ws(not_line_ending)), |v| {
        Atom::TVComment(v.to_string())
    })(i)
}

fn tag_value(i: &str) -> IResult<&str, (&str, &str), VerboseError<&str>> {
    separated_pair(
        ws(alphanumeric0),
        tag(":"),
        alt((ws(multiline_text), ws(not_line_ending))),
    )(i)
}

fn multiline_text(i: &str) -> IResult<&str, &str, VerboseError<&str>> {
    delimited(tag("<text>"), take_until("</text>"), tag("</text>"))(i)
}

/// A combinator that takes a parser `inner` and produces a parser that also consumes both leading and
/// trailing whitespace, returning the output of `inner`.
fn ws<'a, F, O, E: ParseError<&'a str>>(inner: F) -> impl FnMut(&'a str) -> IResult<&'a str, O, E>
where
    F: Fn(&'a str) -> IResult<&'a str, O, E> + 'a,
{
    delimited(multispace0, inner, multispace0)
}

#[cfg(test)]
mod tests {
    use std::fs::read_to_string;

    use crate::{
        models::{Algorithm, AnnotationType, ExternalPackageReferenceCategory, Relationship},
        parsers::tag_value::{
            annotation_type, checksum, document_ref, external_document_reference,
            external_package_reference, package_verification_code, range, relationship,
        },
    };

    use super::{atoms, tag_value, tag_value_to_atom, Atom};

    #[test]
    fn version_can_be_parsed() {
        let (_, value) = tag_value_to_atom("SPDXVersion: SPDX-1.2").unwrap();
        assert_eq!(value, Atom::SpdxVersion("SPDX-1.2".to_string()));
    }

    #[test]
    fn range_can_be_parsed() {
        let (_, value) = range("310:420").unwrap();
        assert_eq!(value, (310, 420));
    }

    #[test]
    fn annotation_type_can_be_parsed() {
        let (_, value) = annotation_type("REVIEW").unwrap();
        assert_eq!(value, AnnotationType::Review);
        let (_, value) = annotation_type("OTHER").unwrap();
        assert_eq!(value, AnnotationType::Other);
    }

    #[test]
    fn relationship_can_be_parsed() {
        let (_, value) = relationship("SPDXRef-JenaLib CONTAINS SPDXRef-Package").unwrap();
        let expected = Relationship::new(
            "SPDXRef-JenaLib",
            "SPDXRef-Package",
            crate::models::RelationshipType::Contains,
            None,
        );
        assert_eq!(value, expected);
    }

    #[test]
    fn data_license_can_be_parsed() {
        let (_, value) = tag_value_to_atom("DataLicense: CC0-1.0").unwrap();
        assert_eq!(value, Atom::DataLicense("CC0-1.0".to_string()));
    }

    #[test]
    fn package_verification_code_can_be_parsed() {
        let (_, value) = package_verification_code(
            "d6a770ba38583ed4bb4525bd96e50461655d2758(excludes: ./package.spdx)",
        )
        .unwrap();
        assert_eq!(value.value, "d6a770ba38583ed4bb4525bd96e50461655d2758");
        assert_eq!(value.excludes, vec!["./package.spdx"]);
    }

    #[test]
    fn package_verification_code_without_excludes_can_be_parsed() {
        let (_, value) =
            package_verification_code("d6a770ba38583ed4bb4525bd96e50461655d2758").unwrap();
        assert_eq!(value.value, "d6a770ba38583ed4bb4525bd96e50461655d2758");
        let expected: Vec<String> = Vec::new();
        assert_eq!(value.excludes, expected);
    }

    #[test]
    fn external_package_ref_can_be_parsed() {
        let (_, value) = external_package_reference(
            "SECURITY cpe23Type cpe:2.3:a:pivotal_software:spring_framework:4.1.0:*:*:*:*:*:*:*",
        )
        .unwrap();
        assert_eq!(
            value.reference_category,
            ExternalPackageReferenceCategory::Security
        );
        assert_eq!(value.reference_type, "cpe23Type");
        assert_eq!(
            value.reference_locator,
            "cpe:2.3:a:pivotal_software:spring_framework:4.1.0:*:*:*:*:*:*:*"
        );
    }

    #[test]
    fn external_document_reference_can_be_parsed() {
        let (_, value) = external_document_reference("DocumentRef-spdx-tool-1.2 http://spdx.org/spdxdocs/spdx-tools-v1.2-3F2504E0-4F89-41D3-9A0C-0305E82C3301 SHA1: d6a770ba38583ed4bb4525bd96e50461655d2759").unwrap();
        assert_eq!(value.id_string, "spdx-tool-1.2");
        assert_eq!(
            value.spdx_document_uri,
            "http://spdx.org/spdxdocs/spdx-tools-v1.2-3F2504E0-4F89-41D3-9A0C-0305E82C3301"
        );
        assert_eq!(value.checksum.algorithm, Algorithm::SHA1);
        assert_eq!(
            value.checksum.value,
            "d6a770ba38583ed4bb4525bd96e50461655d2759"
        );
    }

    #[test]
    fn document_ref_can_be_parsed() {
        let (_, value) = document_ref("DocumentRef-spdx-tool-1.2").unwrap();
        assert_eq!(value, "spdx-tool-1.2");
    }

    #[test]
    fn checksum_can_be_parsed() {
        let (_, value) = checksum("SHA1: d6a770ba38583ed4bb4525bd96e50461655d2759").unwrap();
        assert_eq!(value.algorithm, Algorithm::SHA1);
        assert_eq!(value.value, "d6a770ba38583ed4bb4525bd96e50461655d2759");
    }

    #[test]
    fn document_comment_can_be_parsed() {
        let (_, value) = tag_value_to_atom("DocumentComment: <text>Sample Comment</text>").unwrap();
        assert_eq!(value, Atom::DocumentComment("Sample Comment".to_string()));
    }

    #[test]
    fn multiline_document_comment_can_be_parsed() {
        let (_, value) = tag_value_to_atom(
            "DocumentComment: <text>Sample
Comment</text>",
        )
        .unwrap();
        assert_eq!(value, Atom::DocumentComment("Sample\nComment".to_string()));
    }

    #[test]
    fn multiple_key_values_can_be_parsed() {
        let input = "SPDXVersion: SPDX-1.2
                    DataLicense: CC0-1.0
                    DocumentComment: <text>Sample Comment</text>";

        let (_, value) = atoms(input).unwrap();
        assert_eq!(
            value,
            vec![
                Atom::SpdxVersion("SPDX-1.2".to_string()),
                Atom::DataLicense("CC0-1.0".to_string()),
                Atom::DocumentComment("Sample Comment".to_string())
            ]
        );
    }

    #[test]
    fn multiple_key_values_with_comment_can_be_parsed() {
        let input = "SPDXVersion: SPDX-1.2
                    # A comment
                    DataLicense: CC0-1.0
                    DocumentComment: <text>Sample Comment</text>";

        let (_, value) = atoms(input).unwrap();
        assert_eq!(
            value,
            vec![
                Atom::SpdxVersion("SPDX-1.2".to_string()),
                Atom::TVComment("A comment".to_string()),
                Atom::DataLicense("CC0-1.0".to_string()),
                Atom::DocumentComment("Sample Comment".to_string())
            ]
        );
    }

    #[test]
    fn multiple_key_values_with_space_can_be_parsed() {
        let input = "SPDXVersion: SPDX-1.2

                    DataLicense: CC0-1.0
                    DocumentComment: <text>Sample Comment</text>";

        let (_, value) = atoms(input).unwrap();
        assert_eq!(
            value,
            vec![
                Atom::SpdxVersion("SPDX-1.2".to_string()),
                Atom::DataLicense("CC0-1.0".to_string()),
                Atom::DocumentComment("Sample Comment".to_string())
            ]
        );
    }

    #[test]
    fn key_value_pair_is_detected() {
        let (_, value) = tag_value("SPDXVersion: SPDX-1.2").unwrap();
        assert_eq!(value, ("SPDXVersion", "SPDX-1.2"));
    }

    #[test]
    fn get_tag_values_from_simple_example_file() {
        let file = read_to_string("tests/data/SPDXSimpleTag.tag").unwrap();
        let (remains, result) = atoms(&file).unwrap();
        assert_eq!(remains.len(), 0);
        assert!(result.contains(&Atom::SpdxVersion("SPDX-1.2".to_string())));
        assert!(result.contains(&Atom::PackageName("Test".to_string())));
        assert!(result.contains(&Atom::PackageDescription("A package.".to_string())));
    }

    #[test]
    fn get_tag_values_from_example_file() {
        let file = read_to_string("tests/data/SPDXTagExample-v2.2.spdx").unwrap();
        let (remains, result) = atoms(&file).unwrap();
        assert_eq!(remains.len(), 0);
        assert!(result.contains(&Atom::SpdxVersion("SPDX-2.2".to_string())));
        assert!(result.contains(&Atom::LicenseListVersion("3.9".to_string())));
        assert!(result.contains(&Atom::PackageLicenseDeclared("MPL-1.0".to_string())));
    }

    #[test]
    fn relationship_case() {
        relationship("SPDXRef-DOCUMENT DESCRIBES SPDXRef-File").expect("Caps is expected");
        relationship("SPDXRef-DOCUMENT describes SPDXRef-File")
            .expect("At least reuse-tool emits lowercase");
    }
}

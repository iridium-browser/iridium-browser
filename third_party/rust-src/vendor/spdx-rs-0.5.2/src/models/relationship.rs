// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use serde::{Deserialize, Serialize};
use strum_macros::AsRefStr;

/// <https://spdx.github.io/spdx-spec/7-relationships-between-SPDX-elements/#71-relationship>
#[derive(Debug, Serialize, Deserialize, PartialEq, Clone, Eq, Hash)]
#[serde(rename_all = "camelCase")]
pub struct Relationship {
    /// SPDX ID of the element.
    pub spdx_element_id: String,

    /// SPDX ID of the related element.
    pub related_spdx_element: String,

    /// Type of the relationship.
    pub relationship_type: RelationshipType,

    /// <https://spdx.github.io/spdx-spec/7-relationships-between-SPDX-elements/#72-relationship-comment>
    #[serde(skip_serializing_if = "Option::is_none")]
    #[serde(default)]
    pub comment: Option<String>,
}

impl Relationship {
    /// Create a new relationship.
    pub fn new(
        spdx_element_id: &str,
        related_spdx_element: &str,
        relationship_type: RelationshipType,
        comment: Option<String>,
    ) -> Self {
        Self {
            spdx_element_id: spdx_element_id.to_string(),
            related_spdx_element: related_spdx_element.to_string(),
            relationship_type,
            comment,
        }
    }
}

/// <https://spdx.github.io/spdx-spec/7-relationships-between-SPDX-elements/#71-relationship>
#[derive(Serialize, Deserialize, Debug, PartialEq, Clone, AsRefStr, Eq, Hash)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum RelationshipType {
    Describes,
    DescribedBy,
    Contains,
    ContainedBy,
    DependsOn,
    DependencyOf,
    DependencyManifestOf,
    BuildDependencyOf,
    DevDependencyOf,
    OptionalDependencyOf,
    ProvidedDependencyOf,
    TestDependencyOf,
    RuntimeDependencyOf,
    ExampleOf,
    Generates,
    GeneratedFrom,
    AncestorOf,
    DescendantOf,
    VariantOf,
    DistributionArtifact,
    PatchFor,
    PatchApplied,
    CopyOf,
    FileAdded,
    FileDeleted,
    FileModified,
    ExpandedFromArchive,
    DynamicLink,
    StaticLink,
    DataFileOf,
    TestCaseOf,
    BuildToolOf,
    DevToolOf,
    TestOf,
    TestToolOf,
    DocumentationOf,
    OptionalComponentOf,
    MetafileOf,
    PackageOf,
    Amends,
    PrerequisiteFor,
    HasPrerequisite,
    Other,
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use crate::models::SPDX;

    use super::*;

    #[test]
    fn spdx_element_id() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.relationships[0].spdx_element_id,
            "SPDXRef-DOCUMENT".to_string()
        );
    }
    #[test]
    fn related_spdx_element() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.relationships[0].related_spdx_element,
            "SPDXRef-Package".to_string()
        );
    }
    #[test]
    fn relationship_type() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.relationships[0].relationship_type,
            RelationshipType::Contains
        );
        assert_eq!(
            spdx.relationships[2].relationship_type,
            RelationshipType::CopyOf
        );
    }
}

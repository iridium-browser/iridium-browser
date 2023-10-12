// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

/// <https://spdx.github.io/spdx-spec/8-annotations/>
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Annotation {
    /// <https://spdx.github.io/spdx-spec/8-annotations/#81-annotator>
    pub annotator: String,

    /// <https://spdx.github.io/spdx-spec/8-annotations/#82-annotation-date>
    pub annotation_date: DateTime<Utc>,

    /// <https://spdx.github.io/spdx-spec/8-annotations/#83-annotation-type>
    pub annotation_type: AnnotationType,

    /// <https://spdx.github.io/spdx-spec/8-annotations/#84-spdx-identifier-reference>
    // TODO: According to the spec this is mandatory, but the example file doesn't
    // have it.
    pub spdx_identifier_reference: Option<String>,

    /// <https://spdx.github.io/spdx-spec/8-annotations/#85-annotation-comment>
    #[serde(rename = "comment")]
    pub annotation_comment: String,
}

impl Annotation {
    pub fn new(
        annotator: String,
        annotation_date: DateTime<Utc>,
        annotation_type: AnnotationType,
        spdx_identifier_reference: Option<String>,
        annotation_comment: String,
    ) -> Self {
        Self {
            annotator,
            annotation_date,
            annotation_type,
            spdx_identifier_reference,
            annotation_comment,
        }
    }
}

/// <https://spdx.github.io/spdx-spec/8-annotations/#83-annotation-type>
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq, Clone, Copy)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum AnnotationType {
    Review,
    Other,
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use chrono::TimeZone;

    use crate::models::SPDX;

    use super::*;

    #[test]
    fn annotator() {
        let spdx_file: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx_file.annotations[0].annotator,
            "Person: Jane Doe ()".to_string()
        );
    }

    #[test]
    fn annotation_date() {
        let spdx_file: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx_file.annotations[0].annotation_date,
            Utc.with_ymd_and_hms(2010, 1, 29, 18, 30, 22).unwrap()
        );
    }

    #[test]
    fn annotation_type() {
        let spdx_file: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx_file.annotations[0].annotation_type,
            AnnotationType::Other
        );
    }

    #[test]
    fn annotation_comment() {
        let spdx_file: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx_file.annotations[0].annotation_comment,
            "Document level annotation"
        );
    }
}

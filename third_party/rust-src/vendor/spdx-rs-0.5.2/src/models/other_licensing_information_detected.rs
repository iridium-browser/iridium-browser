// SPDX-FileCopyrightText: 2020-2021 HH Partners
//
// SPDX-License-Identifier: MIT

use serde::{Deserialize, Serialize};

/// <https://spdx.github.io/spdx-spec/6-other-licensing-information-detected/>
#[derive(Debug, Serialize, Deserialize, Clone, Default, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct OtherLicensingInformationDetected {
    /// <https://spdx.github.io/spdx-spec/6-other-licensing-information-detected/#61-license-identifier>
    #[serde(rename = "licenseId")]
    pub license_identifier: String,

    /// <https://spdx.github.io/spdx-spec/6-other-licensing-information-detected/#62-extracted-text>
    pub extracted_text: String,

    /// <https://spdx.github.io/spdx-spec/6-other-licensing-information-detected/#63-license-name>
    #[serde(rename = "name")]
    #[serde(default = "default_noassertion")]
    pub license_name: String,

    /// <https://spdx.github.io/spdx-spec/6-other-licensing-information-detected/#64-license-cross-reference>
    #[serde(rename = "seeAlsos", skip_serializing_if = "Vec::is_empty", default)]
    pub license_cross_reference: Vec<String>,

    /// <https://spdx.github.io/spdx-spec/6-other-licensing-information-detected/#65-license-comment>
    #[serde(rename = "comment", skip_serializing_if = "Option::is_none", default)]
    pub license_comment: Option<String>,
}

fn default_noassertion() -> String {
    "NOASSERTION".into()
}

#[cfg(test)]
mod test {
    use std::fs::read_to_string;

    use crate::models::SPDX;

    #[test]
    fn license_identifier() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.other_licensing_information_detected[0].license_identifier,
            "LicenseRef-Beerware-4.2".to_string()
        );
    }
    #[test]
    fn extracted_text() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(spdx.other_licensing_information_detected[0].extracted_text, "\"THE BEER-WARE LICENSE\" (Revision 42):\nphk@FreeBSD.ORG wrote this file. As long as you retain this notice you\ncan do whatever you want with this stuff. If we meet some day, and you think this stuff is worth it, you can buy me a beer in return Poul-Henning Kamp  </\nLicenseName: Beer-Ware License (Version 42)\nLicenseCrossReference:  http://people.freebsd.org/~phk/\nLicenseComment: \nThe beerware license has a couple of other standard variants.");
    }
    #[test]
    fn license_name() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.other_licensing_information_detected[2].license_name,
            "CyberNeko License".to_string()
        );
    }
    #[test]
    fn license_cross_reference() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.other_licensing_information_detected[2].license_cross_reference,
            vec![
                "http://people.apache.org/~andyc/neko/LICENSE".to_string(),
                "http://justasample.url.com".to_string()
            ]
        );
    }
    #[test]
    fn license_comment() {
        let spdx: SPDX = serde_json::from_str(
            &read_to_string("tests/data/SPDXJSONExample-v2.2.spdx.json").unwrap(),
        )
        .unwrap();
        assert_eq!(
            spdx.other_licensing_information_detected[2].license_comment,
            Some("This is tye CyperNeko License".to_string())
        );
    }
}

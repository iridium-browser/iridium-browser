// SPDX-FileCopyrightText: 2022 HH Partners
//
// SPDX-License-Identifier: MIT

//! Errors of the library.

/// Custom error struct.
#[derive(thiserror::Error, Debug)]
pub enum SpdxExpressionError {
    #[error("Parsing for expression `{0}` failed.")]
    Parse(String),

    #[error("Error parsing the SPDX Expression {0}.")]
    Nom(String),
}

impl From<nom::Err<nom::error::Error<&str>>> for SpdxExpressionError {
    fn from(err: nom::Err<nom::error::Error<&str>>) -> Self {
        Self::Nom(err.to_string())
    }
}

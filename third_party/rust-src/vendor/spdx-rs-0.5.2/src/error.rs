// SPDX-FileCopyrightText: 2021 HH Partners
//
// SPDX-License-Identifier: MIT

use std::io;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum SpdxError {
    #[error("Error parsing the SPDX Expression.")]
    Parse {
        #[from]
        source: spdx_expression::SpdxExpressionError,
    },

    #[error("Path {0} doesn't have an extension.")]
    PathExtension(String),

    #[error("Error with file I/O.")]
    Io {
        #[from]
        source: io::Error,
    },

    #[error("Error while parsing date.")]
    DateTimeParse {
        #[from]
        source: chrono::ParseError,
    },

    #[error("Error parsing tag-value: {0}")]
    TagValueParse(String),
}

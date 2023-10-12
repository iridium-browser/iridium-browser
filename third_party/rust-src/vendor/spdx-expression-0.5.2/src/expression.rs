// SPDX-FileCopyrightText: 2022 HH Partners
//
// SPDX-License-Identifier: MIT

//! The main struct of the library.

use std::{collections::HashSet, fmt::Display, string::ToString};

use serde::{de::Visitor, Deserialize, Serialize};

use crate::{
    error::SpdxExpressionError,
    expression_variant::{ExpressionVariant, SimpleExpression},
};

/// Main struct for SPDX License Expressions.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SpdxExpression {
    /// The parsed expression.
    inner: ExpressionVariant,
}

impl SpdxExpression {
    /// Parse `Self` from a string. The input expression needs to be a syntactically valid SPDX
    /// expression, `NONE` or `NOASSERTION`. The parser accepts license identifiers that are not
    /// valid SPDX.
    ///
    /// # Examples
    ///
    /// ```
    /// # use spdx_expression::SpdxExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SpdxExpression::parse("MIT")?;
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    ///
    /// License expressions need to be syntactically valid, but they can include license
    /// identifiers not on the SPDX license list or not specified with `LicenseRef`.
    ///
    /// ```
    /// # use spdx_expression::SpdxExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SpdxExpression::parse("MIT OR InvalidLicenseId")?;
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    ///
    /// # Errors
    ///
    /// Returns `SpdxExpressionError` if the license expression is not syntactically valid.
    pub fn parse(expression: &str) -> Result<Self, SpdxExpressionError> {
        Ok(Self {
            inner: ExpressionVariant::parse(expression)
                .map_err(|err| SpdxExpressionError::Parse(err.to_string()))?,
        })
    }

    /// Get all license and exception identifiers from the `SpdxExpression`.
    ///
    /// # Examples
    ///
    /// ```
    /// # use std::collections::HashSet;
    /// # use std::iter::FromIterator;
    /// # use spdx_expression::SpdxExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SpdxExpression::parse("MIT OR Apache-2.0")?;
    /// let licenses = expression.identifiers();
    /// assert_eq!(licenses, HashSet::from_iter(["Apache-2.0".to_string(), "MIT".to_string()]));
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    ///
    /// ```
    /// # use std::collections::HashSet;
    /// # use std::iter::FromIterator;
    /// # use spdx_expression::SpdxExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SpdxExpression::parse("MIT OR GPL-2.0-only WITH Classpath-exception-2.0")?;
    /// let licenses = expression.identifiers();
    /// assert_eq!(
    ///     licenses,
    ///     HashSet::from_iter([
    ///         "MIT".to_string(),
    ///         "GPL-2.0-only".to_string(),
    ///         "Classpath-exception-2.0".to_string()
    ///     ])
    /// );
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    pub fn identifiers(&self) -> HashSet<String> {
        let mut identifiers = self
            .licenses()
            .iter()
            .map(ToString::to_string)
            .collect::<HashSet<_>>();

        identifiers.extend(self.exceptions().iter().map(ToString::to_string));

        identifiers
    }

    /// Get all simple license expressions in `Self`. For licenses with exceptions, returns the
    /// license without the exception
    ///
    /// # Examples
    ///
    /// ```
    /// # use std::collections::HashSet;
    /// # use std::iter::FromIterator;
    /// # use spdx_expression::SpdxExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SpdxExpression::parse(
    ///     "(MIT OR Apache-2.0 AND (GPL-2.0-only WITH Classpath-exception-2.0 OR ISC))",
    /// )
    /// .unwrap();
    ///
    /// let licenses = expression.licenses();
    ///
    /// assert_eq!(
    ///     licenses
    ///         .iter()
    ///         .map(|&license| license.identifier.as_str())
    ///         .collect::<HashSet<_>>(),
    ///     HashSet::from_iter(["Apache-2.0", "GPL-2.0-only", "ISC", "MIT"])
    /// );
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    pub fn licenses(&self) -> HashSet<&SimpleExpression> {
        self.inner.licenses()
    }

    /// Get all exception identifiers for `Self`.
    ///
    /// # Examples
    ///
    /// ```
    /// # use std::collections::HashSet;
    /// # use std::iter::FromIterator;
    /// # use spdx_expression::SpdxExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SpdxExpression::parse(
    ///     "(MIT OR Apache-2.0 AND (GPL-2.0-only WITH Classpath-exception-2.0 OR ISC))",
    /// )
    /// .unwrap();
    ///
    /// let exceptions = expression.exceptions();
    ///
    /// assert_eq!(exceptions, HashSet::from_iter(["Classpath-exception-2.0"]));
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    pub fn exceptions(&self) -> HashSet<&str> {
        self.inner.exceptions()
    }
}

impl Default for SpdxExpression {
    fn default() -> Self {
        Self::parse("NOASSERTION").expect("will not fail")
    }
}

impl Serialize for SpdxExpression {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.collect_str(self)
    }
}

struct SpdxExpressionVisitor;

impl<'de> Visitor<'de> for SpdxExpressionVisitor {
    type Value = SpdxExpression;

    fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        formatter.write_str("a syntactically valid SPDX expression")
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        SpdxExpression::parse(v)
            .map_err(|err| E::custom(format!("error parsing the expression: {}", err)))
    }

    fn visit_borrowed_str<E>(self, v: &'de str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        self.visit_str(v)
    }

    fn visit_string<E>(self, v: String) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        self.visit_str(&v)
    }
}

impl<'de> Deserialize<'de> for SpdxExpression {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        deserializer.deserialize_str(SpdxExpressionVisitor)
    }
}

impl Display for SpdxExpression {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.inner)
    }
}

#[cfg(test)]
mod tests {
    use std::iter::FromIterator;

    use serde_json::Value;

    use super::*;

    #[test]
    fn test_parsing_works() {
        let expression = SpdxExpression::parse("MIT AND (Apache-2.0 OR ISC)").unwrap();
        assert_eq!(expression.to_string(), "MIT AND (Apache-2.0 OR ISC)");
    }

    #[test]
    fn test_identifiers_from_simple_expression() {
        let expression = SpdxExpression::parse("MIT").unwrap();
        let licenses = expression.identifiers();
        assert_eq!(licenses, HashSet::from_iter(["MIT".to_string()]));
    }

    #[test]
    fn test_identifiers_from_compound_or_expression() {
        let expression = SpdxExpression::parse("MIT OR Apache-2.0").unwrap();
        let licenses = expression.identifiers();
        assert_eq!(
            licenses,
            HashSet::from_iter(["Apache-2.0".to_string(), "MIT".to_string()])
        );
    }

    #[test]
    fn test_identifiers_from_compound_parentheses_expression() {
        let expression = SpdxExpression::parse(
            "(MIT OR Apache-2.0 AND (GPL-2.0-only WITH Classpath-exception-2.0 OR ISC))",
        )
        .unwrap();
        let licenses = expression.identifiers();
        assert_eq!(
            licenses,
            HashSet::from_iter([
                "Apache-2.0".to_string(),
                "Classpath-exception-2.0".to_string(),
                "GPL-2.0-only".to_string(),
                "ISC".to_string(),
                "MIT".to_string()
            ])
        );
    }

    #[test]
    fn test_licenses_from_compound_parentheses_expression() {
        let expression = SpdxExpression::parse(
            "(MIT OR Apache-2.0 AND (GPL-2.0-only WITH Classpath-exception-2.0 OR ISC))",
        )
        .unwrap();

        let licenses = expression.licenses();

        assert_eq!(
            licenses
                .iter()
                .map(|&license| license.identifier.as_str())
                .collect::<HashSet<_>>(),
            HashSet::from_iter(["Apache-2.0", "GPL-2.0-only", "ISC", "MIT"])
        );
    }

    #[test]
    fn test_exceptions_from_compound_parentheses_expression() {
        let expression = SpdxExpression::parse(
            "(MIT OR Apache-2.0 AND (GPL-2.0-only WITH Classpath-exception-2.0 OR ISC))",
        )
        .unwrap();

        let exceptions = expression.exceptions();

        assert_eq!(exceptions, HashSet::from_iter(["Classpath-exception-2.0"]));
    }

    #[test]
    fn serialize_expression_correctly() {
        let expression = SpdxExpression::parse("MIT OR ISC").unwrap();

        let value = serde_json::to_value(expression).unwrap();

        assert_eq!(value, Value::String("MIT OR ISC".to_string()));
    }

    #[test]
    fn deserialize_expression_correctly() {
        let expected = SpdxExpression::parse("MIT OR ISC").unwrap();

        let value = Value::String("MIT OR ISC".to_string());

        let actual: SpdxExpression = serde_json::from_value(value).unwrap();

        assert_eq!(actual, expected);
    }
}

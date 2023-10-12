// SPDX-FileCopyrightText: 2022 HH Partners
//
// SPDX-License-Identifier: MIT

//! Private inner structs for [`crate::SpdxExpression`].

use std::{collections::HashSet, fmt::Display};

use nom::Finish;
use serde::{de::Visitor, Deserialize, Serialize};

use crate::{
    error::SpdxExpressionError,
    parser::{parse_expression, simple_expression},
};

/// Simple SPDX license expression.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct SimpleExpression {
    /// The license identifier.
    pub identifier: String,

    /// Optional DocumentRef for the expression.
    pub document_ref: Option<String>,

    /// `true` if the expression is a user defined license reference.
    pub license_ref: bool,
}

impl Serialize for SimpleExpression {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.collect_str(self)
    }
}

struct SimpleExpressionVisitor;

impl<'de> Visitor<'de> for SimpleExpressionVisitor {
    type Value = SimpleExpression;

    fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        formatter.write_str("a syntactically valid SPDX simple expression")
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        SimpleExpression::parse(v)
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

impl<'de> Deserialize<'de> for SimpleExpression {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        deserializer.deserialize_str(SimpleExpressionVisitor)
    }
}

impl Display for SimpleExpression {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let document_ref = match &self.document_ref {
            Some(document_ref) => {
                format!("DocumentRef-{}:", document_ref)
            }
            None => "".to_string(),
        };

        let license_ref = if self.license_ref { "LicenseRef-" } else { "" };
        write!(
            f,
            "{document_ref}{license_ref}{identifier}",
            identifier = self.identifier
        )
    }
}

impl SimpleExpression {
    /// Create a new simple expression.
    pub const fn new(identifier: String, document_ref: Option<String>, license_ref: bool) -> Self {
        Self {
            identifier,
            document_ref,
            license_ref,
        }
    }

    /// Parse a simple expression.
    ///
    /// # Examples
    ///
    /// ```
    /// # use spdx_expression::SimpleExpression;
    /// # use spdx_expression::SpdxExpressionError;
    /// #
    /// let expression = SimpleExpression::parse("MIT")?;
    /// # Ok::<(), SpdxExpressionError>(())
    /// ```
    ///
    /// The function will only accept simple expressions, compound expressions will fail.
    ///
    /// ```
    /// # use spdx_expression::SimpleExpression;
    /// #
    /// let expression = SimpleExpression::parse("MIT OR ISC");
    /// assert!(expression.is_err());
    /// ```
    ///
    /// # Errors
    ///
    /// Fails if parsing fails.
    pub fn parse(expression: &str) -> Result<Self, SpdxExpressionError> {
        let (remaining, result) = simple_expression(expression)?;

        if remaining.is_empty() {
            Ok(result)
        } else {
            Err(SpdxExpressionError::Parse(expression.to_string()))
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct WithExpression {
    pub license: SimpleExpression,
    pub exception: String,
}

impl WithExpression {
    pub const fn new(license: SimpleExpression, exception: String) -> Self {
        Self { license, exception }
    }
}

impl Display for WithExpression {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{license} WITH {exception}",
            license = self.license,
            exception = self.exception
        )
    }
}

#[derive(Debug, PartialEq, Clone, Eq)]
pub enum ExpressionVariant {
    Simple(SimpleExpression),
    With(WithExpression),
    And(Box<Self>, Box<Self>),
    Or(Box<Self>, Box<Self>),
    Parens(Box<Self>),
}

impl Display for ExpressionVariant {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use self::ExpressionVariant::{And, Or, Parens, Simple, With};

        match self {
            Simple(expression) => write!(f, "{expression}"),
            With(expression) => write!(f, "{expression}"),
            And(left, right) => write!(f, "{left} AND {right}"),
            Or(left, right) => write!(f, "{left} OR {right}"),
            Parens(expression) => write!(f, "({expression})"),
        }
    }
}

impl ExpressionVariant {
    pub fn parse(i: &str) -> Result<Self, SpdxExpressionError> {
        let (remaining, expression) = parse_expression(i)
            .finish()
            .map_err(|_| SpdxExpressionError::Parse(i.to_string()))?;

        if remaining.is_empty() {
            Ok(expression)
        } else {
            Err(SpdxExpressionError::Parse(i.to_string()))
        }
    }

    pub fn licenses(&self) -> HashSet<&SimpleExpression> {
        let mut expressions = HashSet::new();

        match self {
            ExpressionVariant::Simple(expression) => {
                expressions.insert(expression);
            }
            ExpressionVariant::With(expression) => {
                expressions.insert(&expression.license);
            }
            ExpressionVariant::And(left, right) | ExpressionVariant::Or(left, right) => {
                expressions.extend(left.licenses());
                expressions.extend(right.licenses());
            }
            ExpressionVariant::Parens(expression) => {
                expressions.extend(expression.licenses());
            }
        }

        expressions
    }

    pub fn exceptions(&self) -> HashSet<&str> {
        let mut expressions = HashSet::new();

        match self {
            ExpressionVariant::Simple(_) => {}
            ExpressionVariant::With(expression) => {
                expressions.insert(expression.exception.as_str());
            }
            ExpressionVariant::And(left, right) | ExpressionVariant::Or(left, right) => {
                expressions.extend(left.exceptions());
                expressions.extend(right.exceptions());
            }
            ExpressionVariant::Parens(expression) => {
                expressions.extend(expression.exceptions());
            }
        }

        expressions
    }
}

#[cfg(test)]
mod tests {
    use std::iter::FromIterator;

    use serde_json::Value;

    use super::*;

    #[test]
    fn display_simple_correctly() {
        let expression =
            ExpressionVariant::Simple(SimpleExpression::new("MIT".to_string(), None, false));
        assert_eq!(expression.to_string(), "MIT".to_string());
    }

    #[test]
    fn display_licenseref_correctly() {
        let expression =
            ExpressionVariant::Simple(SimpleExpression::new("license".to_string(), None, true));
        assert_eq!(expression.to_string(), "LicenseRef-license".to_string());
    }

    #[test]
    fn display_documentref_correctly() {
        let expression = ExpressionVariant::Simple(SimpleExpression::new(
            "license".to_string(),
            Some("document".to_string()),
            true,
        ));
        assert_eq!(
            expression.to_string(),
            "DocumentRef-document:LicenseRef-license".to_string()
        );
    }

    #[test]
    fn display_with_expression_correctly() {
        let expression = ExpressionVariant::With(WithExpression::new(
            SimpleExpression::new("license".to_string(), None, false),
            "exception".to_string(),
        ));
        assert_eq!(expression.to_string(), "license WITH exception".to_string());
    }

    #[test]
    fn display_and_expression_correctly() {
        let expression = ExpressionVariant::And(
            Box::new(ExpressionVariant::And(
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license1".to_string(),
                    None,
                    false,
                ))),
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license2".to_string(),
                    None,
                    false,
                ))),
            )),
            Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                "license3".to_string(),
                None,
                false,
            ))),
        );
        assert_eq!(
            expression.to_string(),
            "license1 AND license2 AND license3".to_string()
        );
    }

    #[test]
    fn display_or_expression_correctly() {
        let expression = ExpressionVariant::Or(
            Box::new(ExpressionVariant::Or(
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license1".to_string(),
                    None,
                    false,
                ))),
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license2".to_string(),
                    None,
                    false,
                ))),
            )),
            Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                "license3".to_string(),
                None,
                false,
            ))),
        );
        assert_eq!(
            expression.to_string(),
            "license1 OR license2 OR license3".to_string()
        );
    }

    #[test]
    fn get_licenses_correctly() {
        let expression = ExpressionVariant::And(
            Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                "license1+".to_string(),
                None,
                false,
            ))),
            Box::new(ExpressionVariant::Parens(Box::new(ExpressionVariant::Or(
                Box::new(ExpressionVariant::Parens(Box::new(
                    ExpressionVariant::With(WithExpression::new(
                        SimpleExpression::new("license2".to_string(), None, false),
                        "exception1".to_string(),
                    )),
                ))),
                Box::new(ExpressionVariant::And(
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license3+".to_string(),
                        None,
                        false,
                    ))),
                    Box::new(ExpressionVariant::With(WithExpression::new(
                        SimpleExpression::new("license4".to_string(), None, false),
                        "exception2".to_string(),
                    ))),
                )),
            )))),
        );

        assert_eq!(
            expression.licenses(),
            HashSet::from_iter([
                &SimpleExpression::new("license1+".to_string(), None, false),
                &SimpleExpression::new("license2".to_string(), None, false),
                &SimpleExpression::new("license3+".to_string(), None, false),
                &SimpleExpression::new("license4".to_string(), None, false),
            ])
        );
    }
    #[test]
    fn get_exceptions_correctly() {
        let expression = ExpressionVariant::And(
            Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                "license1+".to_string(),
                None,
                false,
            ))),
            Box::new(ExpressionVariant::Parens(Box::new(ExpressionVariant::Or(
                Box::new(ExpressionVariant::Parens(Box::new(
                    ExpressionVariant::With(WithExpression::new(
                        SimpleExpression::new("license2".to_string(), None, false),
                        "exception1".to_string(),
                    )),
                ))),
                Box::new(ExpressionVariant::And(
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license3+".to_string(),
                        None,
                        false,
                    ))),
                    Box::new(ExpressionVariant::With(WithExpression::new(
                        SimpleExpression::new("license4".to_string(), None, false),
                        "exception2".to_string(),
                    ))),
                )),
            )))),
        );

        assert_eq!(
            expression.exceptions(),
            HashSet::from_iter(["exception1", "exception2"])
        );
    }

    #[test]
    fn parse_simple_expression() {
        let expression = SimpleExpression::parse("MIT").unwrap();
        assert_eq!(
            expression,
            SimpleExpression::new("MIT".to_string(), None, false)
        );

        let expression = SimpleExpression::parse("MIT OR ISC");
        assert!(expression.is_err());

        let expression = SimpleExpression::parse("GPL-2.0-only WITH Classpath-exception-2.0");
        assert!(expression.is_err());
    }

    #[test]
    fn serialize_simple_expression_correctly() {
        let expression = SimpleExpression::parse("MIT").unwrap();

        let value = serde_json::to_value(expression).unwrap();

        assert_eq!(value, Value::String("MIT".to_string()));
    }

    #[test]
    fn deserialize_simple_expression_correctly() {
        let expected = SimpleExpression::parse("LicenseRef-license1").unwrap();

        let value = Value::String("LicenseRef-license1".to_string());

        let actual: SimpleExpression = serde_json::from_value(value).unwrap();

        assert_eq!(actual, expected);
    }
}

// SPDX-FileCopyrightText: 2022 HH Partners
//
// SPDX-License-Identifier: MIT

//! Parsing logic for SPDX Expressions.
//!
//! The code is heavily inspired by
//! <https://github.com/Geal/nom/blob/294ffb3d9e0ade2c3b7ddfff52484b6d643dcce1/tests/arithmetic_ast.rs>
//! which is licensed under the MIT License. The source project includes the following copyright
//! statement: Copyright (c) 2014-2019 Geoffroy Couprie.

use nom::{
    branch::alt,
    bytes::complete::{tag, tag_no_case, take_while1},
    character::{
        complete::{multispace0, multispace1},
        streaming::char,
    },
    combinator::{complete, map, opt, recognize},
    multi::many0,
    sequence::{delimited, pair, preceded, separated_pair},
    AsChar, IResult,
};

use crate::expression_variant::{ExpressionVariant, SimpleExpression, WithExpression};

#[derive(Debug)]
enum Operator {
    And,
    Or,
}

fn parentheses(i: &str) -> IResult<&str, ExpressionVariant> {
    delimited(
        multispace0,
        delimited(
            tag("("),
            map(or_expression, |e| ExpressionVariant::Parens(Box::new(e))),
            tag(")"),
        ),
        multispace0,
    )(i)
}

fn terminal_expression(i: &str) -> IResult<&str, ExpressionVariant> {
    alt((
        delimited(multispace0, with_expression, multispace0),
        map(
            delimited(multispace0, simple_expression, multispace0),
            ExpressionVariant::Simple,
        ),
        parentheses,
    ))(i)
}

fn with_expression(i: &str) -> IResult<&str, ExpressionVariant> {
    map(
        separated_pair(
            simple_expression,
            delimited(multispace1, tag_no_case("WITH"), multispace1),
            idstring,
        ),
        |(lic, exc)| ExpressionVariant::With(WithExpression::new(lic, exc.to_string())),
    )(i)
}

fn fold_expressions(
    initial: ExpressionVariant,
    remainder: Vec<(Operator, ExpressionVariant)>,
) -> ExpressionVariant {
    remainder.into_iter().fold(initial, |acc, pair| {
        let (oper, expr) = pair;
        match oper {
            Operator::And => ExpressionVariant::And(Box::new(acc), Box::new(expr)),
            Operator::Or => ExpressionVariant::Or(Box::new(acc), Box::new(expr)),
        }
    })
}

fn and_expression(i: &str) -> IResult<&str, ExpressionVariant> {
    let (i, initial) = terminal_expression(i)?;
    let (i, remainder) = many0(|i| {
        let (i, and) = preceded(tag_no_case("AND"), terminal_expression)(i)?;
        Ok((i, (Operator::And, and)))
    })(i)?;

    Ok((i, fold_expressions(initial, remainder)))
}

fn or_expression(i: &str) -> IResult<&str, ExpressionVariant> {
    let (i, initial) = and_expression(i)?;
    let (i, remainder) = many0(|i| {
        let (i, or) = preceded(tag_no_case("OR"), and_expression)(i)?;
        Ok((i, (Operator::Or, or)))
    })(i)?;

    Ok((i, fold_expressions(initial, remainder)))
}

pub fn parse_expression(i: &str) -> IResult<&str, ExpressionVariant> {
    or_expression(i)
}

fn idstring(i: &str) -> IResult<&str, &str> {
    take_while1(|c: char| c.is_alphanum() || c == '-' || c == '.')(i)
}

fn license_idstring(i: &str) -> IResult<&str, &str> {
    recognize(pair(idstring, opt(complete(char('+')))))(i)
}

fn document_ref(i: &str) -> IResult<&str, &str> {
    delimited(tag("DocumentRef-"), idstring, char(':'))(i)
}

fn license_ref(i: &str) -> IResult<&str, (Option<&str>, &str)> {
    separated_pair(opt(document_ref), tag("LicenseRef-"), idstring)(i)
}

pub fn simple_expression(i: &str) -> IResult<&str, SimpleExpression> {
    alt((
        map(license_ref, |(document_ref, id)| {
            let document_ref = document_ref.map(std::string::ToString::to_string);
            SimpleExpression::new(id.to_string(), document_ref, true)
        }),
        map(license_idstring, |id| {
            SimpleExpression::new(id.to_string(), None, false)
        }),
    ))(i)
}

#[cfg(test)]
mod tests {
    //! A lot of the test cases for parsing are copied from
    //! <https://github.com/oss-review-toolkit/ort/blob/6eb18b6d36f59c6d7ec221bad1cf5d4cd6acfc8b/utils/spdx/src/test/kotlin/SpdxExpressionParserTest.kt>
    //! which is licensed under the Apache License, Version 2.0 and includes the following copyright
    //! statement:
    //! Copyright (C) 2017-2019 HERE Europe B.V.

    use super::*;

    use pretty_assertions::assert_eq;

    #[test]
    fn parse_a_license_id_correctly() {
        let parsed = ExpressionVariant::parse("spdx.license-id").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Simple(SimpleExpression::new(
                "spdx.license-id".to_string(),
                None,
                false
            ))
        );
    }

    #[test]
    fn parse_a_license_id_starting_with_a_digit_correctly() {
        let parsed = ExpressionVariant::parse("0license").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Simple(SimpleExpression::new("0license".to_string(), None, false))
        );
    }

    #[test]
    fn parse_a_license_id_with_any_later_version_correctly() {
        let parsed = ExpressionVariant::parse("license+").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Simple(SimpleExpression::new("license+".to_string(), None, false))
        );
    }

    #[test]
    fn parse_a_document_ref_correctly() {
        let parsed = ExpressionVariant::parse("DocumentRef-document:LicenseRef-license").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Simple(SimpleExpression::new(
                "license".to_string(),
                Some("document".to_string()),
                true
            ))
        );
    }

    #[test]
    fn parse_a_license_ref_correctly() {
        let parsed = ExpressionVariant::parse("LicenseRef-license").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Simple(SimpleExpression::new("license".to_string(), None, true))
        );
    }

    #[test]
    fn parse_a_with_expression_correctly() {
        let parsed = ExpressionVariant::parse("license WITH exception").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::With(WithExpression::new(
                SimpleExpression::new("license".to_string(), None, false),
                "exception".to_string()
            ))
        );
    }

    #[test]
    fn parse_a_complex_expression_correctly() {
        let parsed = ExpressionVariant::parse(
            "license1+ and ((license2 with exception1) OR license3+ AND license4 WITH exception2)",
        )
        .unwrap();

        assert_eq!(
            parsed,
            ExpressionVariant::And(
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license1+".to_string(),
                    None,
                    false
                ))),
                Box::new(ExpressionVariant::Parens(Box::new(ExpressionVariant::Or(
                    Box::new(ExpressionVariant::Parens(Box::new(
                        ExpressionVariant::With(WithExpression::new(
                            SimpleExpression::new("license2".to_string(), None, false),
                            "exception1".to_string()
                        ))
                    ))),
                    Box::new(ExpressionVariant::And(
                        Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                            "license3+".to_string(),
                            None,
                            false
                        ))),
                        Box::new(ExpressionVariant::With(WithExpression::new(
                            SimpleExpression::new("license4".to_string(), None, false),
                            "exception2".to_string()
                        )))
                    )),
                ))))
            )
        );
    }

    #[test]
    fn bind_plus_stronger_than_with() {
        let parsed = ExpressionVariant::parse("license+ WITH exception").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::With(WithExpression::new(
                SimpleExpression::new("license+".to_string(), None, false),
                "exception".to_string()
            ))
        );
    }

    #[test]
    fn bind_with_stronger_than_and() {
        let parsed = ExpressionVariant::parse("license1 AND license2 WITH exception").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::And(
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license1".to_string(),
                    None,
                    false
                ))),
                Box::new(ExpressionVariant::With(WithExpression::new(
                    SimpleExpression::new("license2".to_string(), None, false),
                    "exception".to_string()
                )))
            )
        );
    }

    #[test]
    fn bind_and_stronger_than_or() {
        let parsed = ExpressionVariant::parse("license1 OR license2 AND license3").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Or(
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license1".to_string(),
                    None,
                    false
                ))),
                Box::new(ExpressionVariant::And(
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license2".to_string(),
                        None,
                        false
                    ))),
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license3".to_string(),
                        None,
                        false
                    )))
                ))
            )
        );
    }

    #[test]
    fn bind_the_and_operator_left_associative() {
        let parsed = ExpressionVariant::parse("license1 AND license2 AND license3").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::And(
                Box::new(ExpressionVariant::And(
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license1".to_string(),
                        None,
                        false
                    ))),
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license2".to_string(),
                        None,
                        false
                    )))
                )),
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license3".to_string(),
                    None,
                    false
                ))),
            )
        );
    }

    #[test]
    fn bind_the_or_operator_left_associative() {
        let parsed = ExpressionVariant::parse("license1 OR license2 OR license3").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::Or(
                Box::new(ExpressionVariant::Or(
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license1".to_string(),
                        None,
                        false
                    ))),
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license2".to_string(),
                        None,
                        false
                    )))
                )),
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license3".to_string(),
                    None,
                    false
                ))),
            )
        );
    }

    #[test]
    fn respect_parentheses_for_binding_strength_of_operators() {
        let parsed = ExpressionVariant::parse("(license1 OR license2) AND license3").unwrap();
        assert_eq!(
            parsed,
            ExpressionVariant::And(
                Box::new(ExpressionVariant::Parens(Box::new(ExpressionVariant::Or(
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license1".to_string(),
                        None,
                        false
                    ))),
                    Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                        "license2".to_string(),
                        None,
                        false
                    )))
                )))),
                Box::new(ExpressionVariant::Simple(SimpleExpression::new(
                    "license3".to_string(),
                    None,
                    false
                ))),
            )
        );
    }

    #[test]
    fn fail_if_plus_is_used_in_an_exception_expression() {
        let parsed = ExpressionVariant::parse("license WITH exception+");
        assert!(parsed.is_err());
    }

    #[test]
    fn fail_if_a_compound_expressions_is_used_before_with() {
        let parsed = ExpressionVariant::parse("(license1 AND license2) WITH exception");
        assert!(parsed.is_err());
    }

    #[test]
    fn fail_on_an_invalid_symbol() {
        let parsed = ExpressionVariant::parse("/");
        assert!(parsed.is_err());
    }

    #[test]
    fn fail_on_a_syntax_error() {
        let parsed = ExpressionVariant::parse("((");
        assert!(parsed.is_err());
    }
}

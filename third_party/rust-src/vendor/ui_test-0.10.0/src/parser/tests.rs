use crate::{
    parser::{Condition, Pattern},
    Error,
};

use super::Comments;

#[test]
fn parse_simple_comment() {
    let s = r"
use std::mem;

fn main() {
    let _x: &i32 = unsafe { mem::transmute(16usize) }; //~ ERROR: encountered a dangling reference (address $HEX is unallocated)
}
    ";
    let comments = Comments::parse(s).unwrap();
    println!("parsed comments: {:#?}", comments);
    assert_eq!(comments.revisioned.len(), 1);
    let revisioned = &comments.revisioned[&vec![]];
    assert_eq!(revisioned.error_matches[0].definition_line, 5);
    match &revisioned.error_matches[0].pattern {
        Pattern::SubString(s) => {
            assert_eq!(
                s,
                "encountered a dangling reference (address $HEX is unallocated)"
            )
        }
        other => panic!("expected substring, got {other:?}"),
    }
}

#[test]
fn parse_missing_level() {
    let s = r"
use std::mem;

fn main() {
    let _x: &i32 = unsafe { mem::transmute(16usize) }; //~ encountered a dangling reference (address $HEX is unallocated)
}
    ";
    let errors = Comments::parse(s).unwrap_err();
    println!("parsed comments: {:#?}", errors);
    assert_eq!(errors.len(), 1);
    match &errors[0] {
        Error::InvalidComment { msg, line: 5 } => assert_eq!(msg, "unknown level `encountered`"),
        _ => unreachable!(),
    }
}

#[test]
fn parse_slash_slash_at() {
    let s = r"
//@  error-in-other-file:  foomp
use std::mem;

    ";
    let comments = Comments::parse(s).unwrap();
    println!("parsed comments: {:#?}", comments);
    assert_eq!(comments.revisioned.len(), 1);
    let revisioned = &comments.revisioned[&vec![]];
    let pat = &revisioned.error_in_other_files[0];
    assert_eq!(format!("{:?}", pat.0), r#"SubString("foomp")"#);
    assert_eq!(pat.1, 2);
}

#[test]
fn parse_regex_error_pattern() {
    let s = r"
//@  error-in-other-file:  /foomp/
use std::mem;

    ";
    let comments = Comments::parse(s).unwrap();
    println!("parsed comments: {:#?}", comments);
    assert_eq!(comments.revisioned.len(), 1);
    let revisioned = &comments.revisioned[&vec![]];
    let pat = &revisioned.error_in_other_files[0];
    assert_eq!(format!("{:?}", pat.0), r#"Regex(foomp)"#);
    assert_eq!(pat.1, 2);
}

#[test]
fn parse_slash_slash_at_fail() {
    let s = r"
//@  error-patttern  foomp
use std::mem;

    ";
    let errors = Comments::parse(s).unwrap_err();
    println!("parsed comments: {:#?}", errors);
    assert_eq!(errors.len(), 2);
    match &errors[0] {
        Error::InvalidComment { msg, line: 2 } => {
            assert!(msg.contains("must be followed by `:`"))
        }
        _ => unreachable!(),
    }
    match &errors[1] {
        Error::InvalidComment { msg, line: 2 } => {
            assert_eq!(msg, "unknown command `error-patttern`");
        }
        _ => unreachable!(),
    }
}

#[test]
fn missing_colon_fail() {
    let s = r"
//@stderr-per-bitwidth hello
use std::mem;

    ";
    let errors = Comments::parse(s).unwrap_err();
    println!("parsed comments: {:#?}", errors);
    assert_eq!(errors.len(), 1);
    match &errors[0] {
        Error::InvalidComment { msg, line: 2 } => {
            assert!(msg.contains("must be followed by `:`"))
        }
        _ => unreachable!(),
    }
}

#[test]
fn parse_x86_64() {
    let s = r"//@ only-target-x86_64-unknown-linux";
    let comments = Comments::parse(s).unwrap();
    println!("parsed comments: {:#?}", comments);
    assert_eq!(comments.revisioned.len(), 1);
    let revisioned = &comments.revisioned[&vec![]];
    assert_eq!(revisioned.only.len(), 1);
    match &revisioned.only[0] {
        Condition::Target(t) => assert_eq!(t, "x86_64-unknown-linux"),
        _ => unreachable!(),
    }
}

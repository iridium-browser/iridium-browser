// Copyright 2012-2015 The Rust Project Developers. See the COPYRIGHT
// file at the top-level directory of this distribution and at
// http://rust-lang.org/COPYRIGHT.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#[test]
fn test_general_security_profile_identifier_allowed() {
    use crate::GeneralSecurityProfile;
    assert_eq!(GeneralSecurityProfile::identifier_allowed('A'), true);
    assert_eq!('A'.identifier_allowed(), true);
    assert_eq!(GeneralSecurityProfile::identifier_allowed('0'), true);
    assert_eq!('0'.identifier_allowed(), true);
    assert_eq!(GeneralSecurityProfile::identifier_allowed('_'), true);
    assert_eq!('_'.identifier_allowed(), true);
    assert_eq!(GeneralSecurityProfile::identifier_allowed('\x00'), false);
    assert_eq!('\x00'.identifier_allowed(), false);
    // U+00B5 MICRO SIGN
    assert_eq!(GeneralSecurityProfile::identifier_allowed('µ'), false);
    assert_eq!('µ'.identifier_allowed(), false);
    // U+2160 ROMAN NUMERAL ONE
    assert_eq!(GeneralSecurityProfile::identifier_allowed('Ⅰ'), false);
    assert_eq!('Ⅰ'.identifier_allowed(), false);
}

#[test]
fn test_mixed_script() {
    use crate::MixedScript;
    assert_eq!("".is_single_script(), true);
    assert_eq!("".resolve_script_set().is_empty(), false);
    assert_eq!("".resolve_script_set().is_all(), true);
    assert_eq!("A".is_single_script(), true);
    assert_eq!("A".resolve_script_set().is_empty(), false);
    assert_eq!("A".resolve_script_set().is_all(), false);
    assert_eq!("A0".is_single_script(), true);
    assert_eq!("A0".resolve_script_set().is_empty(), false);
    assert_eq!("A0".resolve_script_set().is_all(), false);
    assert_eq!("0.".is_single_script(), true);
    assert_eq!("0.".resolve_script_set().is_empty(), false);
    assert_eq!("0.".resolve_script_set().is_all(), true);
    assert_eq!("福".is_single_script(), true);
    assert_eq!("福".resolve_script_set().is_empty(), false);
    assert_eq!("福".resolve_script_set().is_all(), false);
    assert_eq!("冬の雪".is_single_script(), true);
    assert_eq!("冬の雪".resolve_script_set().is_empty(), false);
    assert_eq!("冬の雪".resolve_script_set().is_all(), false);
    assert_eq!("幻ㄒㄧㄤ".is_single_script(), true);
    assert_eq!("幻ㄒㄧㄤ".resolve_script_set().is_empty(), false);
    assert_eq!("幻ㄒㄧㄤ".resolve_script_set().is_all(), false);
    assert_eq!("日出은".is_single_script(), true);
    assert_eq!("日出은".resolve_script_set().is_empty(), false);
    assert_eq!("日出은".resolve_script_set().is_all(), false);
    assert_eq!("夏の幻ㄒㄧㄤ".is_single_script(), false);
    assert_eq!("夏の幻ㄒㄧㄤ".resolve_script_set().is_empty(), true);
    assert_eq!("夏の幻ㄒㄧㄤ".resolve_script_set().is_all(), false);
}

#[test]
fn test_confusable_detection() {
    use crate::skeleton;
    use std::string::String;
    assert_eq!(&skeleton("").collect::<String>(), "");
    assert_eq!(&skeleton("ｓ").collect::<String>(), "s");
    assert_eq!(&skeleton("ｓｓｓ").collect::<String>(), "sss");
    assert_eq!(&skeleton("ﶛ").collect::<String>(), "نمى");
    assert_eq!(&skeleton("ﶛﶛ").collect::<String>(), "نمىنمى");
}

#[test]
fn test_potential_mixed_script_detection() {
    use crate::is_potential_mixed_script_confusable_char;

    assert!(is_potential_mixed_script_confusable_char('A'));
    assert!(!is_potential_mixed_script_confusable_char('D'));
}

#[test]
fn test_augmented_script_set_fmt_debug() {
    use crate::mixed_script::AugmentedScriptSet;
    let augmented_script_sets = vec![
        AugmentedScriptSet::default(),
        AugmentedScriptSet::from('0'),
        AugmentedScriptSet::from('a'),
        AugmentedScriptSet::from('μ'),
        AugmentedScriptSet::from('汉'),
        AugmentedScriptSet::from('ひ'),
        AugmentedScriptSet::from('カ'),
        AugmentedScriptSet::from('한'),
        AugmentedScriptSet::from("汉ひ"),
        AugmentedScriptSet::from("汉a"),
        AugmentedScriptSet::from("汉μ"),
        AugmentedScriptSet::from("〆切"),
    ];
    let debug_output = vec![
        "AugmentedScriptSet {ALL}",
        "AugmentedScriptSet {ALL}",
        "AugmentedScriptSet {Latn}",
        "AugmentedScriptSet {Grek}",
        "AugmentedScriptSet {Hanb, Jpan, Kore, Hani}",
        "AugmentedScriptSet {Jpan, Hira}",
        "AugmentedScriptSet {Jpan, Kana}",
        "AugmentedScriptSet {Kore, Hang}",
        "AugmentedScriptSet {Jpan}",
        "AugmentedScriptSet {∅}",
        "AugmentedScriptSet {∅}",
        "AugmentedScriptSet {Hanb, Jpan, Kore, Hani}",
    ];

    for (ss, output) in augmented_script_sets.into_iter().zip(debug_output) {
        assert_eq!(format!("{:?}", ss), output);
    }
}

#[test]
fn test_augmented_script_set_fmt_display() {
    use crate::mixed_script::AugmentedScriptSet;
    let augmented_script_sets = vec![
        AugmentedScriptSet::default(),
        AugmentedScriptSet::from('0'),
        AugmentedScriptSet::from('a'),
        AugmentedScriptSet::from('μ'),
        AugmentedScriptSet::from('汉'),
        AugmentedScriptSet::from('ひ'),
        AugmentedScriptSet::from('カ'),
        AugmentedScriptSet::from('한'),
        AugmentedScriptSet::from("汉ひ"),
        AugmentedScriptSet::from("汉a"),
        AugmentedScriptSet::from("汉μ"),
        AugmentedScriptSet::from("〆切"),
    ];
    let debug_output = vec![
        "All",
        "All",
        "Latin",
        "Greek",
        "Han with Bopomofo, Japanese, Korean, Han",
        "Japanese, Hiragana",
        "Japanese, Katakana",
        "Korean, Hangul",
        "Japanese",
        "Empty",
        "Empty",
        "Han with Bopomofo, Japanese, Korean, Han",
    ];

    for (ss, output) in augmented_script_sets.into_iter().zip(debug_output) {
        assert_eq!(format!("{}", ss), output);
    }
}

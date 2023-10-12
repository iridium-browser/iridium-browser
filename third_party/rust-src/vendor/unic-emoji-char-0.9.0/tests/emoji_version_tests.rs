// Copyright 2017 The UNIC Project Developers.
//
// See the COPYRIGHT file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use unic_emoji_char;
use unic_ucd_version;

#[test]
fn test_emoji_data_version_against_ucd_version() {
    // At the moment, Emoji Version is strictly smaller than Unicode Version. Emoji Version is going
    // to be *synced* with Unicode Version, but may receive minor updates separately.
    assert!(unic_emoji_char::EMOJI_VERSION.major <= unic_ucd_version::UNICODE_VERSION.major);
}

// Copyright 2017 The UNIC Project Developers.
//
// See the COPYRIGHT file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Unicode `Emoji_Modifier_Base` Character Property.

char_property! {
    /// Represents values of the Unicode character property
    /// [`Emoji_Modifier_Base`](https://www.unicode.org/reports/tr51/#Emoji_Properties).
    ///
    /// The value is `true` for characters that are emoji modifiers.
    pub struct EmojiModifierBase(bool) {
        abbr => "Emoji_Modifier_Base";
        long => "Emoji_Modifier_Base";
        human => "Emoji Modifier Base";

        data_table_path => "../tables/emoji_modifier_base.rsv";
    }

    /// The value is `true` for characters that are emoji modifiers.
    pub fn is_emoji_modifier_base(char) -> bool;
}

#[cfg(test)]
mod tests {
    #[test]
    fn test_values() {
        use super::is_emoji_modifier_base;

        // ASCII
        assert_eq!(is_emoji_modifier_base('\u{0000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{0021}'), false);

        assert_eq!(is_emoji_modifier_base('\u{0027}'), false);
        assert_eq!(is_emoji_modifier_base('\u{0028}'), false);
        assert_eq!(is_emoji_modifier_base('\u{0029}'), false);
        assert_eq!(is_emoji_modifier_base('\u{002a}'), false);

        assert_eq!(is_emoji_modifier_base('\u{003b}'), false);
        assert_eq!(is_emoji_modifier_base('\u{003c}'), false);
        assert_eq!(is_emoji_modifier_base('\u{003d}'), false);

        assert_eq!(is_emoji_modifier_base('\u{007a}'), false);
        assert_eq!(is_emoji_modifier_base('\u{007b}'), false);
        assert_eq!(is_emoji_modifier_base('\u{007c}'), false);
        assert_eq!(is_emoji_modifier_base('\u{007d}'), false);
        assert_eq!(is_emoji_modifier_base('\u{007e}'), false);

        // Other BMP
        assert_eq!(is_emoji_modifier_base('\u{061b}'), false);
        assert_eq!(is_emoji_modifier_base('\u{061c}'), false);
        assert_eq!(is_emoji_modifier_base('\u{061d}'), false);

        assert_eq!(is_emoji_modifier_base('\u{200d}'), false);
        assert_eq!(is_emoji_modifier_base('\u{200e}'), false);
        assert_eq!(is_emoji_modifier_base('\u{200f}'), false);
        assert_eq!(is_emoji_modifier_base('\u{2010}'), false);

        assert_eq!(is_emoji_modifier_base('\u{2029}'), false);
        assert_eq!(is_emoji_modifier_base('\u{202a}'), false);
        assert_eq!(is_emoji_modifier_base('\u{202e}'), false);
        assert_eq!(is_emoji_modifier_base('\u{202f}'), false);

        // Other Planes
        assert_eq!(is_emoji_modifier_base('\u{10000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{10001}'), false);

        assert_eq!(is_emoji_modifier_base('\u{1f1e5}'), false);
        assert_eq!(is_emoji_modifier_base('\u{1f1e6}'), false);
        assert_eq!(is_emoji_modifier_base('\u{1f1ff}'), false);
        assert_eq!(is_emoji_modifier_base('\u{1f200}'), false);

        assert_eq!(is_emoji_modifier_base('\u{20000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{30000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{40000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{50000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{60000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{70000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{80000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{90000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{a0000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{b0000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{c0000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{d0000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{e0000}'), false);

        assert_eq!(is_emoji_modifier_base('\u{efffe}'), false);
        assert_eq!(is_emoji_modifier_base('\u{effff}'), false);

        // Priavte-Use Area
        assert_eq!(is_emoji_modifier_base('\u{f0000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{f0001}'), false);
        assert_eq!(is_emoji_modifier_base('\u{ffffe}'), false);
        assert_eq!(is_emoji_modifier_base('\u{fffff}'), false);
        assert_eq!(is_emoji_modifier_base('\u{100000}'), false);
        assert_eq!(is_emoji_modifier_base('\u{100001}'), false);
        assert_eq!(is_emoji_modifier_base('\u{10fffe}'), false);
        assert_eq!(is_emoji_modifier_base('\u{10ffff}'), false);
    }
}

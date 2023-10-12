// Copyright 2017 The UNIC Project Developers.
//
// See the COPYRIGHT file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Unicode `Emoji_Component` Character Property.

char_property! {
    /// Represents values of the Unicode character property
    /// [`Emoji_Component`](https://www.unicode.org/reports/tr51/#Emoji_Properties).
    ///
    /// The value is `true` for characters that normally do not appear on emoji keyboards as
    /// separate choices, such as Keycap base characters, Regional_Indicators, …, `false` otherwise.
    pub struct EmojiComponent(bool) {
        abbr => "Emoji_Component";
        long => "Emoji_Component";
        human => "Emoji Component";

        data_table_path => "../tables/emoji_component.rsv";
    }

    /// The value is `true` for characters that normally do not appear on emoji keyboards as
    /// separate choices, such as Keycap base characters, Regional_Indicators, …, `false` otherwise.
    pub fn is_emoji_component(char) -> bool;
}

#[cfg(test)]
mod tests {
    #[test]
    fn test_values() {
        use super::is_emoji_component;

        // ASCII
        assert_eq!(is_emoji_component('\u{0000}'), false);
        assert_eq!(is_emoji_component('\u{0021}'), false);

        assert_eq!(is_emoji_component('\u{0027}'), false);
        assert_eq!(is_emoji_component('\u{0028}'), false);
        assert_eq!(is_emoji_component('\u{0029}'), false);
        assert_eq!(is_emoji_component('\u{002a}'), true);

        assert_eq!(is_emoji_component('\u{003b}'), false);
        assert_eq!(is_emoji_component('\u{003c}'), false);
        assert_eq!(is_emoji_component('\u{003d}'), false);

        assert_eq!(is_emoji_component('\u{007a}'), false);
        assert_eq!(is_emoji_component('\u{007b}'), false);
        assert_eq!(is_emoji_component('\u{007c}'), false);
        assert_eq!(is_emoji_component('\u{007d}'), false);
        assert_eq!(is_emoji_component('\u{007e}'), false);

        // Other BMP
        assert_eq!(is_emoji_component('\u{061b}'), false);
        assert_eq!(is_emoji_component('\u{061c}'), false);
        assert_eq!(is_emoji_component('\u{061d}'), false);

        assert_eq!(is_emoji_component('\u{200d}'), false);
        assert_eq!(is_emoji_component('\u{200e}'), false);
        assert_eq!(is_emoji_component('\u{200f}'), false);
        assert_eq!(is_emoji_component('\u{2010}'), false);

        assert_eq!(is_emoji_component('\u{2029}'), false);
        assert_eq!(is_emoji_component('\u{202a}'), false);
        assert_eq!(is_emoji_component('\u{202e}'), false);
        assert_eq!(is_emoji_component('\u{202f}'), false);

        // Other Planes
        assert_eq!(is_emoji_component('\u{10000}'), false);
        assert_eq!(is_emoji_component('\u{10001}'), false);

        assert_eq!(is_emoji_component('\u{1f1e5}'), false);
        assert_eq!(is_emoji_component('\u{1f1e6}'), true);
        assert_eq!(is_emoji_component('\u{1f1ff}'), true);
        assert_eq!(is_emoji_component('\u{1f200}'), false);

        assert_eq!(is_emoji_component('\u{20000}'), false);
        assert_eq!(is_emoji_component('\u{30000}'), false);
        assert_eq!(is_emoji_component('\u{40000}'), false);
        assert_eq!(is_emoji_component('\u{50000}'), false);
        assert_eq!(is_emoji_component('\u{60000}'), false);
        assert_eq!(is_emoji_component('\u{70000}'), false);
        assert_eq!(is_emoji_component('\u{80000}'), false);
        assert_eq!(is_emoji_component('\u{90000}'), false);
        assert_eq!(is_emoji_component('\u{a0000}'), false);
        assert_eq!(is_emoji_component('\u{b0000}'), false);
        assert_eq!(is_emoji_component('\u{c0000}'), false);
        assert_eq!(is_emoji_component('\u{d0000}'), false);
        assert_eq!(is_emoji_component('\u{e0000}'), false);

        assert_eq!(is_emoji_component('\u{efffe}'), false);
        assert_eq!(is_emoji_component('\u{effff}'), false);

        // Priavte-Use Area
        assert_eq!(is_emoji_component('\u{f0000}'), false);
        assert_eq!(is_emoji_component('\u{f0001}'), false);
        assert_eq!(is_emoji_component('\u{ffffe}'), false);
        assert_eq!(is_emoji_component('\u{fffff}'), false);
        assert_eq!(is_emoji_component('\u{100000}'), false);
        assert_eq!(is_emoji_component('\u{100001}'), false);
        assert_eq!(is_emoji_component('\u{10fffe}'), false);
        assert_eq!(is_emoji_component('\u{10ffff}'), false);
    }
}

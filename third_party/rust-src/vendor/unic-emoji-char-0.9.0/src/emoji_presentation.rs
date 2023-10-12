// Copyright 2017 The UNIC Project Developers.
//
// See the COPYRIGHT file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Unicode `Emoji_Presentation` Character Property.

char_property! {
    /// Represents values of the Unicode character property
    /// [`Emoji_Presentation`](https://www.unicode.org/reports/tr51/#Emoji_Properties).
    ///
    /// The value is `true` for characters that have emoji presentation by default.
    pub struct EmojiPresentation(bool) {
        abbr => "Emoji_Presentation";
        long => "Emoji_Presentation";
        human => "Emoji Presentation";

        data_table_path => "../tables/emoji_presentation.rsv";
    }

    /// The value is `true` for characters that have emoji presentation by default.
    pub fn is_emoji_presentation(char) -> bool;
}

#[cfg(test)]
mod tests {
    #[test]
    fn test_values() {
        use super::is_emoji_presentation;

        // ASCII
        assert_eq!(is_emoji_presentation('\u{0000}'), false);
        assert_eq!(is_emoji_presentation('\u{0021}'), false);

        assert_eq!(is_emoji_presentation('\u{0027}'), false);
        assert_eq!(is_emoji_presentation('\u{0028}'), false);
        assert_eq!(is_emoji_presentation('\u{0029}'), false);
        assert_eq!(is_emoji_presentation('\u{002a}'), false);

        assert_eq!(is_emoji_presentation('\u{003b}'), false);
        assert_eq!(is_emoji_presentation('\u{003c}'), false);
        assert_eq!(is_emoji_presentation('\u{003d}'), false);

        assert_eq!(is_emoji_presentation('\u{007a}'), false);
        assert_eq!(is_emoji_presentation('\u{007b}'), false);
        assert_eq!(is_emoji_presentation('\u{007c}'), false);
        assert_eq!(is_emoji_presentation('\u{007d}'), false);
        assert_eq!(is_emoji_presentation('\u{007e}'), false);

        // Other BMP
        assert_eq!(is_emoji_presentation('\u{061b}'), false);
        assert_eq!(is_emoji_presentation('\u{061c}'), false);
        assert_eq!(is_emoji_presentation('\u{061d}'), false);

        assert_eq!(is_emoji_presentation('\u{200d}'), false);
        assert_eq!(is_emoji_presentation('\u{200e}'), false);
        assert_eq!(is_emoji_presentation('\u{200f}'), false);
        assert_eq!(is_emoji_presentation('\u{2010}'), false);

        assert_eq!(is_emoji_presentation('\u{2029}'), false);
        assert_eq!(is_emoji_presentation('\u{202a}'), false);
        assert_eq!(is_emoji_presentation('\u{202e}'), false);
        assert_eq!(is_emoji_presentation('\u{202f}'), false);

        // Other Planes
        assert_eq!(is_emoji_presentation('\u{10000}'), false);
        assert_eq!(is_emoji_presentation('\u{10001}'), false);

        assert_eq!(is_emoji_presentation('\u{1f1e5}'), false);
        assert_eq!(is_emoji_presentation('\u{1f1e6}'), true);
        assert_eq!(is_emoji_presentation('\u{1f1ff}'), true);
        assert_eq!(is_emoji_presentation('\u{1f200}'), false);

        assert_eq!(is_emoji_presentation('\u{20000}'), false);
        assert_eq!(is_emoji_presentation('\u{30000}'), false);
        assert_eq!(is_emoji_presentation('\u{40000}'), false);
        assert_eq!(is_emoji_presentation('\u{50000}'), false);
        assert_eq!(is_emoji_presentation('\u{60000}'), false);
        assert_eq!(is_emoji_presentation('\u{70000}'), false);
        assert_eq!(is_emoji_presentation('\u{80000}'), false);
        assert_eq!(is_emoji_presentation('\u{90000}'), false);
        assert_eq!(is_emoji_presentation('\u{a0000}'), false);
        assert_eq!(is_emoji_presentation('\u{b0000}'), false);
        assert_eq!(is_emoji_presentation('\u{c0000}'), false);
        assert_eq!(is_emoji_presentation('\u{d0000}'), false);
        assert_eq!(is_emoji_presentation('\u{e0000}'), false);

        assert_eq!(is_emoji_presentation('\u{efffe}'), false);
        assert_eq!(is_emoji_presentation('\u{effff}'), false);

        // Priavte-Use Area
        assert_eq!(is_emoji_presentation('\u{f0000}'), false);
        assert_eq!(is_emoji_presentation('\u{f0001}'), false);
        assert_eq!(is_emoji_presentation('\u{ffffe}'), false);
        assert_eq!(is_emoji_presentation('\u{fffff}'), false);
        assert_eq!(is_emoji_presentation('\u{100000}'), false);
        assert_eq!(is_emoji_presentation('\u{100001}'), false);
        assert_eq!(is_emoji_presentation('\u{10fffe}'), false);
        assert_eq!(is_emoji_presentation('\u{10ffff}'), false);
    }
}

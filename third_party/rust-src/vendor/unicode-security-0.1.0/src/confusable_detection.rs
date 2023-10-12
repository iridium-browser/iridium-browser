//! [Confusable detection](https://www.unicode.org/reports/tr39/#Confusable_Detection)

use core::iter;

enum OnceOrMore<T, I> {
    Once(iter::Once<T>),
    More(I),
}

impl<T, I> Iterator for OnceOrMore<T, I>
where
    I: Iterator<Item = T>,
{
    type Item = T;

    fn next(&mut self) -> Option<T> {
        use OnceOrMore::*;
        match self {
            Once(v) => v.next(),
            More(i) => i.next(),
        }
    }
}

type StaticSliceIterCloned = core::iter::Cloned<core::slice::Iter<'static, char>>;

fn char_prototype(c: char) -> OnceOrMore<char, StaticSliceIterCloned> {
    use crate::tables::confusable_detection::char_confusable_prototype;
    match char_confusable_prototype(c) {
        None => OnceOrMore::Once(iter::once(c)),
        Some(l) => OnceOrMore::More(l.iter().cloned()),
    }
}

/// Calculate skeleton for string, as defined by UTS 39
pub fn skeleton(s: &str) -> impl Iterator<Item = char> + '_ {
    use unicode_normalization::UnicodeNormalization;
    s.chars().nfd().flat_map(char_prototype).nfd()
}

// Take a look at the license at the top of the repository in the LICENSE file.

use crate::json::json_minifier::JsonMinifier;

use std::fmt;
use std::str::Chars;

#[derive(Clone)]
pub struct JsonMultiFilter<'a, P: Clone> {
    minifier: JsonMinifier,
    iter: Chars<'a>,
    predicate: P,
    initialized: bool,
    item1: Option<<Chars<'a> as Iterator>::Item>,
}

impl<'a, P: Clone> JsonMultiFilter<'a, P> {
    #[inline]
    pub fn new(iter: Chars<'a>, predicate: P) -> Self {
        JsonMultiFilter {
            minifier: JsonMinifier::default(),
            iter,
            predicate,
            initialized: false,
            item1: None,
        }
    }
}

impl<'a, P: Clone> fmt::Debug for JsonMultiFilter<'a, P> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Filter")
            .field("minifier", &self.minifier)
            .field("iter", &self.iter)
            .field("initialized", &self.initialized)
            .finish()
    }
}

impl<'a, P: Clone> Iterator for JsonMultiFilter<'a, P>
where
    P: FnMut(
        &mut JsonMinifier,
        &<Chars<'a> as Iterator>::Item,
        Option<&<Chars<'a> as Iterator>::Item>,
    ) -> bool,
{
    type Item = <Chars<'a> as Iterator>::Item;

    #[inline]
    fn next(&mut self) -> Option<<Chars<'a> as Iterator>::Item> {
        if !self.initialized {
            self.item1 = self.iter.next();
            self.initialized = true;
        }

        while let Some(item) = self.item1.take() {
            self.item1 = self.iter.next();
            if (self.predicate)(&mut self.minifier, &item, self.item1.as_ref()) {
                return Some(item);
            }
        }
        None
    }
}

impl<'a, P> JsonMultiFilter<'a, P>
where
    P: FnMut(
            &mut JsonMinifier,
            &<Chars<'a> as Iterator>::Item,
            Option<&<Chars<'a> as Iterator>::Item>,
        ) -> bool
        + Clone,
{
    pub(super) fn write<W: std::io::Write>(self, mut w: W) -> std::io::Result<()> {
        for token in self {
            write!(w, "{}", token)?;
        }
        Ok(())
    }
}

impl<'a, P> fmt::Display for JsonMultiFilter<'a, P>
where
    P: FnMut(
            &mut JsonMinifier,
            &<Chars<'a> as Iterator>::Item,
            Option<&<Chars<'a> as Iterator>::Item>,
        ) -> bool
        + Clone,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = (*self).clone();
        for token in s {
            write!(f, "{}", token)?;
        }
        Ok(())
    }
}

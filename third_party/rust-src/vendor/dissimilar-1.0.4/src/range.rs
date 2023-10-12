use crate::find::find;
use std::fmt::Debug;
use std::ops::{self, RangeFrom, RangeFull, RangeTo};
use std::str::{CharIndices, Chars};

#[derive(Copy, Clone)]
pub struct Range<'a> {
    pub doc: &'a str,
    pub offset: usize,
    pub len: usize,
}

impl<'a> Range<'a> {
    pub fn empty() -> Self {
        Range {
            doc: "",
            offset: 0,
            len: 0,
        }
    }

    pub fn new(doc: &'a str, bounds: impl RangeBounds) -> Self {
        let (offset, len) = bounds.index(doc.len());
        Range { doc, offset, len }
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn substring(&self, bounds: impl RangeBounds) -> Self {
        let (offset, len) = bounds.index(self.len);
        Range {
            doc: self.doc,
            offset: self.offset + offset,
            len,
        }
    }

    pub fn get(&self, bounds: impl RangeBounds) -> Option<Self> {
        let (offset, len) = bounds.try_index(self.len)?;
        Some(Range {
            doc: self.doc,
            offset: self.offset + offset,
            len,
        })
    }

    pub fn split_at(&self, mid: usize) -> (Self, Self) {
        (self.substring(..mid), self.substring(mid..))
    }

    pub fn chars(&self) -> Chars<'a> {
        str(*self).chars()
    }

    pub fn char_indices(&self) -> CharIndices<'a> {
        str(*self).char_indices()
    }

    pub fn bytes(&self) -> impl Iterator<Item = u8> + DoubleEndedIterator + ExactSizeIterator + 'a {
        bytes(*self).iter().cloned()
    }

    pub fn starts_with(&self, prefix: impl AsRef<[u8]>) -> bool {
        bytes(*self).starts_with(prefix.as_ref())
    }

    pub fn ends_with(&self, suffix: impl AsRef<[u8]>) -> bool {
        bytes(*self).ends_with(suffix.as_ref())
    }

    pub fn find(&self, needle: impl AsRef<[u8]>) -> Option<usize> {
        find(bytes(*self), needle.as_ref())
    }
}

pub fn str(range: Range) -> &str {
    if cfg!(debug)
        && range
            .doc
            .get(range.offset..range.offset + range.len)
            .is_none()
    {
        eprintln!(
            "doc={:?} offset={} len={}",
            range.doc, range.offset, range.len
        );
    }
    &range.doc[range.offset..range.offset + range.len]
}

pub fn bytes(range: Range) -> &[u8] {
    &range.doc.as_bytes()[range.offset..range.offset + range.len]
}

impl AsRef<[u8]> for Range<'_> {
    fn as_ref(&self) -> &[u8] {
        bytes(*self)
    }
}

pub trait RangeBounds: Sized + Clone + Debug {
    // Returns (offset, len).
    fn try_index(self, len: usize) -> Option<(usize, usize)>;
    fn index(self, len: usize) -> (usize, usize) {
        match self.clone().try_index(len) {
            Some(range) => range,
            None => panic!("index out of range, index={:?}, len={}", self, len),
        }
    }
}

impl RangeBounds for ops::Range<usize> {
    fn try_index(self, len: usize) -> Option<(usize, usize)> {
        if self.start <= self.end && self.end <= len {
            Some((self.start, self.end - self.start))
        } else {
            None
        }
    }
}

impl RangeBounds for RangeFrom<usize> {
    fn try_index(self, len: usize) -> Option<(usize, usize)> {
        if self.start <= len {
            Some((self.start, len - self.start))
        } else {
            None
        }
    }
}

impl RangeBounds for RangeTo<usize> {
    fn try_index(self, len: usize) -> Option<(usize, usize)> {
        if self.end <= len {
            Some((0, self.end))
        } else {
            None
        }
    }
}

impl RangeBounds for RangeFull {
    fn try_index(self, len: usize) -> Option<(usize, usize)> {
        Some((0, len))
    }
}

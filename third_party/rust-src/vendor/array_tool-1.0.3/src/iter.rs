use std::cmp;
use std::iter::IntoIterator;

#[doc(hidden)]
#[derive(Clone, Debug)]
#[must_use = "iterator adaptors are lazy and do nothing unless consumed"]
pub struct ZipOption<A, B> {
  a: A,
  b: B,
  // index and len are only used by the specialized version of zip
  index: usize,
  len: usize,
}

/// Zips to iterators together to the longest length
/// via Option<(Option<A>, Option<B>)>
pub trait ZipOpt {
  /// Zip to iterators to longest length via Option<(Option<A>, Option<B>)> results.
  /// # Example
  /// ```
  /// use array_tool::iter::ZipOpt;
  ///
  /// let a = vec!["a","b","c", "d"];
  /// let b = vec!["c","d"];
  /// let mut x = a.iter().zip_option(b.iter());
  ///
  /// assert_eq!(x.next(), Some((Some(&"a"), Some(&"c"))));
  /// assert_eq!(x.next(), Some((Some(&"b"), Some(&"d"))));
  /// assert_eq!(x.next(), Some((Some(&"c"), None)));
  /// assert_eq!(x.next(), Some((Some(&"d"), None)));
  /// assert_eq!(x.next(), None);
  /// ```
  ///
  /// # Output
  /// ```text
  /// vec![ "a", "b", "c", "d" ]
  /// ```
  fn zip_option<U>(self, other: U) -> ZipOption<Self, U::IntoIter>
    where Self: Sized, U: IntoIterator;
}

impl<I: Iterator> ZipOpt for I {
  #[inline]
  fn zip_option<U>(self, other: U) -> ZipOption<Self, U::IntoIter>
    where Self: Sized, U: IntoIterator {

    ZipOption::new(self, other.into_iter())
  }
}

impl<A, B> Iterator for ZipOption<A, B> where A: Iterator, B: Iterator {
  type Item = (Option<A::Item>, Option<B::Item>);

  #[inline]
  fn next(&mut self) -> Option<Self::Item> {
    ZipImpl::next(self)
  }

  #[inline]
  fn size_hint(&self) -> (usize, Option<usize>) {
    ZipImpl::size_hint(self)
  }

  #[inline]
  fn nth(&mut self, n: usize) -> Option<Self::Item> {
    ZipImpl::nth(self, n)
  }
}

#[doc(hidden)]
impl<A, B> DoubleEndedIterator for ZipOption<A, B> where
A: DoubleEndedIterator + ExactSizeIterator,
B: DoubleEndedIterator + ExactSizeIterator,
{
  #[inline]
  fn next_back(&mut self) -> Option<(Option<A::Item>, Option<B::Item>)> {
    ZipImpl::next_back(self)
  }
}

#[doc(hidden)]
trait ZipImpl<A, B> {
  type Item;
  fn new(a: A, b: B) -> Self;
  fn next(&mut self) -> Option<Self::Item>;
  fn size_hint(&self) -> (usize, Option<usize>);
  fn nth(&mut self, n: usize) -> Option<Self::Item>;
  fn super_nth(&mut self, mut n: usize) -> Option<Self::Item> {
    while let Some(x) = self.next() {
      if n == 0 { return Some(x) }
      n -= 1;
    }
    None
  }
  fn next_back(&mut self) -> Option<Self::Item>
    where A: DoubleEndedIterator + ExactSizeIterator,
          B: DoubleEndedIterator + ExactSizeIterator;
}

#[doc(hidden)]
impl<A, B> ZipImpl<A, B> for ZipOption<A, B>
  where A: Iterator, B: Iterator {
  type Item = (Option<A::Item>, Option<B::Item>);
  fn new(a: A, b: B) -> Self {
    ZipOption {
      a,
      b,
      index: 0, // unused
      len: 0, // unused
    }
  }

  #[inline]
  fn next(&mut self) -> Option<(Option<A::Item>, Option<B::Item>)> {
    let first = self.a.next();
    let second = self.b.next();

    if first.is_some() || second.is_some() {
      Some((first, second))
    } else {
      None
    }
  }

  #[inline]
  fn nth(&mut self, n: usize) -> Option<Self::Item> {
    self.super_nth(n)
  }

  #[inline]
  fn next_back(&mut self) -> Option<(Option<A::Item>, Option<B::Item>)>
    where A: DoubleEndedIterator + ExactSizeIterator,
          B: DoubleEndedIterator + ExactSizeIterator {
    let a_sz = self.a.len();
    let b_sz = self.b.len();
    if a_sz != b_sz {
      // Adjust a, b to equal length
      if a_sz > b_sz {
        for _ in 0..a_sz - b_sz { self.a.next_back(); }
      } else {
        for _ in 0..b_sz - a_sz { self.b.next_back(); }
      }
    }
    match (self.a.next_back(), self.b.next_back()) {
      (None, None) => None,
      (f,s) => Some((f, s)),
    }
  }

  #[inline]
  fn size_hint(&self) -> (usize, Option<usize>) {
    let (a_lower, a_upper) = self.a.size_hint();
    let (b_lower, b_upper) = self.b.size_hint();

    let lower = cmp::min(a_lower, b_lower);

    let upper = match (a_upper, b_upper) {
      (Some(x), Some(y)) => Some(cmp::max(x,y)),
      (Some(x), None) => Some(x),
      (None, Some(y)) => Some(y),
      (None, None) => None
    };

    (lower, upper)
  }
}

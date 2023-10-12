// Copyright 2015-2017 Daniel P. Clark & array_tool Developers
// 
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.

/// A grapheme iterator that produces the bytes for each grapheme.
#[derive(Debug)]
pub struct GraphemeBytesIter<'a> {
  source: &'a str,
  offset: usize,
  grapheme_count: usize,
}
impl<'a> GraphemeBytesIter<'a> {
  /// Creates a new grapheme iterator from a string source.
  pub fn new(source: &'a str) -> GraphemeBytesIter<'a> {
    GraphemeBytesIter {
      source: source,
      offset: 0,
      grapheme_count: 0,
    }
  }
}
impl<'a> Iterator for GraphemeBytesIter<'a> {
  type Item = &'a [u8];

  fn next(&mut self) -> Option<&'a [u8]> {
    let mut result: Option<&[u8]> = None;
    let mut idx = self.offset;
    for _ in self.offset..self.source.len() {
      idx += 1;
      if self.offset < self.source.len() {
        if self.source.is_char_boundary(idx) {
          let slice: &[u8] = self.source[self.offset..idx].as_bytes();

          self.grapheme_count += 1;
          self.offset = idx;

          result = Some(slice);
          break
        }
      }
    }
    result
  }
}
impl<'a> ExactSizeIterator for GraphemeBytesIter<'a> {
  fn len(&self) -> usize {
    self.source.chars().count()
  }
}
/// ToGraphemeBytesIter - create an iterator to return bytes for each grapheme in a string.
pub trait ToGraphemeBytesIter<'a> {
  /// Returns a GraphemeBytesIter which you may iterate over.
  ///
  /// # Example
  /// ```
  /// use array_tool::string::ToGraphemeBytesIter;
  ///
  /// let string = "a s—d féZ";
  /// let mut graphemes = string.grapheme_bytes_iter();
  /// graphemes.skip(3).next();
  /// ```
  ///
  /// # Output
  /// ```text
  /// [226, 128, 148]
  /// ```
  fn grapheme_bytes_iter(&'a self) -> GraphemeBytesIter<'a>;
}
impl<'a> ToGraphemeBytesIter<'a> for str {
  fn grapheme_bytes_iter(&'a self) -> GraphemeBytesIter<'a> {
    GraphemeBytesIter::new(&self)
  }
}

/// Squeeze - squeezes duplicate characters down to one each
pub trait Squeeze {
  /// # Example
  /// ```
  /// use array_tool::string::Squeeze;
  ///
  /// "yellow moon".squeeze("");
  /// ```
  ///
  /// # Output
  /// ```text
  /// "yelow mon"
  /// ```
  fn squeeze(&self, targets: &'static str) -> String;
}
impl Squeeze for str {
  fn squeeze(&self, targets: &'static str) -> String {
    let mut output = Vec::<u8>::with_capacity(self.len());
    let everything: bool = targets.is_empty();
    let chars = targets.grapheme_bytes_iter().collect::<Vec<&[u8]>>();
    let mut last: &[u8] = &[0];
    for character in self.grapheme_bytes_iter() {
      if last != character {
        output.extend_from_slice(character);
      } else if !(everything || chars.contains(&character)) {
        output.extend_from_slice(character);
      }
      last = character;
    }
    String::from_utf8(output).expect("squeeze failed to render String!")
  }
}

/// Justify - expand line to given width.
pub trait Justify {
  /// # Example
  /// ```
  /// use array_tool::string::Justify;
  ///
  /// "asd asdf asd".justify_line(14);
  /// ```
  ///
  /// # Output
  /// ```text
  /// "asd  asdf  asd"
  /// ```
  fn justify_line(&self, width: usize) -> String;
}

impl Justify for str {
  fn justify_line(&self, width: usize) -> String {
    if self.is_empty() { return format!("{}", self) };
    let trimmed = self.trim() ;
    let len = trimmed.chars().count();
    if len >= width { return self.to_string(); };
    let difference = width - len;
    let iter = trimmed.split_whitespace();
    let spaces = iter.count() - 1;
    let mut iter = trimmed.split_whitespace().peekable();
    if spaces == 0 { return self.to_string(); }
    let mut obj = String::with_capacity(trimmed.len() + spaces);

    let div = difference / spaces;
    let mut remainder = difference % spaces;

    while let Some(x) = iter.next() {
      obj.push_str( x );
      let val = if remainder > 0 {
        remainder = remainder - 1;
        div + 1
      } else { div };
      for _ in 0..val+1 {
        if let Some(_) = iter.peek() { // Don't add spaces if last word
          obj.push_str( " " );
        }
      }
    }
    obj
  }
}

/// Substitute string character for each index given.
pub trait SubstMarks {
  /// # Example
  /// ```
  /// use array_tool::string::SubstMarks;
  ///
  /// "asdf asdf asdf".subst_marks(vec![0,5,8], "Z");
  /// ```
  ///
  /// # Output
  /// ```text
  /// "Zsdf ZsdZ asdf"
  /// ```
  fn subst_marks(&self, marks: Vec<usize>, chr: &'static str) -> String;
}
impl SubstMarks for str {
  fn subst_marks(&self, marks: Vec<usize>, chr: &'static str) -> String {
    let mut output = Vec::<u8>::with_capacity(self.len());
    let mut count = 0;
    let mut last = 0;
    for i in 0..self.len() {
      let idx = i + 1;
      if self.is_char_boundary(idx) {
        if marks.contains(&count) {
          count += 1;
          last = idx;
          output.extend_from_slice(chr.as_bytes());
          continue
        }

        let slice: &[u8] = self[last..idx].as_bytes();
        output.extend_from_slice(slice);

        count += 1;
        last = idx
      }
    }
    String::from_utf8(output).expect("subst_marks failed to render String!")
  }
}

/// After whitespace
pub trait AfterWhitespace {
  /// Given offset method will seek from there to end of string to find the first
  /// non white space.  Resulting value is counted from offset.
  ///
  /// # Example
  /// ```
  /// use array_tool::string::AfterWhitespace;
  ///
  /// assert_eq!(
  ///   "asdf           asdf asdf".seek_end_of_whitespace(6),
  ///   Some(9)
  /// );
  /// ```
  fn seek_end_of_whitespace(&self, offset: usize) -> Option<usize>;
}
impl AfterWhitespace for str {
  fn seek_end_of_whitespace(&self, offset: usize) -> Option<usize> {
    if self.len() < offset { return None; };
    let mut seeker = self[offset..self.len()].chars();
    let mut val = None;
    let mut indx = 0;
    while let Some(x) = seeker.next() {
      if x.ne(&" ".chars().next().unwrap()) {
        val = Some(indx);
        break;
      }
      indx += 1;
    }
    val
  }
}

/// Word wrapping
pub trait WordWrap {
  ///  White space is treated as valid content and new lines will only be swapped in for
  ///  the last white space character at the end of the given width.  White space may reach beyond
  ///  the width you've provided.  You will need to trim end of lines in your own output (e.g.
  ///  splitting string at each new line and printing the line with trim_right).  Or just trust
  ///  that lines that are beyond the width are just white space and only print the width -
  ///  ignoring tailing white space.
  ///
  /// # Example
  /// ```
  /// use array_tool::string::WordWrap;
  ///
  /// "asd asdf asd".word_wrap(8);
  /// ```
  ///
  /// # Output
  /// ```text
  /// "asd asdf\nasd"
  /// ```
  fn word_wrap(&self, width: usize) -> String;
}
// No need to worry about character encoding since we're only checking for the
// space and new line characters.
impl WordWrap for &'static str {
  fn word_wrap(&self, width: usize) -> String {
    let mut markers = vec![];
    fn wordwrap(t: &'static str, chunk: usize, offset: usize, mrkrs: &mut Vec<usize>) -> String {
      match t[offset..*vec![offset+chunk,t.len()].iter().min().unwrap()].rfind("\n") {
        None => {
          match t[offset..*vec![offset+chunk,t.len()].iter().min().unwrap()].rfind(" ") {
            Some(x) => {
              let mut eows = x; // end of white space
              if offset+chunk < t.len() { // check if white space continues
                match t.seek_end_of_whitespace(offset+x) {
                  Some(a) => {
                    if a.ne(&0) {
                      eows = x+a-1;
                    }
                  },
                  None => {},
                }
              }
              if offset+chunk < t.len() { // safe to seek ahead by 1 or not end of string
                if !["\n".chars().next().unwrap(), " ".chars().next().unwrap()].contains(
                  &t[offset+eows+1..offset+eows+2].chars().next().unwrap()
                  ) {
                  mrkrs.push(offset+eows)
                }
              };
              wordwrap(t, chunk, offset+eows+1, mrkrs)
            },
            None => { 
              if offset+chunk < t.len() { // String may continue
                wordwrap(t, chunk, offset+1, mrkrs) // Recurse + 1 until next space
              } else {
                use string::SubstMarks;

                return t.subst_marks(mrkrs.to_vec(), "\n")
              }
            },
          }
        },
        Some(x) => {
          wordwrap(t, chunk, offset+x+1, mrkrs)
        },
      }
    };
    wordwrap(self, width+1, 0, &mut markers)
  }
}

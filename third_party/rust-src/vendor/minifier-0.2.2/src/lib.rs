// Take a look at the license at the top of the repository in the LICENSE file.

#[cfg(feature = "html")]
extern crate regex;

pub mod css;
#[cfg(feature = "html")]
pub mod html;
pub mod js;
pub mod json;

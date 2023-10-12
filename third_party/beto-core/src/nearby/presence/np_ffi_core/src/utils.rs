// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//! Common utilities used by other modules within this crate.

use handle_map::HandleLike;

/// Type-level predicate for handle types which uniformly hold a lock
/// for longer than some other handle type in API calls.
pub(crate) trait LocksLongerThan<H: HandleLike>: HandleLike {}

/// Trait which canonicalizes the relationship between FFI
/// tagged-unions and their tags
pub trait FfiEnum {
    /// The [FFI-safe] type of tags used to identify
    /// variants of this tagged union.
    type Kind;

    /// Returns the tag for this FFI-safe tagged union.
    fn kind(&self) -> Self::Kind;
}

/// Declares a method of the given name which attempts to cast
/// the enclosing enum to the payload held under the given
/// variant name, yielding a result of the given type wrapped
/// in an `Option::Some`.
///
/// If the enclosing enum turns out to not be the requested
/// variant, the generated method will return `None`.
macro_rules! declare_enum_cast {
    ($projection_method_name:ident, $variant_enum_name:ident, $variant_type_name:ty) => {
        #[doc = concat!("Attempts to cast `self` to the `", stringify!($variant_enum_name),
          "` variant, returning `None` in the \ncase where the passed value is of a different enum variant.")]
        pub fn $projection_method_name(self) -> Option<$variant_type_name> {
            match self {
                Self::$variant_enum_name(x) => Some(x),
                _ => None,
            }
        }
    }
}

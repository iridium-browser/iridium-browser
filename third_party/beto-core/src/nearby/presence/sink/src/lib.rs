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

//! A no_std-friendly data-writing "sink" trait which allows for convenient expression
//! of "write me into a limited-size buffer"-type methods on traits.
#![cfg_attr(not(feature = "std"), no_std)]
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]

/// An append-only, limited-size collection.
pub trait Sink<T> {
    /// Returns `Some` if the slice was appended, `None` otherwise, in which case nothing was written
    fn try_extend_from_slice(&mut self, items: &[T]) -> Option<()>;

    /// Returns `Some` if the item could be pushed, `None` otherwise
    fn try_push(&mut self, item: T) -> Option<()>;

    /// Uses the given [`SinkWriter`] to write to this sink.
    fn try_extend_from_writer<W: SinkWriter<DataType = T>>(&mut self, writer: W) -> Option<()>
    where
        Self: Sized,
    {
        writer.write_payload(self)
    }
}

/// A use-once trait (like `FnOnce`) which represents some
/// code which writes data to a [`Sink`].
pub trait SinkWriter {
    /// The type of data being written to the [`Sink`].
    type DataType;

    /// Returns `Some` if all data was successfully written to the [`Sink`],
    /// but if doing so failed at any point, returns `None`. If this method
    /// fails, the contents of the [`Sink`] should be considered to be invalid.
    fn write_payload<S: Sink<Self::DataType> + ?Sized>(self, sink: &mut S) -> Option<()>;
}

impl<T, A> Sink<T> for tinyvec::ArrayVec<A>
where
    A: tinyvec::Array<Item = T>,
    T: Clone,
{
    fn try_extend_from_slice(&mut self, items: &[T]) -> Option<()> {
        if items.len() > (self.capacity() - self.len()) {
            return None;
        }
        // won't panic: just checked the length
        self.extend_from_slice(items);
        Some(())
    }

    fn try_push(&mut self, item: T) -> Option<()> {
        // tinyvec uses None to indicate success, whereas for our limited purposes we want the
        // opposite
        match tinyvec::ArrayVec::try_push(self, item) {
            None => Some(()),
            Some(_) => None,
        }
    }
}

#[cfg(feature = "std")]
impl<T: Clone> Sink<T> for std::vec::Vec<T> {
    fn try_extend_from_slice(&mut self, items: &[T]) -> Option<()> {
        self.extend_from_slice(items);
        Some(())
    }

    fn try_push(&mut self, item: T) -> Option<()> {
        self.push(item);
        Some(())
    }
}

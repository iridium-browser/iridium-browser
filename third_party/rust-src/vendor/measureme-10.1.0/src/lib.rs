//! This crate provides a library for high-performance event tracing which is used by
//! the Rust compiler's unstable `-Z self-profile` feature.
//!
//! The output of a tracing session will be an `.mm_profdata` file containing a stream
//! of events and a string table that allows to decode the `StringId`s in the event stream.
//!
//! # Writing event trace files
//!
//! The main entry point for writing event trace files is the [`Profiler`] struct.
//!
//! To create a [`Profiler`], call the [`Profiler::new()`] function and provide a `Path` with
//! the directory and file name for the trace files.
//! Alternatively, call the [`Profiler::with_counter()`] function, to choose the [`Counter`]
//! the profiler will use for events (whereas [`Profiler::new()`] defaults to `wall-time`).
//!
//! For more information on available counters, see the [`counters`] module documentation.
//!
//! To record an event, call the [`Profiler::record_instant_event()`] method, passing a few
//! arguments:
//!   - `event_kind`: a [`StringId`] which assigns an arbitrary category to the event
//!   - `event_id`: a [`StringId`] which specifies the name of the event
//!   - `thread_id`: a `u32` id of the thread which is recording this event
//!
//! Alternatively, events can also be recorded via the
//! [`Profiler::start_recording_interval_event()`] method. This method records a "start" event and
//! returns a `TimingGuard` object that will automatically record the corresponding "end" event
//! when it is dropped.
//!
//! To create a [`StringId`], call one of the string allocation methods:
//!   - [`Profiler::alloc_string()`]: allocates a string and returns the [`StringId`] that refers
//!     to it
//!
//! [`Counter`]: counters::Counter
#![deny(warnings)]

#[macro_use]
extern crate log;

pub mod counters;
pub mod event_id;
pub mod file_header;
mod profiler;
mod raw_event;
mod serialization;
pub mod stringtable;

pub mod rustc;

pub use crate::event_id::{EventId, EventIdBuilder};
pub use crate::profiler::{DetachedTiming, Profiler, TimingGuard};
pub use crate::raw_event::{RawEvent, MAX_INTERVAL_VALUE, MAX_SINGLE_VALUE};
pub use crate::serialization::{
    split_streams, Addr, PageTag, SerializationSink, SerializationSinkBuilder,
};
pub use crate::stringtable::{SerializableString, StringComponent, StringId, StringTableBuilder};

use crate::counters::Counter;
use crate::file_header::{write_file_header, FILE_MAGIC_EVENT_STREAM, FILE_MAGIC_TOP_LEVEL};
use crate::raw_event::RawEvent;
use crate::serialization::{PageTag, SerializationSink, SerializationSinkBuilder};
use crate::stringtable::{SerializableString, StringId, StringTableBuilder};
use crate::{event_id::EventId, file_header::FILE_EXTENSION};
use std::error::Error;
use std::fs;
use std::path::Path;
use std::sync::Arc;

pub struct Profiler {
    event_sink: Arc<SerializationSink>,
    string_table: StringTableBuilder,
    counter: Counter,
}

impl Profiler {
    pub fn new<P: AsRef<Path>>(path_stem: P) -> Result<Profiler, Box<dyn Error + Send + Sync>> {
        Self::with_counter(
            path_stem,
            Counter::WallTime(crate::counters::WallTime::new()),
        )
    }

    pub fn with_counter<P: AsRef<Path>>(
        path_stem: P,
        counter: Counter,
    ) -> Result<Profiler, Box<dyn Error + Send + Sync>> {
        let path = path_stem.as_ref().with_extension(FILE_EXTENSION);

        fs::create_dir_all(path.parent().unwrap())?;
        let mut file = fs::File::create(path)?;

        // The first thing in the file must be the top-level file header.
        write_file_header(&mut file, FILE_MAGIC_TOP_LEVEL)?;

        let sink_builder = SerializationSinkBuilder::new_from_file(file)?;
        let event_sink = Arc::new(sink_builder.new_sink(PageTag::Events));

        // The first thing in every stream we generate must be the stream header.
        write_file_header(&mut event_sink.as_std_write(), FILE_MAGIC_EVENT_STREAM)?;

        let string_table = StringTableBuilder::new(
            Arc::new(sink_builder.new_sink(PageTag::StringData)),
            Arc::new(sink_builder.new_sink(PageTag::StringIndex)),
        )?;

        let profiler = Profiler {
            event_sink,
            string_table,
            counter,
        };

        let mut args = String::new();
        for arg in std::env::args() {
            args.push_str(&arg.escape_default().to_string());
            args.push(' ');
        }

        profiler.string_table.alloc_metadata(&*format!(
            r#"{{ "start_time": {}, "process_id": {}, "cmd": "{}", "counter": {} }}"#,
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos(),
            std::process::id(),
            args,
            profiler.counter.describe_as_json(),
        ));

        Ok(profiler)
    }

    #[inline(always)]
    pub fn map_virtual_to_concrete_string(&self, virtual_id: StringId, concrete_id: StringId) {
        self.string_table
            .map_virtual_to_concrete_string(virtual_id, concrete_id);
    }

    #[inline(always)]
    pub fn bulk_map_virtual_to_single_concrete_string<I>(
        &self,
        virtual_ids: I,
        concrete_id: StringId,
    ) where
        I: Iterator<Item = StringId> + ExactSizeIterator,
    {
        self.string_table
            .bulk_map_virtual_to_single_concrete_string(virtual_ids, concrete_id);
    }

    #[inline(always)]
    pub fn alloc_string<STR: SerializableString + ?Sized>(&self, s: &STR) -> StringId {
        self.string_table.alloc(s)
    }

    /// Records an event with the given parameters. The event time is computed
    /// automatically.
    pub fn record_instant_event(&self, event_kind: StringId, event_id: EventId, thread_id: u32) {
        let raw_event =
            RawEvent::new_instant(event_kind, event_id, thread_id, self.counter.since_start());

        self.record_raw_event(&raw_event);
    }

    /// Records an event with the given parameters. The event time is computed
    /// automatically.
    pub fn record_integer_event(
        &self,
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
        value: u64,
    ) {
        let raw_event = RawEvent::new_integer(event_kind, event_id, thread_id, value);
        self.record_raw_event(&raw_event);
    }

    /// Creates a "start" event and returns a `TimingGuard` that will create
    /// the corresponding "end" event when it is dropped.
    #[inline]
    pub fn start_recording_interval_event<'a>(
        &'a self,
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
    ) -> TimingGuard<'a> {
        TimingGuard {
            profiler: self,
            event_id,
            event_kind,
            thread_id,
            start_count: self.counter.since_start(),
        }
    }

    /// Creates a "start" event and returns a `DetachedTiming`.
    /// To create the corresponding "event" event, you must call
    /// `finish_recording_internal_event` with the returned
    /// `DetachedTiming`.
    /// Since `DetachedTiming` does not capture the lifetime of `&self`,
    /// this method can sometimes be more convenient than
    /// `start_recording_interval_event` - e.g. it can be stored
    /// in a struct without the need to add a lifetime parameter.
    #[inline]
    pub fn start_recording_interval_event_detached(
        &self,
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
    ) -> DetachedTiming {
        DetachedTiming {
            event_id,
            event_kind,
            thread_id,
            start_count: self.counter.since_start(),
        }
    }

    /// Creates the corresponding "end" event for
    /// the "start" event represented by `timing`. You
    /// must have obtained `timing` from the same `Profiler`
    pub fn finish_recording_interval_event(&self, timing: DetachedTiming) {
        drop(TimingGuard {
            profiler: self,
            event_id: timing.event_id,
            event_kind: timing.event_kind,
            thread_id: timing.thread_id,
            start_count: timing.start_count,
        });
    }

    fn record_raw_event(&self, raw_event: &RawEvent) {
        self.event_sink
            .write_atomic(std::mem::size_of::<RawEvent>(), |bytes| {
                raw_event.serialize(bytes);
            });
    }
}

/// Created by `Profiler::start_recording_interval_event_detached`.
/// Must be passed to `finish_recording_interval_event` to record an
/// "end" event.
#[must_use]
pub struct DetachedTiming {
    event_id: EventId,
    event_kind: StringId,
    thread_id: u32,
    start_count: u64,
}

/// When dropped, this `TimingGuard` will record an "end" event in the
/// `Profiler` it was created by.
#[must_use]
pub struct TimingGuard<'a> {
    profiler: &'a Profiler,
    event_id: EventId,
    event_kind: StringId,
    thread_id: u32,
    start_count: u64,
}

impl<'a> Drop for TimingGuard<'a> {
    #[inline]
    fn drop(&mut self) {
        let raw_event = RawEvent::new_interval(
            self.event_kind,
            self.event_id,
            self.thread_id,
            self.start_count,
            self.profiler.counter.since_start(),
        );

        self.profiler.record_raw_event(&raw_event);
    }
}

impl<'a> TimingGuard<'a> {
    /// This method set a new `event_id` right before actually recording the
    /// event.
    #[inline]
    pub fn finish_with_override_event_id(mut self, event_id: EventId) {
        self.event_id = event_id;
        // Let's be explicit about it: Dropping the guard will record the event.
        drop(self)
    }
}

// Make sure that `Profiler` can be used in a multithreaded context
fn _assert_bounds() {
    assert_bounds_inner(&Profiler::new(""));
    fn assert_bounds_inner<S: Sized + Send + Sync + 'static>(_: &S) {}
}

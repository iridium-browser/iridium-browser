pub(crate) mod format;

use ansi_term::{Color, Style};
use format::{Buffers, ColorLevel, Config, FmtEvent, SpanMode};
use std::{
    fmt::{self, Write as _},
    io,
    sync::Mutex,
    time::Instant,
};
use tracing_core::{
    field::{Field, Visit},
    span::{Attributes, Id},
    Event, Subscriber,
};
#[cfg(feature = "tracing-log")]
use tracing_log::NormalizeEvent;
use tracing_subscriber::{
    fmt::MakeWriter,
    layer::{Context, Layer},
    registry::{self, LookupSpan},
};

pub(crate) struct Data {
    start: Instant,
    kvs: Vec<(&'static str, String)>,
}

impl Data {
    pub fn new(attrs: &Attributes<'_>) -> Self {
        let mut span = Self {
            start: Instant::now(),
            kvs: Vec::new(),
        };
        attrs.record(&mut span);
        span
    }
}

impl Visit for Data {
    fn record_debug(&mut self, field: &Field, value: &dyn fmt::Debug) {
        self.kvs.push((field.name(), format!("{:?}", value)))
    }
}

#[derive(Debug)]
pub struct HierarchicalLayer<W = fn() -> io::Stderr>
where
    W: for<'writer> MakeWriter<'writer> + 'static,
{
    make_writer: W,
    bufs: Mutex<Buffers>,
    config: Config,
}

impl Default for HierarchicalLayer {
    fn default() -> Self {
        Self::new(2)
    }
}

impl HierarchicalLayer<fn() -> io::Stderr> {
    pub fn new(indent_amount: usize) -> Self {
        let ansi = atty::is(atty::Stream::Stderr);
        let config = Config {
            ansi,
            indent_amount,
            ..Default::default()
        };
        Self {
            make_writer: io::stderr,
            bufs: Mutex::new(Buffers::new()),
            config,
        }
    }
}

impl<W> HierarchicalLayer<W>
where
    W: for<'writer> MakeWriter<'writer> + 'static,
{
    /// Enables terminal colors, boldness and italics.
    pub fn with_ansi(self, ansi: bool) -> Self {
        Self {
            config: self.config.with_ansi(ansi),
            ..self
        }
    }

    pub fn with_writer<W2>(self, make_writer: W2) -> HierarchicalLayer<W2>
    where
        W2: for<'writer> MakeWriter<'writer>,
    {
        HierarchicalLayer {
            make_writer,
            config: self.config,
            bufs: self.bufs,
        }
    }

    pub fn with_indent_amount(self, indent_amount: usize) -> Self {
        let config = Config {
            indent_amount,
            ..self.config
        };
        Self { config, ..self }
    }

    /// Renders an ascii art tree instead of just using whitespace indentation.
    pub fn with_indent_lines(self, indent_lines: bool) -> Self {
        Self {
            config: self.config.with_indent_lines(indent_lines),
            ..self
        }
    }

    /// Whether to render the event and span targets. Usually targets are the module path to the
    /// event/span macro invocation.
    pub fn with_targets(self, targets: bool) -> Self {
        Self {
            config: self.config.with_targets(targets),
            ..self
        }
    }

    /// Whether to render the thread id in the beginning of every line. This is helpful to
    /// untangle the tracing statements emitted by each thread.
    pub fn with_thread_ids(self, thread_ids: bool) -> Self {
        Self {
            config: self.config.with_thread_ids(thread_ids),
            ..self
        }
    }

    /// Whether to render the thread name in the beginning of every line. Not all threads have
    /// names, but if they do, this may be more helpful than the generic thread ids.
    pub fn with_thread_names(self, thread_names: bool) -> Self {
        Self {
            config: self.config.with_thread_names(thread_names),
            ..self
        }
    }

    /// Resets the indentation to zero after `wraparound` indentation levels.
    /// This is helpful if you expect very deeply nested spans as otherwise the indentation
    /// just runs out of your screen.
    pub fn with_wraparound(self, wraparound: usize) -> Self {
        Self {
            config: self.config.with_wraparound(wraparound),
            ..self
        }
    }

    /// Whether to print the currently active span's message again before entering a new span.
    /// This helps if the entry to the current span was quite a while back (and with scrolling
    /// upwards in logs).
    pub fn with_verbose_entry(self, verbose_entry: bool) -> Self {
        Self {
            config: self.config.with_verbose_entry(verbose_entry),
            ..self
        }
    }

    /// Whether to print the currently active span's message again before dropping it.
    /// This helps if the entry to the current span was quite a while back (and with scrolling
    /// upwards in logs).
    pub fn with_verbose_exit(self, verbose_exit: bool) -> Self {
        Self {
            config: self.config.with_verbose_exit(verbose_exit),
            ..self
        }
    }

    /// Whether to print `{}` around the fields when printing a span.
    /// This can help visually distinguish fields from the rest of the message.
    pub fn with_bracketed_fields(self, bracketed_fields: bool) -> Self {
        Self {
            config: self.config.with_bracketed_fields(bracketed_fields),
            ..self
        }
    }

    fn styled(&self, style: Style, text: impl AsRef<str>) -> String {
        if self.config.ansi {
            style.paint(text.as_ref()).to_string()
        } else {
            text.as_ref().to_string()
        }
    }

    fn print_kvs<'a, I, V>(&self, buf: &mut impl fmt::Write, kvs: I) -> fmt::Result
    where
        I: IntoIterator<Item = (&'a str, V)>,
        V: fmt::Display + 'a,
    {
        let mut kvs = kvs.into_iter();
        if let Some((k, v)) = kvs.next() {
            if k == "message" {
                write!(buf, "{}", v)?;
            } else {
                write!(buf, "{}={}", k, v)?;
            }
        }
        for (k, v) in kvs {
            write!(buf, ", {}={}", k, v)?;
        }
        Ok(())
    }

    fn write_span_info<S>(&self, id: &Id, ctx: &Context<S>, style: SpanMode)
    where
        S: Subscriber + for<'span> LookupSpan<'span> + fmt::Debug,
    {
        let span = ctx
            .span(id)
            .expect("in on_enter/on_exit but span does not exist");
        let ext = span.extensions();
        let data = ext.get::<Data>().expect("span does not have data");

        let mut guard = self.bufs.lock().unwrap();
        let bufs = &mut *guard;
        let mut current_buf = &mut bufs.current_buf;

        let indent = ctx
            .lookup_current()
            .as_ref()
            .map(registry::SpanRef::scope)
            .map(registry::Scope::from_root)
            .into_iter()
            .flatten()
            .count();

        if self.config.verbose_entry || matches!(style, SpanMode::Open { .. } | SpanMode::Event) {
            if self.config.targets {
                let target = span.metadata().target();
                write!(
                    &mut current_buf,
                    "{}::",
                    self.styled(Style::new().dimmed(), target,),
                )
                .expect("Unable to write to buffer");
            }

            write!(
                current_buf,
                "{name}",
                name = self.styled(Style::new().fg(Color::Green).bold(), span.metadata().name())
            )
            .unwrap();
            if self.config.bracketed_fields {
                write!(
                    current_buf,
                    "{}",
                    self.styled(Style::new().fg(Color::Green).bold(), "{") // Style::new().fg(Color::Green).dimmed().paint("{")
                )
                .unwrap();
            } else {
                write!(current_buf, " ").unwrap();
            }
            self.print_kvs(&mut current_buf, data.kvs.iter().map(|(k, v)| (*k, v)))
                .unwrap();
            if self.config.bracketed_fields {
                write!(
                    current_buf,
                    "{}",
                    self.styled(Style::new().fg(Color::Green).bold(), "}") // Style::new().dimmed().paint("}")
                )
                .unwrap();
            }
        }

        bufs.indent_current(indent, &self.config, style);
        let writer = self.make_writer.make_writer();
        bufs.flush_current_buf(writer)
    }
}

impl<S, W> Layer<S> for HierarchicalLayer<W>
where
    S: Subscriber + for<'span> LookupSpan<'span> + fmt::Debug,
    W: for<'writer> MakeWriter<'writer> + 'static,
{
    fn on_new_span(&self, attrs: &Attributes, id: &Id, ctx: Context<S>) {
        let data = Data::new(attrs);
        let span = ctx.span(id).expect("in new_span but span does not exist");
        span.extensions_mut().insert(data);
        if self.config.verbose_exit {
            if let Some(span) = span.parent() {
                self.write_span_info(&span.id(), &ctx, SpanMode::PreOpen);
            }
        }

        self.write_span_info(
            id,
            &ctx,
            SpanMode::Open {
                verbose: self.config.verbose_entry,
            },
        );
    }

    fn on_event(&self, event: &Event<'_>, ctx: Context<S>) {
        let mut guard = self.bufs.lock().unwrap();
        let bufs = &mut *guard;
        let mut event_buf = &mut bufs.current_buf;

        // printing the indentation
        let indent = if ctx.current_span().id().is_some() {
            // size hint isn't implemented on Scope.
            ctx.event_scope(event)
                .expect("Unable to get span scope; this is a bug")
                .count()
        } else {
            0
        };

        // check if this event occurred in the context of a span.
        // if it has, get the start time of this span.
        let start = match ctx.current_span().id() {
            Some(id) => match ctx.span(id) {
                // if the event is in a span, get the span's starting point.
                Some(ctx) => {
                    let ext = ctx.extensions();
                    let data = ext
                        .get::<Data>()
                        .expect("Data cannot be found in extensions");
                    Some(data.start)
                }
                None => None,
            },
            None => None,
        };
        if let Some(start) = start {
            let elapsed = start.elapsed();
            write!(
                &mut event_buf,
                "{timestamp}{unit} ",
                timestamp = self.styled(Style::new().dimmed(), elapsed.as_millis().to_string()),
                unit = self.styled(Style::new().dimmed(), "ms"),
            )
            .expect("Unable to write to buffer");
        }

        #[cfg(feature = "tracing-log")]
        let normalized_meta = event.normalized_metadata();
        #[cfg(feature = "tracing-log")]
        let metadata = normalized_meta.as_ref().unwrap_or_else(|| event.metadata());
        #[cfg(not(feature = "tracing-log"))]
        let metadata = event.metadata();

        let level = metadata.level();
        let level = if self.config.ansi {
            ColorLevel(level).to_string()
        } else {
            level.to_string()
        };
        write!(&mut event_buf, "{level}", level = level).expect("Unable to write to buffer");

        if self.config.targets {
            let target = metadata.target();
            write!(
                &mut event_buf,
                " {}",
                self.styled(Style::new().dimmed(), target,),
            )
            .expect("Unable to write to buffer");
        }

        let mut visitor = FmtEvent { comma: false, bufs };
        event.record(&mut visitor);
        visitor
            .bufs
            .indent_current(indent, &self.config, SpanMode::Event);
        let writer = self.make_writer.make_writer();
        bufs.flush_current_buf(writer)
    }

    fn on_close(&self, id: Id, ctx: Context<S>) {
        self.write_span_info(
            &id,
            &ctx,
            SpanMode::Close {
                verbose: self.config.verbose_exit,
            },
        );

        if self.config.verbose_exit {
            if let Some(span) = ctx.span(&id).and_then(|span| span.parent()) {
                self.write_span_info(&span.id(), &ctx, SpanMode::PostClose);
            }
        }
    }
}

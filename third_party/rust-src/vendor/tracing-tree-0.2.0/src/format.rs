use ansi_term::Color;
use std::{
    fmt::{self, Write as _},
    io,
};
use tracing_core::{
    field::{Field, Visit},
    Level,
};

pub(crate) const LINE_VERT: &str = "│";
const LINE_HORIZ: &str = "─";
pub(crate) const LINE_BRANCH: &str = "├";
pub(crate) const LINE_CLOSE: &str = "┘";
pub(crate) const LINE_OPEN: &str = "┐";

#[derive(Copy, Clone)]
pub(crate) enum SpanMode {
    PreOpen,
    Open { verbose: bool },
    Close { verbose: bool },
    PostClose,
    Event,
}

#[derive(Debug)]
pub struct Config {
    /// Whether to use colors.
    pub ansi: bool,
    /// Whether an ascii art tree is used or (if false) whether to just use whitespace indent
    pub indent_lines: bool,
    /// The amount of chars to indent.
    pub indent_amount: usize,
    /// Whether to show the module paths.
    pub targets: bool,
    /// Whether to show thread ids.
    pub render_thread_ids: bool,
    /// Whether to show thread names.
    pub render_thread_names: bool,
    /// Specifies after how many indentation levels we will wrap back around to zero
    pub wraparound: usize,
    /// Whether to print the current span before activating a new one
    pub verbose_entry: bool,
    /// Whether to print the current span before exiting it.
    pub verbose_exit: bool,
    /// Whether to print squiggly brackets (`{}`) around the list of fields in a span.
    pub bracketed_fields: bool,
}

impl Config {
    pub fn with_ansi(self, ansi: bool) -> Self {
        Self { ansi, ..self }
    }

    pub fn with_indent_lines(self, indent_lines: bool) -> Self {
        Self {
            indent_lines,
            ..self
        }
    }

    pub fn with_targets(self, targets: bool) -> Self {
        Self { targets, ..self }
    }

    pub fn with_thread_ids(self, render_thread_ids: bool) -> Self {
        Self {
            render_thread_ids,
            ..self
        }
    }

    pub fn with_thread_names(self, render_thread_names: bool) -> Self {
        Self {
            render_thread_names,
            ..self
        }
    }

    pub fn with_wraparound(self, wraparound: usize) -> Self {
        Self { wraparound, ..self }
    }

    pub fn with_verbose_entry(self, verbose_entry: bool) -> Self {
        Self {
            verbose_entry,
            ..self
        }
    }

    pub fn with_verbose_exit(self, verbose_exit: bool) -> Self {
        Self {
            verbose_exit,
            ..self
        }
    }

    pub fn with_bracketed_fields(self, bracketed_fields: bool) -> Self {
        Self {
            bracketed_fields,
            ..self
        }
    }

    pub(crate) fn prefix(&self) -> String {
        let mut buf = String::new();
        if self.render_thread_ids {
            write!(buf, "{:?}", std::thread::current().id()).unwrap();
            if buf.ends_with(')') {
                buf.truncate(buf.len() - 1);
            }
            if buf.starts_with("ThreadId(") {
                buf.drain(0.."ThreadId(".len());
            }
        }
        if self.render_thread_names {
            if let Some(name) = std::thread::current().name() {
                if self.render_thread_ids {
                    buf.push(':');
                }
                buf.push_str(name);
            }
        }
        buf
    }
}

impl Default for Config {
    fn default() -> Self {
        Self {
            ansi: true,
            indent_lines: false,
            indent_amount: 2,
            targets: false,
            render_thread_ids: false,
            render_thread_names: false,
            wraparound: usize::max_value(),
            verbose_entry: false,
            verbose_exit: false,
            bracketed_fields: false,
        }
    }
}

#[derive(Debug)]
pub struct Buffers {
    pub current_buf: String,
    pub indent_buf: String,
}

impl Buffers {
    pub fn new() -> Self {
        Self {
            current_buf: String::new(),
            indent_buf: String::new(),
        }
    }

    pub fn flush_current_buf(&mut self, mut writer: impl io::Write) {
        write!(writer, "{}", &self.current_buf).unwrap();
        self.current_buf.clear();
    }

    pub fn flush_indent_buf(&mut self) {
        self.current_buf.push_str(&self.indent_buf);
        self.indent_buf.clear();
    }

    pub(crate) fn indent_current(&mut self, indent: usize, config: &Config, style: SpanMode) {
        self.current_buf.push('\n');
        let prefix = config.prefix();

        // Render something when wraparound occurs so the user is aware of it
        if config.indent_lines {
            match style {
                SpanMode::Close { .. } | SpanMode::PostClose => {
                    if indent > 0 && (indent + 1) % config.wraparound == 0 {
                        self.indent_buf.push_str(&prefix);
                        for _ in 0..(indent % config.wraparound * config.indent_amount) {
                            self.indent_buf.push_str(LINE_HORIZ);
                        }
                        self.indent_buf.push_str(LINE_OPEN);
                        self.indent_buf.push('\n');
                    }
                }
                _ => {}
            }
        }

        indent_block(
            &mut self.current_buf,
            &mut self.indent_buf,
            indent % config.wraparound,
            config.indent_amount,
            config.indent_lines,
            &prefix,
            style,
        );
        self.current_buf.clear();
        self.flush_indent_buf();

        // Render something when wraparound occurs so the user is aware of it
        if config.indent_lines {
            match style {
                SpanMode::PreOpen | SpanMode::Open { .. } => {
                    if indent > 0 && (indent + 1) % config.wraparound == 0 {
                        self.current_buf.push_str(&prefix);
                        for _ in 0..(indent % config.wraparound * config.indent_amount) {
                            self.current_buf.push_str(LINE_HORIZ);
                        }
                        self.current_buf.push_str(LINE_CLOSE);
                        self.current_buf.push('\n');
                    }
                }
                _ => {}
            }
        }
    }
}

pub struct FmtEvent<'a> {
    pub bufs: &'a mut Buffers,
    pub comma: bool,
}

impl<'a> Visit for FmtEvent<'a> {
    fn record_debug(&mut self, field: &Field, value: &dyn fmt::Debug) {
        let buf = &mut self.bufs.current_buf;
        let comma = if self.comma { "," } else { "" };
        match field.name() {
            "message" => {
                write!(buf, "{} {:?}", comma, value).unwrap();
                self.comma = true;
            }
            // Skip fields that are actually log metadata that have already been handled
            #[cfg(feature = "tracing-log")]
            name if name.starts_with("log.") => {}
            name => {
                write!(buf, "{} {}={:?}", comma, name, value).unwrap();
                self.comma = true;
            }
        }
    }
}

pub struct ColorLevel<'a>(pub &'a Level);

impl<'a> fmt::Display for ColorLevel<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self.0 {
            Level::TRACE => Color::Purple.bold().paint("TRACE"),
            Level::DEBUG => Color::Blue.bold().paint("DEBUG"),
            Level::INFO => Color::Green.bold().paint(" INFO"),
            Level::WARN => Color::RGB(252, 234, 160).bold().paint(" WARN"), // orange
            Level::ERROR => Color::Red.bold().paint("ERROR"),
        }
        .fmt(f)
    }
}

fn indent_block_with_lines(
    lines: &[&str],
    buf: &mut String,
    indent: usize,
    indent_amount: usize,
    prefix: &str,
    style: SpanMode,
) {
    let indent_spaces = indent * indent_amount;
    if lines.is_empty() {
        return;
    } else if indent_spaces == 0 {
        for line in lines {
            buf.push_str(prefix);
            // The first indent is special, we only need to print open/close and nothing else
            if indent == 0 {
                match style {
                    SpanMode::Open { .. } => buf.push_str(LINE_OPEN),
                    SpanMode::Close { .. } => buf.push_str(LINE_CLOSE),
                    SpanMode::PreOpen | SpanMode::PostClose => {}
                    SpanMode::Event => {}
                }
            }
            buf.push_str(line);
            buf.push('\n');
        }
        return;
    }
    let mut s = String::with_capacity(indent_spaces + prefix.len());
    s.push_str(prefix);

    // instead of using all spaces to indent, draw a vertical line at every indent level
    // up until the last indent
    for i in 0..(indent_spaces - indent_amount) {
        if i % indent_amount == 0 {
            s.push_str(LINE_VERT);
        } else {
            s.push(' ');
        }
    }

    // draw branch
    buf.push_str(&s);

    match style {
        SpanMode::PreOpen => {
            buf.push_str(LINE_BRANCH);
            for _ in 1..(indent_amount / 2) {
                buf.push_str(LINE_HORIZ);
            }
            buf.push_str(LINE_OPEN);
        }
        SpanMode::Open { verbose: false } => {
            buf.push_str(LINE_BRANCH);
            for _ in 1..indent_amount {
                buf.push_str(LINE_HORIZ);
            }
            buf.push_str(LINE_OPEN);
        }
        SpanMode::Open { verbose: true } => {
            buf.push_str(LINE_VERT);
            for _ in 1..(indent_amount / 2) {
                buf.push(' ');
            }
            // We don't have the space for fancy rendering at single space indent.
            if indent_amount > 1 {
                buf.push('└');
            }
            for _ in (indent_amount / 2)..(indent_amount - 1) {
                buf.push_str(LINE_HORIZ);
            }
            // We don't have the space for fancy rendering at single space indent.
            if indent_amount > 1 {
                buf.push_str(LINE_OPEN);
            } else {
                buf.push_str(LINE_VERT);
            }
        }
        SpanMode::Close { verbose: false } => {
            buf.push_str(LINE_BRANCH);
            for _ in 1..indent_amount {
                buf.push_str(LINE_HORIZ);
            }
            buf.push_str(LINE_CLOSE);
        }
        SpanMode::Close { verbose: true } => {
            buf.push_str(LINE_VERT);
            for _ in 1..(indent_amount / 2) {
                buf.push(' ');
            }
            // We don't have the space for fancy rendering at single space indent.
            if indent_amount > 1 {
                buf.push('┌');
            }
            for _ in (indent_amount / 2)..(indent_amount - 1) {
                buf.push_str(LINE_HORIZ);
            }
            // We don't have the space for fancy rendering at single space indent.
            if indent_amount > 1 {
                buf.push_str(LINE_CLOSE);
            } else {
                buf.push_str(LINE_VERT);
            }
        }
        SpanMode::PostClose => {
            buf.push_str(LINE_BRANCH);
            for _ in 1..(indent_amount / 2) {
                buf.push_str(LINE_HORIZ);
            }
            buf.push_str(LINE_CLOSE);
        }
        SpanMode::Event => {
            buf.push_str(LINE_BRANCH);

            // add `indent_amount - 1` horizontal lines before the span/event
            for _ in 0..(indent_amount - 1) {
                buf.push_str(LINE_HORIZ);
            }
        }
    }
    buf.push_str(lines[0]);
    buf.push('\n');

    // add the rest of the indentation, since we don't want to draw horizontal lines
    // for subsequent lines
    for i in 0..indent_amount {
        if i % indent_amount == 0 {
            s.push_str(LINE_VERT);
        } else {
            s.push(' ');
        }
    }

    // add all of the actual content, with each line preceded by the indent string
    for line in &lines[1..] {
        buf.push_str(&s);
        buf.push_str(line);
        buf.push('\n');
    }
}

fn indent_block(
    block: &mut String,
    buf: &mut String,
    indent: usize,
    indent_amount: usize,
    indent_lines: bool,
    prefix: &str,
    style: SpanMode,
) {
    let lines: Vec<&str> = block.lines().collect();
    let indent_spaces = indent * indent_amount;
    buf.reserve(block.len() + (lines.len() * indent_spaces));
    if indent_lines {
        indent_block_with_lines(&lines, buf, indent, indent_amount, prefix, style);
    } else {
        let indent_str = String::from(" ").repeat(indent_spaces);
        for line in lines {
            buf.push_str(prefix);
            buf.push_str(&indent_str);
            buf.push_str(line);
            buf.push('\n');
        }
    }
}

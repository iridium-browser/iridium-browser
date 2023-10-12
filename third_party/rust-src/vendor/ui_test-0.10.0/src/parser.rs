use std::{
    collections::HashMap,
    path::{Path, PathBuf},
};

use bstr::{ByteSlice, Utf8Error};
use regex::bytes::Regex;

use crate::{rustc_stderr::Level, Error, Mode};

use color_eyre::eyre::{Context, Result};

#[cfg(test)]
mod tests;

/// This crate supports various magic comments that get parsed as file-specific
/// configuration values. This struct parses them all in one go and then they
/// get processed by their respective use sites.
#[derive(Default, Debug)]
pub(crate) struct Comments {
    /// List of revision names to execute. Can only be speicified once
    pub revisions: Option<Vec<String>>,
    /// Comments that are only available under specific revisions.
    /// The defaults are in key `vec![]`
    pub revisioned: HashMap<Vec<String>, Revisioned>,
}

impl Comments {
    /// Check that a comment isn't specified twice across multiple differently revisioned statements.
    /// e.g. `//@[foo, bar] error-in-other-file: bop` and `//@[foo, baz] error-in-other-file boop` would end up
    /// specifying two error patterns that are available in revision `foo`.
    pub fn find_one_for_revision<'a, T: 'a>(
        &'a self,
        revision: &'a str,
        f: impl Fn(&'a Revisioned) -> Option<T>,
        error: impl FnOnce(T),
    ) -> Option<T> {
        let mut rev = self.for_revision(revision).filter_map(f);
        let result = rev.next();
        if let Some(next) = rev.next() {
            error(next);
        }
        result
    }

    /// Returns an iterator over all revisioned comments that match the revision.
    pub fn for_revision<'a>(&'a self, revision: &'a str) -> impl Iterator<Item = &'a Revisioned> {
        self.revisioned.iter().filter_map(move |(k, v)| {
            if k.is_empty() || k.iter().any(|rev| rev == revision) {
                Some(v)
            } else {
                None
            }
        })
    }

    pub(crate) fn edition(
        &self,
        errors: &mut Vec<Error>,
        revision: &str,
        config: &crate::Config,
    ) -> Option<(String, usize)> {
        self.find_one_for_revision(
            revision,
            |r| r.edition.as_ref(),
            |&(_, line)| {
                errors.push(Error::InvalidComment {
                    msg: "`edition` specified twice".into(),
                    line,
                })
            },
        )
        .cloned()
        .or(config.edition.clone().map(|e| (e, 0)))
    }
}

#[derive(Default, Debug)]
/// Comments that can be filtered for specific revisions.
pub(crate) struct Revisioned {
    /// Don't run this test if any of these filters apply
    pub ignore: Vec<Condition>,
    /// Only run this test if all of these filters apply
    pub only: Vec<Condition>,
    /// Generate one .stderr file per bit width, by prepending with `.64bit` and similar
    pub stderr_per_bitwidth: bool,
    /// Additional flags to pass to the executable
    pub compile_flags: Vec<String>,
    /// Additional env vars to set for the executable
    pub env_vars: Vec<(String, String)>,
    /// Normalizations to apply to the stderr output before emitting it to disk
    pub normalize_stderr: Vec<(Regex, Vec<u8>)>,
    /// Arbitrary patterns to look for in the stderr.
    /// The error must be from another file, as errors from the current file must be
    /// checked via `error_matches`.
    pub error_in_other_files: Vec<(Pattern, usize)>,
    pub error_matches: Vec<ErrorMatch>,
    /// Ignore diagnostics below this level.
    /// `None` means pick the lowest level from the `error_pattern`s.
    pub require_annotations_for_level: Option<Level>,
    pub aux_builds: Vec<(PathBuf, String, usize)>,
    pub edition: Option<(String, usize)>,
    /// Overwrites the mode from `Config`.
    pub mode: Option<(Mode, usize)>,
    pub needs_asm_support: bool,
}

#[derive(Debug)]
struct CommentParser<T> {
    /// The comments being built.
    comments: T,
    /// Any errors that ocurred during comment parsing.
    errors: Vec<Error>,
    /// The line currently being parsed.
    line: usize,
}

impl<T> std::ops::Deref for CommentParser<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.comments
    }
}

impl<T> std::ops::DerefMut for CommentParser<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.comments
    }
}

/// The conditions used for "ignore" and "only" filters.
#[derive(Debug)]
pub(crate) enum Condition {
    /// The given string must appear in the host triple.
    Host(String),
    /// The given string must appear in the target triple.
    Target(String),
    /// Tests that the bitwidth is the given one.
    Bitwidth(u8),
    /// Tests that the target is the host.
    OnHost,
}

#[derive(Debug, Clone)]
/// An error pattern parsed from a `//~` comment.
pub enum Pattern {
    SubString(String),
    Regex(Regex),
}

#[derive(Debug)]
pub(crate) struct ErrorMatch {
    pub pattern: Pattern,
    pub level: Level,
    /// The line where the message was defined, for reporting issues with it (e.g. in case it wasn't found).
    pub definition_line: usize,
    /// The line this pattern is expecting to find a message in.
    pub line: usize,
}

impl Condition {
    fn parse(c: &str) -> std::result::Result<Self, String> {
        if c == "on-host" {
            Ok(Condition::OnHost)
        } else if let Some(bits) = c.strip_suffix("bit") {
            let bits: u8 = bits.parse().map_err(|_err| {
                format!("invalid ignore/only filter ending in 'bit': {c:?} is not a valid bitwdith")
            })?;
            Ok(Condition::Bitwidth(bits))
        } else if let Some(triple_substr) = c.strip_prefix("target-") {
            Ok(Condition::Target(triple_substr.to_owned()))
        } else if let Some(triple_substr) = c.strip_prefix("host-") {
            Ok(Condition::Host(triple_substr.to_owned()))
        } else {
            Err(format!(
                "`{c}` is not a valid condition, expected `on-host`, /[0-9]+bit/, /host-.*/, or /target-.*/"
            ))
        }
    }
}

impl Comments {
    pub(crate) fn parse_file(path: &Path) -> Result<std::result::Result<Self, Vec<Error>>> {
        let content =
            std::fs::read(path).wrap_err_with(|| format!("failed to read {}", path.display()))?;
        Ok(Self::parse(&content))
    }

    /// Parse comments in `content`.
    /// `path` is only used to emit diagnostics if parsing fails.
    pub(crate) fn parse(
        content: &(impl AsRef<[u8]> + ?Sized),
    ) -> std::result::Result<Self, Vec<Error>> {
        let mut parser = CommentParser {
            comments: Comments::default(),
            errors: vec![],
            line: 0,
        };

        let mut fallthrough_to = None; // The line that a `|` will refer to.
        for (l, line) in content.as_ref().lines().enumerate() {
            let l = l + 1; // enumerate starts at 0, but line numbers start at 1
            parser.line = l;
            match parser.parse_checked_line(&mut fallthrough_to, line) {
                Ok(()) => {}
                Err(e) => parser.errors.push(Error::InvalidComment {
                    msg: format!("Comment is not utf8: {e:?}"),
                    line: l,
                }),
            }
        }
        if parser.errors.is_empty() {
            Ok(parser.comments)
        } else {
            Err(parser.errors)
        }
    }
}

impl CommentParser<Comments> {
    fn parse_checked_line(
        &mut self,
        fallthrough_to: &mut Option<usize>,
        line: &[u8],
    ) -> std::result::Result<(), Utf8Error> {
        if let Some(command) = line.strip_prefix(b"//@") {
            self.parse_command(command.trim().to_str()?)
        } else if let Some((_, pattern)) = line.split_once_str("//~") {
            let (revisions, pattern) = self.parse_revisions(pattern.to_str()?);
            self.revisioned(revisions, |this| {
                this.parse_pattern(pattern, fallthrough_to)
            })
        } else {
            *fallthrough_to = None;
            for pos in line.find_iter("//") {
                let rest = &line[pos + 2..];
                for rest in std::iter::once(rest).chain(rest.strip_prefix(b" ")) {
                    if let Some('@' | '~' | '[' | ']' | '^' | '|') = rest.chars().next() {
                        self.errors.push(Error::InvalidComment {
                            msg: format!(
                                "comment looks suspiciously like a test suite command: `{}`\n\
                             All `//@` test suite commands must be at the start of the line.\n\
                             The `//` must be directly followed by `@` or `~`.",
                                rest.to_str()?,
                            ),
                            line: self.line,
                        })
                    } else {
                        let mut parser = Self {
                            line: 0,
                            errors: vec![],
                            comments: Comments::default(),
                        };
                        parser.parse_command(rest.to_str()?);
                        if parser.errors.is_empty() {
                            self.errors.push(Error::InvalidComment {
                                msg: "a compiletest-rs style comment was detected.\n\
                                Please use text that could not also be interpreted as a command,\n\
                                and prefix all actual commands with `//@`"
                                    .into(),
                                line: self.line,
                            });
                        }
                    }
                }
            }
        }
        Ok(())
    }
}

impl<CommentsType> CommentParser<CommentsType> {
    fn error(&mut self, s: impl Into<String>) {
        self.errors.push(Error::InvalidComment {
            msg: s.into(),
            line: self.line,
        });
    }

    fn check(&mut self, cond: bool, s: impl Into<String>) {
        if !cond {
            self.error(s);
        }
    }

    fn check_some<T>(&mut self, opt: Option<T>, s: impl Into<String>) -> Option<T> {
        self.check(opt.is_some(), s);
        opt
    }
}

impl CommentParser<Comments> {
    fn parse_command(&mut self, command: &str) {
        let (revisions, command) = self.parse_revisions(command);

        // Commands are letters or dashes, grab everything until the first character that is neither of those.
        let (command, args) = match command
            .char_indices()
            .find_map(|(i, c)| (!c.is_alphanumeric() && c != '-' && c != '_').then_some(i))
        {
            None => (command, ""),
            Some(i) => {
                let (command, args) = command.split_at(i);
                let mut args = args.chars();
                // Commands are separated from their arguments by ':' or ' '
                let next = args
                    .next()
                    .expect("the `position` above guarantees that there is at least one char");
                self.check(
                    next == ':',
                    "test command must be followed by `:` (or end the line)",
                );
                (command, args.as_str().trim())
            }
        };

        if command == "revisions" {
            self.check(
                revisions.is_empty(),
                "revisions cannot be declared under a revision",
            );
            self.check(self.revisions.is_none(), "cannot specify `revisions` twice");
            self.revisions = Some(args.split_whitespace().map(|s| s.to_string()).collect());
            return;
        }
        self.revisioned(revisions, |this| this.parse_command(command, args));
    }

    fn revisioned(
        &mut self,
        revisions: Vec<String>,
        f: impl FnOnce(&mut CommentParser<&mut Revisioned>),
    ) {
        let mut this = CommentParser {
            errors: std::mem::take(&mut self.errors),
            line: self.line,
            comments: self.revisioned.entry(revisions).or_default(),
        };
        f(&mut this);
        self.errors = this.errors;
    }
}

impl CommentParser<&mut Revisioned> {
    fn parse_command(&mut self, command: &str, args: &str) {
        match command {
            "compile-flags" => {
                self.compile_flags
                    .extend(args.split_whitespace().map(|s| s.to_string()));
            }
            "rustc-env" => {
                for env in args.split_whitespace() {
                    if let Some((k, v)) = self.check_some(
                        env.split_once('='),
                        "environment variables must be key/value pairs separated by a `=`",
                    ) {
                        self.env_vars.push((k.to_string(), v.to_string()));
                    }
                }
            }
            "normalize-stderr-test" => {
                let (from, rest) = self.parse_str(args);

                let to = match rest.strip_prefix("->") {
                    Some(v) => v,
                    None => {
                        self.error("normalize-stderr-test needs a pattern and replacement separated by `->`");
                        return;
                    },
                }.trim_start();
                let (to, rest) = self.parse_str(to);

                self.check(
                    rest.is_empty(),
                    format!("trailing text after pattern replacement: {rest}"),
                );

                if let Some(regex) = self.parse_regex(from) {
                    self.normalize_stderr
                        .push((regex, to.as_bytes().to_owned()))
                }
            }
            "error-pattern" => {
                self.error("`error-pattern` has been renamed to `error-in-other-file`");
            }
            "error-in-other-file" => {
                let pat = self.parse_error_pattern(args.trim());
                let line = self.line;
                self.error_in_other_files.push((pat, line));
            }
            "stderr-per-bitwidth" => {
                // args are ignored (can be used as comment)
                self.check(
                    !self.stderr_per_bitwidth,
                    "cannot specify `stderr-per-bitwidth` twice",
                );
                self.stderr_per_bitwidth = true;
            }
            "run-rustfix" => {
                // args are ignored (can be used as comment)
                self.check(
                    self.mode.is_none(),
                    "cannot specify test mode changes twice",
                );
                self.mode = Some((Mode::Fix, self.line))
            }
            "needs-asm-support" => {
                // args are ignored (can be used as comment)
                self.check(
                    !self.needs_asm_support,
                    "cannot specify `needs-asm-support` twice",
                );
                self.needs_asm_support = true;
            }
            "aux-build" => {
                let (name, kind) = args.split_once(':').unwrap_or((args, "lib"));
                let line = self.line;
                self.aux_builds.push((name.into(), kind.into(), line));
            }
            "edition" => {
                self.check(self.edition.is_none(), "cannot specify `edition` twice");
                self.edition = Some((args.into(), self.line))
            }
            "check-pass" => {
                // args are ignored (can be used as comment)
                self.check(
                    self.mode.is_none(),
                    "cannot specify test mode changes twice",
                );
                self.mode = Some((Mode::Pass, self.line))
            }
            "run" => {
                self.check(
                    self.mode.is_none(),
                    "cannot specify test mode changes twice",
                );
                let mut set = |exit_code| self.mode = Some((Mode::Run { exit_code }, self.line));
                if args.is_empty() {
                    set(0);
                } else {
                    match args.parse() {
                        Ok(exit_code) => set(exit_code),
                        Err(err) => self.error(err.to_string()),
                    }
                }
            }
            "require-annotations-for-level" => {
                self.check(
                    self.require_annotations_for_level.is_none(),
                    "cannot specify `require-annotations-for-level` twice",
                );
                match args.trim().parse() {
                    Ok(it) => self.require_annotations_for_level = Some(it),
                    Err(msg) => self.error(msg),
                }
            }
            command => {
                if let Some(s) = command.strip_prefix("ignore-") {
                    // args are ignored (can be used as comment)
                    match Condition::parse(s) {
                        Ok(cond) => self.ignore.push(cond),
                        Err(msg) => self.error(msg),
                    }
                    return;
                }

                if let Some(s) = command.strip_prefix("only-") {
                    // args are ignored (can be used as comment)
                    match Condition::parse(s) {
                        Ok(cond) => self.only.push(cond),
                        Err(msg) => self.error(msg),
                    }
                    return;
                }
                self.error(format!("unknown command `{command}`"));
            }
        }
    }
}

impl<CommentsType> CommentParser<CommentsType> {
    fn parse_regex(&mut self, regex: &str) -> Option<Regex> {
        match Regex::new(regex) {
            Ok(regex) => Some(regex),
            Err(err) => {
                self.error(format!("invalid regex: {err:?}"));
                None
            }
        }
    }

    /// Parses a string literal. `s` has to start with `"`; everything until the next `"` is
    /// returned in the first component. `\` can be used to escape arbitrary character.
    /// Second return component is the rest of the string with leading whitespace removed.
    fn parse_str<'a>(&mut self, s: &'a str) -> (&'a str, &'a str) {
        let mut chars = s.char_indices();
        match chars.next() {
            Some((_, '"')) => {
                let s = chars.as_str();
                let mut escaped = false;
                for (i, c) in chars {
                    if escaped {
                        // Accept any character as literal after a `\`.
                        escaped = false;
                    } else if c == '"' {
                        return (&s[..(i - 1)], s[i..].trim_start());
                    } else {
                        escaped = c == '\\';
                    }
                }
                self.error(format!("no closing quotes found for {s}"));
                (s, "")
            }
            Some((_, c)) => {
                self.error(format!("expected `\"`, got `{c}`"));
                (s, "")
            }
            None => {
                self.error("expected quoted string, but found end of line");
                (s, "")
            }
        }
    }

    // parse something like \[[a-z]+(,[a-z]+)*\]
    fn parse_revisions<'a>(&mut self, pattern: &'a str) -> (Vec<String>, &'a str) {
        match pattern.chars().next() {
            Some('[') => {
                // revisions
                let s = &pattern[1..];
                let end = s.char_indices().find_map(|(i, c)| match c {
                    ']' => Some(i),
                    _ => None,
                });
                let Some(end) = end else {
                    self.error("`[` without corresponding `]`");
                    return (vec![], pattern);
                };
                let (revision, pattern) = s.split_at(end);
                (
                    revision.split(',').map(|s| s.trim().to_string()).collect(),
                    // 1.. because `split_at` includes the separator
                    pattern[1..].trim_start(),
                )
            }
            _ => (vec![], pattern),
        }
    }
}

impl CommentParser<&mut Revisioned> {
    // parse something like (\[[a-z]+(,[a-z]+)*\])?(?P<offset>\||[\^]+)? *(?P<level>ERROR|HELP|WARN|NOTE): (?P<text>.*)
    fn parse_pattern(&mut self, pattern: &str, fallthrough_to: &mut Option<usize>) {
        let (match_line, pattern) = match pattern.chars().next() {
            Some('|') => (
                match fallthrough_to {
                    Some(fallthrough) => *fallthrough,
                    None => {
                        self.error("`//~|` pattern without preceding line");
                        return;
                    }
                },
                &pattern[1..],
            ),
            Some('^') => {
                let offset = pattern.chars().take_while(|&c| c == '^').count();
                (self.line - offset, &pattern[offset..])
            }
            Some(_) => (self.line, pattern),
            None => {
                self.error("no pattern specified");
                return;
            }
        };

        let pattern = pattern.trim_start();
        let offset = match pattern.chars().position(|c| !c.is_ascii_alphabetic()) {
            Some(offset) => offset,
            None => {
                self.error("pattern without level");
                return;
            }
        };

        let level = match pattern[..offset].parse() {
            Ok(level) => level,
            Err(msg) => {
                self.error(msg);
                return;
            }
        };
        let pattern = &pattern[offset..];
        let pattern = match pattern.strip_prefix(':') {
            Some(offset) => offset,
            None => {
                self.error("no `:` after level found");
                return;
            }
        };

        let pattern = pattern.trim();

        self.check(!pattern.is_empty(), "no pattern specified");

        let pattern = self.parse_error_pattern(pattern);

        *fallthrough_to = Some(match_line);

        let definition_line = self.line;
        self.error_matches.push(ErrorMatch {
            pattern,
            level,
            definition_line,
            line: match_line,
        });
    }
}

impl Pattern {
    pub(crate) fn matches(&self, message: &str) -> bool {
        match self {
            Pattern::SubString(s) => message.contains(s),
            Pattern::Regex(r) => r.is_match(message.as_bytes()),
        }
    }
}

impl<CommentsType> CommentParser<CommentsType> {
    fn parse_error_pattern(&mut self, pattern: &str) -> Pattern {
        if let Some(regex) = pattern.strip_prefix('/') {
            match regex.strip_suffix('/') {
                Some(regex) => match self.parse_regex(regex) {
                    Some(regex) => Pattern::Regex(regex),
                    None => Pattern::SubString(pattern.to_string()),
                },
                None => {
                    self.error(
                        "expected regex pattern due to leading `/`, but found no closing `/`",
                    );
                    Pattern::SubString(pattern.to_string())
                }
            }
        } else {
            Pattern::SubString(pattern.to_string())
        }
    }
}

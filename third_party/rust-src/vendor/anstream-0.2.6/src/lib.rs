//! **Auto-adapting [`stdout`] / [`stderr`] streams**
//!
//! *A portmanteau of "ansi stream"*
//!
//! [`AutoStream`] always accepts [ANSI escape codes](https://en.wikipedia.org/wiki/ANSI_escape_code),
//! adapting to the user's terminal's capabilities.
//!
//! Benefits
//! - Allows the caller to not be concerned with the terminal's capabilities
//! - Semver safe way of passing styled text between crates as ANSI escape codes offer more
//!   compatibility than most crate APIs.
//!
//! # Example
//!
//! ```
//! #  #[cfg(feature = "auto")] {
//! use anstream::println;
//! use owo_colors::OwoColorize as _;
//!
//! // Foreground colors
//! println!("My number is {:#x}!", 10.green());
//! // Background colors
//! println!("My number is not {}!", 4.on_red());
//! # }
//! ```
//!
//! And this will correctly handle piping to a file, etc

#![cfg_attr(docsrs, feature(doc_auto_cfg))]

pub mod adapter;
mod buffer;
#[macro_use]
mod macros;
mod auto;
mod lockable;
mod raw;
mod strip;
#[cfg(all(windows, feature = "wincon"))]
mod wincon;

pub use auto::AutoStream;
pub use lockable::Lockable;
pub use raw::RawStream;
pub use strip::StripStream;
#[cfg(all(windows, feature = "wincon"))]
pub use wincon::WinconStream;

pub use buffer::Buffer;

/// Create an ANSI escape code compatible stdout
///
/// **Note:** Call [`AutoStream::lock`] in loops to avoid the performance hit of acquiring/releasing
/// from the implicit locking in each [`std::io::Write`] call
#[cfg(feature = "auto")]
pub fn stdout() -> AutoStream<std::io::Stdout> {
    let stdout = std::io::stdout();
    AutoStream::auto(stdout)
}

/// Create an ANSI escape code compatible stderr
///
/// **Note:** Call [`AutoStream::lock`] in loops to avoid the performance hit of acquiring/releasing
/// from the implicit locking in each [`std::io::Write`] call
#[cfg(feature = "auto")]
pub fn stderr() -> AutoStream<std::io::Stderr> {
    let stderr = std::io::stderr();
    AutoStream::auto(stderr)
}

/// Selection for overriding color output with [`force_color`]
#[cfg(feature = "auto")]
pub use concolor_override::ColorChoice;

/// Override the detected [`ColorChoice`]
#[cfg(feature = "auto")]
pub fn force_color(choice: ColorChoice) {
    concolor_override::set(choice);
}

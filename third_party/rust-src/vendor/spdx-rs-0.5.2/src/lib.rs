// SPDX-FileCopyrightText: 2020 HH Partners
//
// SPDX-License-Identifier: MIT

#![doc = include_str!("../README.md")]
#![warn(clippy::all, clippy::pedantic, clippy::nursery, clippy::cargo)]
#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::non_ascii_literal,
    clippy::missing_const_for_fn,
    clippy::use_self
)]

pub mod error;
pub mod models;
pub mod parsers;

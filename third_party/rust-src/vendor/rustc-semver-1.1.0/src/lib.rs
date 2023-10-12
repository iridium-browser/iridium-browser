#![no_std]

use core::{cmp::Ordering, fmt::Display, num::ParseIntError};

/// `Error` represents an Error during parsing of a [`RustcVersion`].
#[derive(Debug, Eq, PartialEq, Copy, Clone)]
pub enum Error {
    /// A version was passed that has too many elements seperated by `'.'`.
    TooManyElements,
    /// A version was passed that neither is a [`SpecialVersion`], nor a
    /// normal [`RustcVersion`].
    NotASpecialVersion,
    /// A version was passed, that was either an empty string or a part of the
    /// version was left out, e.g. `1.  .3`
    EmptyVersionPart,
    /// A version was passed that has unallowed chracters.
    ParseIntError,
}

impl From<ParseIntError> for Error {
    fn from(_: ParseIntError) -> Self {
        Self::ParseIntError
    }
}

/// Result type for this crate
pub type Result<T> = core::result::Result<T, Error>;

/// `RustcVersion` represents a version of the Rust Compiler.
///
/// This struct only supports the [`NormalVersion`] format
/// ```text
/// major.minor.patch
/// ```
/// and 3 special formats represented by the [`SpecialVersion`] enum.
///
/// A version can be created with one of the functions [`RustcVersion::new`] or
/// [`RustcVersion::parse`]. The [`RustcVersion::new`] method only supports the
/// normal version format.
///
/// You can compare two versions, just as you would expect:
///
/// ```rust
/// use rustc_semver::RustcVersion;
///
/// assert!(RustcVersion::new(1, 34, 0) > RustcVersion::parse("1.10").unwrap());
/// assert!(RustcVersion::new(1, 34, 0) > RustcVersion::parse("0.9").unwrap());
/// ```
///
/// This comparison is semver conform according to the [semver definition of
/// precedence]. However, if you want to check whether one version meets
/// another version according to the [Caret Requirements], you should use
/// [`RustcVersion::meets`].
///
/// [Caret Requirements]: https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#caret-requirements
/// [semver definition of precedence]: https://semver.org/#spec-item-11
#[derive(Debug, Eq, PartialEq, Copy, Clone)]
pub enum RustcVersion {
    Normal(NormalVersion),
    Special(SpecialVersion),
}

/// `NormalVersion` represents a normal version used for all releases since
/// Rust 1.0.0.
///
/// This struct supports versions in the format
/// ```test
/// major.minor.patch
/// ```
#[derive(Debug, Copy, Clone)]
pub struct NormalVersion {
    major: u32,
    minor: u32,
    patch: u32,
    omitted: OmittedParts,
}

#[derive(Debug, Copy, Clone)]
enum OmittedParts {
    None,
    Minor,
    Patch,
}

impl From<usize> for OmittedParts {
    fn from(parts: usize) -> Self {
        match parts {
            1 => Self::Minor,
            2 => Self::Patch,
            3 => Self::None,
            _ => unreachable!(
                "This function should never be called with `parts == 0` or `parts > 3`"
            ),
        }
    }
}

/// `SpecialVersion` represents a special version from the first releases.
///
/// Before Rust 1.0.0, there were two alpha and one beta release, namely
///
/// - `1.0.0-alpha`
/// - `1.0.0-alpha.2`
/// - `1.0.0-beta`
///
/// This enum represents those releases.
#[derive(Debug, Eq, PartialEq, Copy, Clone)]
pub enum SpecialVersion {
    Alpha,
    Alpha2,
    Beta,
}

impl PartialOrd for RustcVersion {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for RustcVersion {
    fn cmp(&self, other: &Self) -> Ordering {
        match (self, other) {
            (Self::Normal(ver), Self::Normal(o_ver)) => ver.cmp(o_ver),
            (Self::Normal(NormalVersion { major, .. }), Self::Special(_)) => {
                if *major >= 1 {
                    Ordering::Greater
                } else {
                    Ordering::Less
                }
            }
            (Self::Special(_), Self::Normal(NormalVersion { major, .. })) => {
                if *major >= 1 {
                    Ordering::Less
                } else {
                    Ordering::Greater
                }
            }
            (Self::Special(s_ver), Self::Special(o_s_ver)) => s_ver.cmp(o_s_ver),
        }
    }
}

impl PartialOrd for NormalVersion {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for NormalVersion {
    fn cmp(&self, other: &Self) -> Ordering {
        match self.major.cmp(&other.major) {
            Ordering::Equal => match self.minor.cmp(&other.minor) {
                Ordering::Equal => self.patch.cmp(&other.patch),
                ord => ord,
            },
            ord => ord,
        }
    }
}

impl PartialEq for NormalVersion {
    fn eq(&self, other: &Self) -> bool {
        self.major == other.major && self.minor == other.minor && self.patch == other.patch
    }
}

impl Eq for NormalVersion {}

impl PartialOrd for SpecialVersion {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for SpecialVersion {
    fn cmp(&self, other: &Self) -> Ordering {
        match (self, other) {
            (Self::Alpha, Self::Alpha)
            | (Self::Alpha2, Self::Alpha2)
            | (Self::Beta, Self::Beta) => Ordering::Equal,
            (Self::Alpha, _) | (Self::Alpha2, Self::Beta) => Ordering::Less,
            (Self::Beta, _) | (Self::Alpha2, Self::Alpha) => Ordering::Greater,
        }
    }
}

impl Display for RustcVersion {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Self::Normal(NormalVersion {
                major,
                minor,
                patch,
                ..
            }) => write!(f, "{}.{}.{}", major, minor, patch),
            Self::Special(special) => write!(f, "{}", special),
        }
    }
}

impl Display for SpecialVersion {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Self::Alpha => write!(f, "1.0.0-alpha"),
            Self::Alpha2 => write!(f, "1.0.0-alpha.2"),
            Self::Beta => write!(f, "1.0.0-beta"),
        }
    }
}

impl From<[u32; 3]> for NormalVersion {
    fn from(arr: [u32; 3]) -> Self {
        NormalVersion {
            major: arr[0],
            minor: arr[1],
            patch: arr[2],
            omitted: OmittedParts::None,
        }
    }
}

const ACCEPTED_SPECIAL_VERSIONS: [(&str, SpecialVersion); 3] = [
    ("1.0.0-alpha", SpecialVersion::Alpha),
    ("1.0.0-alpha.2", SpecialVersion::Alpha2),
    ("1.0.0-beta", SpecialVersion::Beta),
];

impl RustcVersion {
    /// `RustcVersion::new` is a `const` constructor for a `RustcVersion`.
    ///
    /// This function is primarily used to construct constants, for everything
    /// else use [`RustcVersion::parse`].
    ///
    /// This function only allows to construct normal versions. For special
    /// versions, construct them directly with the [`SpecialVersion`] enum.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use rustc_semver::RustcVersion;
    ///
    /// const MY_FAVORITE_RUST: RustcVersion = RustcVersion::new(1, 48, 0);
    ///
    /// assert!(MY_FAVORITE_RUST > RustcVersion::new(1, 0, 0))
    /// ```
    pub const fn new(major: u32, minor: u32, patch: u32) -> Self {
        Self::Normal(NormalVersion {
            major,
            minor,
            patch,
            omitted: OmittedParts::None,
        })
    }

    /// `RustcVersion::parse` parses a [`RustcVersion`].
    ///
    /// This function can parse all normal and special versions. It is possbile
    /// to omit parts of the version, like the patch or minor version part. So
    /// `1`, `1.0`, and `1.0.0` are all valid inputs and will result in the
    /// same version.
    ///
    /// # Errors
    ///
    /// This function returns an [`Error`], if the passed string is not a valid
    /// [`RustcVersion`]
    ///
    /// # Examples
    ///
    /// ```rust
    /// use rustc_semver::{SpecialVersion, RustcVersion};
    ///
    /// let ver = RustcVersion::new(1, 0, 0);
    ///
    /// assert_eq!(RustcVersion::parse("1").unwrap(), ver);
    /// assert_eq!(RustcVersion::parse("1.0").unwrap(), ver);
    /// assert_eq!(RustcVersion::parse("1.0.0").unwrap(), ver);
    /// assert_eq!(
    ///     RustcVersion::parse("1.0.0-alpha").unwrap(),
    ///     RustcVersion::Special(SpecialVersion::Alpha)
    /// );
    /// ```
    pub fn parse(version: &str) -> Result<Self> {
        let special_version = ACCEPTED_SPECIAL_VERSIONS.iter().find_map(|sv| {
            if version == sv.0 {
                Some(sv.1)
            } else {
                None
            }
        });
        if let Some(special_version) = special_version {
            return Ok(RustcVersion::Special(special_version));
        }

        let mut rustc_version = [0_u32; 3];
        let mut parts = 0;
        for (i, part) in version.split('.').enumerate() {
            let part = part.trim();
            if part.is_empty() {
                return Err(Error::EmptyVersionPart);
            }
            if i == 3 {
                return Err(Error::TooManyElements);
            }
            match str::parse(part) {
                Ok(part) => rustc_version[i] = part,
                Err(e) => {
                    if i == 2 {
                        return Err(Error::NotASpecialVersion);
                    } else {
                        return Err(e.into());
                    }
                }
            }

            parts = i + 1;
        }

        let mut ver = NormalVersion::from(rustc_version);
        ver.omitted = OmittedParts::from(parts);
        Ok(RustcVersion::Normal(ver))
    }

    /// `RustcVersion::meets` implements a semver conform version check
    /// according to the [Caret Requirements].
    ///
    /// Note that [`SpecialVersion`]s only meet themself and no other version
    /// meets a [`SpecialVersion`]. This is because [according to semver],
    /// special versions are considered unstable and "might not satisfy the
    /// intended compatibility requirements as denoted by \[their\] associated
    /// normal version".
    ///
    /// # Examples
    ///
    /// ```rust
    /// use rustc_semver::RustcVersion;
    ///
    /// assert!(RustcVersion::new(1, 30, 0).meets(RustcVersion::parse("1.29").unwrap()));
    /// assert!(!RustcVersion::new(1, 30, 0).meets(RustcVersion::parse("1.31").unwrap()));
    ///
    /// assert!(RustcVersion::new(0, 2, 1).meets(RustcVersion::parse("0.2").unwrap()));
    /// assert!(!RustcVersion::new(0, 3, 0).meets(RustcVersion::parse("0.2").unwrap()));
    /// ```
    ///
    /// [Caret Requirements]: https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#caret-requirements
    /// [according to semver]: https://semver.org/#spec-item-9
    pub fn meets(self, other: Self) -> bool {
        match (self, other) {
            (RustcVersion::Special(_), _) | (_, RustcVersion::Special(_)) => self == other,
            (RustcVersion::Normal(ver), RustcVersion::Normal(o_ver)) => {
                // In any case must `self` be bigger than `other`, with the major part matching
                // the other version.
                let mut meets = ver >= o_ver && ver.major == o_ver.major;

                // In addition, the left-most non-zero digit must not be modified.
                match o_ver.omitted {
                    OmittedParts::None => {
                        // Nothing was omitted, this means that everything must match in case of
                        // leading zeros.
                        if o_ver.major == 0 {
                            // Leading 0 in major position, check for
                            // `self.minor == other.minor`
                            meets &= ver.minor == o_ver.minor;

                            if o_ver.minor == 0 {
                                // Leading 0 in minor position, check for
                                // `self.patch == other.patch`.
                                meets &= ver.patch == o_ver.patch;
                            }
                        }
                    }
                    OmittedParts::Patch => {
                        // The patch version was omitted, this means the patch version of `self`
                        // does not have to match the patch version of `other`.
                        if o_ver.major == 0 {
                            meets &= ver.minor == o_ver.minor;
                        }
                    }
                    OmittedParts::Minor => {
                        // The minor (and patch) version was omitted, this means
                        // the minor and patch version of `self` do not have to
                        // match the minor and patch version of `other`
                    }
                }

                meets
            }
        }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn omitted_parts() {
        assert_eq!(
            RustcVersion::parse("1.0.0").unwrap(),
            RustcVersion::new(1, 0, 0)
        );
        assert_eq!(
            RustcVersion::parse("1.0").unwrap(),
            RustcVersion::new(1, 0, 0)
        );
        assert_eq!(
            RustcVersion::parse("1").unwrap(),
            RustcVersion::new(1, 0, 0)
        );
    }

    #[test]
    fn special_versions() {
        assert_eq!(
            RustcVersion::parse("1.0.0-alpha").unwrap(),
            RustcVersion::Special(SpecialVersion::Alpha)
        );
        assert_eq!(
            RustcVersion::parse("1.0.0-alpha.2").unwrap(),
            RustcVersion::Special(SpecialVersion::Alpha2)
        );
        assert_eq!(
            RustcVersion::parse("1.0.0-beta").unwrap(),
            RustcVersion::Special(SpecialVersion::Beta)
        );
        assert_eq!(
            RustcVersion::parse("1.0.0-sigma"),
            Err(Error::NotASpecialVersion)
        );
        assert_eq!(
            RustcVersion::parse("1.0.0beta"),
            Err(Error::NotASpecialVersion)
        );
        assert_eq!(
            RustcVersion::parse("1.1.0-beta"),
            Err(Error::NotASpecialVersion)
        );
    }

    #[test]
    fn less_than() {
        let bigger = RustcVersion::new(1, 30, 1);
        assert!(RustcVersion::parse("1.0.0").unwrap() < bigger);
        assert!(RustcVersion::parse("1.0").unwrap() < bigger);
        assert!(RustcVersion::parse("1").unwrap() < bigger);
        assert!(RustcVersion::parse("1.30").unwrap() < bigger);
        assert!(RustcVersion::parse("1.0.0-beta").unwrap() < bigger);
        assert!(RustcVersion::parse("0.9").unwrap() < RustcVersion::Special(SpecialVersion::Alpha));
        assert!(
            RustcVersion::parse("1.0.0-alpha").unwrap()
                < RustcVersion::Special(SpecialVersion::Alpha2)
        );
        assert!(
            RustcVersion::parse("1.0.0-alpha").unwrap()
                < RustcVersion::Special(SpecialVersion::Beta)
        );
        assert!(
            RustcVersion::parse("1.0.0-alpha.2").unwrap()
                < RustcVersion::Special(SpecialVersion::Beta)
        );
    }

    #[test]
    fn equal() {
        assert_eq!(
            RustcVersion::parse("1.22.0").unwrap(),
            RustcVersion::new(1, 22, 0)
        );
        assert_eq!(
            RustcVersion::parse("1.22").unwrap(),
            RustcVersion::new(1, 22, 0)
        );
        assert_eq!(
            RustcVersion::parse("1.48.1").unwrap(),
            RustcVersion::new(1, 48, 1)
        );
        assert_eq!(
            RustcVersion::parse("1.0.0-alpha")
                .unwrap()
                .cmp(&RustcVersion::Special(SpecialVersion::Alpha)),
            Ordering::Equal
        );
        assert_eq!(
            RustcVersion::parse("1.0.0-alpha.2")
                .unwrap()
                .cmp(&RustcVersion::Special(SpecialVersion::Alpha2)),
            Ordering::Equal
        );
        assert_eq!(
            RustcVersion::parse("1.0.0-beta")
                .unwrap()
                .cmp(&RustcVersion::Special(SpecialVersion::Beta)),
            Ordering::Equal
        );
    }

    #[test]
    fn greater_than() {
        let less = RustcVersion::new(1, 15, 1);
        assert!(RustcVersion::parse("1.16.0").unwrap() > less);
        assert!(RustcVersion::parse("1.16").unwrap() > less);
        assert!(RustcVersion::parse("2").unwrap() > less);
        assert!(RustcVersion::parse("1.15.2").unwrap() > less);
        assert!(
            RustcVersion::parse("1.0.0-beta").unwrap()
                > RustcVersion::Special(SpecialVersion::Alpha2)
        );
        assert!(
            RustcVersion::parse("1.0.0-beta").unwrap()
                > RustcVersion::Special(SpecialVersion::Alpha)
        );
        assert!(
            RustcVersion::parse("1.0.0-alpha.2").unwrap()
                > RustcVersion::Special(SpecialVersion::Alpha)
        );
        assert!(RustcVersion::parse("1.0.0-alpha.2").unwrap() > RustcVersion::new(0, 8, 0));
        assert!(
            RustcVersion::parse("1.45.2").unwrap() > RustcVersion::Special(SpecialVersion::Alpha2)
        );
    }

    #[test]
    fn edge_cases() {
        assert_eq!(RustcVersion::parse(""), Err(Error::EmptyVersionPart));
        assert_eq!(RustcVersion::parse(" "), Err(Error::EmptyVersionPart));
        assert_eq!(RustcVersion::parse("\t"), Err(Error::EmptyVersionPart));
        assert_eq!(RustcVersion::parse("1."), Err(Error::EmptyVersionPart));
        assert_eq!(RustcVersion::parse("1. "), Err(Error::EmptyVersionPart));
        assert_eq!(RustcVersion::parse("1.\t"), Err(Error::EmptyVersionPart));
        assert_eq!(RustcVersion::parse("1. \t.3"), Err(Error::EmptyVersionPart));
        assert_eq!(
            RustcVersion::parse(" 1  . \t 3.\r 5").unwrap(),
            RustcVersion::new(1, 3, 5)
        );
    }

    #[test]
    fn formatting() {
        extern crate alloc;
        use alloc::string::{String, ToString};
        assert_eq!(
            RustcVersion::new(1, 42, 28).to_string(),
            String::from("1.42.28")
        );
        assert_eq!(
            RustcVersion::Special(SpecialVersion::Alpha).to_string(),
            String::from("1.0.0-alpha")
        );
        assert_eq!(
            RustcVersion::Special(SpecialVersion::Alpha2).to_string(),
            String::from("1.0.0-alpha.2")
        );
        assert_eq!(
            RustcVersion::Special(SpecialVersion::Beta).to_string(),
            String::from("1.0.0-beta")
        );
    }

    #[test]
    fn too_many_elements() {
        assert_eq!(
            RustcVersion::parse("1.0.0.100"),
            Err(Error::TooManyElements)
        );
    }

    #[test]
    fn alpha_numeric_version() {
        assert_eq!(RustcVersion::parse("a.0.1"), Err(Error::ParseIntError));
        assert_eq!(RustcVersion::parse("2.x.1"), Err(Error::ParseIntError));
        assert_eq!(RustcVersion::parse("0.2.s"), Err(Error::NotASpecialVersion));
    }

    #[test]
    fn meets_full() {
        // Nothing was omitted
        assert!(RustcVersion::new(1, 2, 3).meets(RustcVersion::new(1, 2, 3)));
        assert!(RustcVersion::new(1, 2, 5).meets(RustcVersion::new(1, 2, 3)));
        assert!(RustcVersion::new(1, 3, 0).meets(RustcVersion::new(1, 2, 3)));
        assert!(!RustcVersion::new(2, 0, 0).meets(RustcVersion::new(1, 2, 3)));
        assert!(!RustcVersion::new(0, 9, 0).meets(RustcVersion::new(1, 0, 0)));

        assert!(RustcVersion::new(0, 2, 3).meets(RustcVersion::new(0, 2, 3)));
        assert!(RustcVersion::new(0, 2, 5).meets(RustcVersion::new(0, 2, 3)));
        assert!(!RustcVersion::new(0, 3, 0).meets(RustcVersion::new(0, 2, 3)));
        assert!(!RustcVersion::new(1, 0, 0).meets(RustcVersion::new(0, 2, 3)));

        assert!(RustcVersion::new(0, 0, 3).meets(RustcVersion::new(0, 0, 3)));
        assert!(!RustcVersion::new(0, 0, 5).meets(RustcVersion::new(0, 0, 3)));
        assert!(!RustcVersion::new(0, 1, 0).meets(RustcVersion::new(0, 0, 3)));

        assert!(RustcVersion::new(0, 0, 0).meets(RustcVersion::new(0, 0, 0)));
        assert!(!RustcVersion::new(0, 0, 1).meets(RustcVersion::new(0, 0, 0)));
    }

    #[test]
    fn meets_no_patch() {
        // Patch was omitted
        assert!(RustcVersion::new(1, 2, 0).meets(RustcVersion::parse("1.2").unwrap()));
        assert!(RustcVersion::new(1, 2, 5).meets(RustcVersion::parse("1.2").unwrap()));
        assert!(RustcVersion::new(1, 3, 0).meets(RustcVersion::parse("1.2").unwrap()));
        assert!(!RustcVersion::new(2, 0, 0).meets(RustcVersion::parse("1.2").unwrap()));
        assert!(!RustcVersion::new(0, 9, 0).meets(RustcVersion::parse("1.0").unwrap()));

        assert!(RustcVersion::new(0, 2, 0).meets(RustcVersion::parse("0.2").unwrap()));
        assert!(RustcVersion::new(0, 2, 5).meets(RustcVersion::parse("0.2").unwrap()));
        assert!(!RustcVersion::new(0, 3, 0).meets(RustcVersion::parse("0.2").unwrap()));
        assert!(!RustcVersion::new(1, 0, 0).meets(RustcVersion::parse("0.2").unwrap()));

        assert!(RustcVersion::new(0, 0, 0).meets(RustcVersion::parse("0.0").unwrap()));
        assert!(RustcVersion::new(0, 0, 5).meets(RustcVersion::parse("0.0").unwrap()));
        assert!(!RustcVersion::new(0, 1, 0).meets(RustcVersion::parse("0.0").unwrap()));
    }

    #[test]
    fn meets_no_minor() {
        // Minor was omitted
        assert!(RustcVersion::new(1, 0, 0).meets(RustcVersion::parse("1").unwrap()));
        assert!(RustcVersion::new(1, 3, 0).meets(RustcVersion::parse("1").unwrap()));
        assert!(!RustcVersion::new(2, 0, 0).meets(RustcVersion::parse("1").unwrap()));
        assert!(!RustcVersion::new(0, 9, 0).meets(RustcVersion::parse("1").unwrap()));

        assert!(RustcVersion::new(0, 0, 0).meets(RustcVersion::parse("0").unwrap()));
        assert!(RustcVersion::new(0, 0, 1).meets(RustcVersion::parse("0").unwrap()));
        assert!(RustcVersion::new(0, 2, 5).meets(RustcVersion::parse("0").unwrap()));
        assert!(!RustcVersion::new(1, 0, 0).meets(RustcVersion::parse("0").unwrap()));
    }

    #[test]
    fn meets_special() {
        assert!(RustcVersion::Special(SpecialVersion::Alpha)
            .meets(RustcVersion::Special(SpecialVersion::Alpha)));
        assert!(RustcVersion::Special(SpecialVersion::Alpha2)
            .meets(RustcVersion::Special(SpecialVersion::Alpha2)));
        assert!(RustcVersion::Special(SpecialVersion::Beta)
            .meets(RustcVersion::Special(SpecialVersion::Beta)));
        assert!(!RustcVersion::Special(SpecialVersion::Alpha)
            .meets(RustcVersion::Special(SpecialVersion::Alpha2)));
        assert!(!RustcVersion::Special(SpecialVersion::Alpha)
            .meets(RustcVersion::Special(SpecialVersion::Beta)));
        assert!(!RustcVersion::Special(SpecialVersion::Alpha2)
            .meets(RustcVersion::Special(SpecialVersion::Beta)));
        assert!(!RustcVersion::Special(SpecialVersion::Alpha).meets(RustcVersion::new(1, 0, 0)));
        assert!(!RustcVersion::Special(SpecialVersion::Alpha2).meets(RustcVersion::new(1, 0, 0)));
        assert!(!RustcVersion::Special(SpecialVersion::Beta).meets(RustcVersion::new(1, 0, 0)));
        assert!(!RustcVersion::new(1, 0, 0).meets(RustcVersion::Special(SpecialVersion::Alpha)));
        assert!(!RustcVersion::new(1, 0, 0).meets(RustcVersion::Special(SpecialVersion::Alpha2)));
        assert!(!RustcVersion::new(1, 0, 0).meets(RustcVersion::Special(SpecialVersion::Beta)));
    }

    #[test]
    #[should_panic(
        expected = "This function should never be called with `parts == 0` or `parts > 3`"
    )]
    fn omitted_parts_with_zero() {
        OmittedParts::from(0);
    }

    #[test]
    #[should_panic(
        expected = "This function should never be called with `parts == 0` or `parts > 3`"
    )]
    fn omitted_parts_with_four() {
        OmittedParts::from(4);
    }
}

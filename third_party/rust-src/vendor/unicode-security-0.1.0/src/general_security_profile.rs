//! Utilities for working with the [General Security Profile](https://www.unicode.org/reports/tr39/#General_Security_Profile)
//! for identifiers

use crate::tables::identifier;

pub use identifier::IdentifierType;

/// Methods for determining characters not restricted from use for identifiers.
pub trait GeneralSecurityProfile {
    /// Returns whether the character is not restricted from use for identifiers.
    fn identifier_allowed(self) -> bool;

    /// Returns the [identifier type](https://www.unicode.org/reports/tr39/#Identifier_Status_and_Type)
    fn identifier_type(self) -> Option<IdentifierType>;
}

impl GeneralSecurityProfile for char {
    #[inline]
    fn identifier_allowed(self) -> bool {
        identifier::identifier_status_allowed(self)
    }
    #[inline]
    fn identifier_type(self) -> Option<IdentifierType> {
        identifier::identifier_type(self)
    }
}

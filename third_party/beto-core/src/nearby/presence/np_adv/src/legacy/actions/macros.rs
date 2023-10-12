// Copyright 2022 Google LLC
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

/// Create a struct holding a `bool`.
macro_rules! boolean_element_struct {
    ($type_name:ident) => {
        #[derive(Debug)]
        #[allow(missing_docs)]
        pub struct $type_name {
            enabled: bool,
        }
    };
}

/// Create a `From<bool>` impl for a struct made with [boolean_element_struct].
macro_rules! boolean_element_struct_from_bool {
    ($type_name:ident) => {
        impl From<bool> for $type_name {
            fn from(value: bool) -> Self {
                $type_name { enabled: value }
            }
        }
    };
}

/// The guts of an ActionElement impl
macro_rules! boolean_element_action_element_impl_shared {
    ($type_name:ident, $index:expr) => {
        const HIGH_BIT_INDEX: u32 = $index;
        const BITS_LEN: u32 = 1;
        const ACTION_TYPE: $crate::legacy::actions::ActionType =
            $crate::legacy::actions::ActionType::$type_name;
    };
}

/// Create a struct w/ From<bool> and ActionElement impls.
/// Use `plaintext_only`, `ciphertext_only`, or `plaintext_and_ciphertext` to create appropriate
/// impls.
macro_rules! boolean_element {
    ($type_name:ident, $index:expr, plaintext_only) => {
        $crate::legacy::actions::macros::boolean_element_struct!($type_name);
        $crate::legacy::actions::macros::boolean_element_struct_from_bool!($type_name);

        impl $crate::legacy::actions::ActionElement for $type_name {
            $crate::legacy::actions::macros::boolean_element_action_element_impl_shared!(
                $type_name, $index
            );

            fn supports_flavor(flavor: $crate::legacy::PacketFlavorEnum) -> bool {
                match flavor {
                    $crate::legacy::PacketFlavorEnum::Plaintext => true,
                    $crate::legacy::PacketFlavorEnum::Ciphertext => false,
                }
            }
        }

        $crate::legacy::actions::macros::boolean_element_to_plaintext_element!($type_name);
    };
    ($type_name:ident, $index:expr, ciphertext_only) => {
        $crate::legacy::actions::macros::boolean_element_struct!($type_name);
        $crate::legacy::actions::macros::boolean_element_struct_from_bool!($type_name);

        impl $crate::legacy::actions::ActionElement for $type_name {
            $crate::legacy::actions::macros::boolean_element_action_element_impl_shared!(
                $type_name, $index
            );

            fn supports_flavor(flavor: $crate::legacy::PacketFlavorEnum) -> bool {
                match flavor {
                    $crate::legacy::PacketFlavorEnum::Plaintext => false,
                    $crate::legacy::PacketFlavorEnum::Ciphertext => true,
                }
            }
        }

        $crate::legacy::actions::macros::boolean_element_to_encrypted_element!($type_name);
    };
    ($type_name:ident, $index:expr, plaintext_and_ciphertext) => {
        $crate::legacy::actions::macros::boolean_element_struct!($type_name);
        $crate::legacy::actions::macros::boolean_element_struct_from_bool!($type_name);

        impl $crate::legacy::actions::ActionElement for $type_name {
            $crate::legacy::actions::macros::boolean_element_action_element_impl_shared!(
                $type_name, $index
            );

            fn supports_flavor(flavor: $crate::legacy::PacketFlavorEnum) -> bool {
                match flavor {
                    $crate::legacy::PacketFlavorEnum::Plaintext => true,
                    $crate::legacy::PacketFlavorEnum::Ciphertext => true,
                }
            }
        }

        $crate::legacy::actions::macros::boolean_element_to_plaintext_element!($type_name);
        $crate::legacy::actions::macros::boolean_element_to_encrypted_element!($type_name);
    };
}

/// Create a [ToActionElement<Encrypted>] impl with the given index and length 1.
macro_rules! boolean_element_to_encrypted_element {
    ( $type_name:ident) => {
        impl $crate::legacy::actions::ToActionElement<$crate::legacy::Ciphertext> for $type_name {
            fn bits(&self) -> u8 {
                self.enabled as u8
            }
        }
    };
}

/// Create a [ToActionElement<Plaintext>] impl with the given index and length 1.
macro_rules! boolean_element_to_plaintext_element {
    ( $type_name:ident) => {
        impl $crate::legacy::actions::ToActionElement<$crate::legacy::Plaintext> for $type_name {
            fn bits(&self) -> u8 {
                self.enabled as u8
            }
        }
    };
}

// expose macros to the rest of the crate without macro_export's behavior of exporting at crate root
pub(crate) use {
    boolean_element, boolean_element_action_element_impl_shared, boolean_element_struct,
    boolean_element_struct_from_bool, boolean_element_to_encrypted_element,
    boolean_element_to_plaintext_element,
};

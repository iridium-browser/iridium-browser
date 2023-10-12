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

//! V0 data elements and core trait impls.
use crate::legacy::{
    de_type::{DataElementType, PlainDataElementType},
    serialize::{DataElementBundle, ToDataElementBundle},
    PacketFlavor, PacketFlavorEnum,
};
use crate::shared_data::*;

/// Core behavior for data elements.
///
/// See also [ToDataElementBundle] for flavor-specific, infallible serialization.
pub trait DataElement: Sized {
    /// The corresponding DataElementType variant.
    const DE_TYPE_VARIANT: DataElementType;

    /// Return true if the DE supports serialization and deserialization for the provided flavor.
    fn supports_flavor(flavor: PacketFlavorEnum) -> bool;

    /// Returns `Err` if the flavor is not supported or if parsing fails.
    ///
    /// `<F>` is the flavor of the packet being deserialized.
    fn deserialize<F: PacketFlavor>(
        de_contents: &[u8],
    ) -> Result<Self, DataElementDeserializeError>;
}

/// Errors possible when deserializing a DE
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum DataElementDeserializeError {
    /// The data element doesn't support the [PacketFlavor] of the advertisement packet.
    FlavorNotSupported {
        /// The DE type attempting to be deserialized
        de_type: DataElementType,
        /// The flavor that was not supported
        flavor: PacketFlavorEnum,
    },
    /// The data element couldn't be deserialized from the supplied data.
    DeserializeError {
        /// The DE type attempting to be deserialized
        de_type: DataElementType,
    },
}

/// Data element holding a [TxPower].
#[derive(Debug, PartialEq, Eq)]
pub struct TxPowerDataElement {
    tx_power: TxPower,
}

impl TxPowerDataElement {
    /// Gets the underlying Tx Power value
    pub fn tx_power_value(&self) -> i8 {
        self.tx_power.as_i8()
    }
}

impl From<TxPower> for TxPowerDataElement {
    fn from(tx_power: TxPower) -> Self {
        Self { tx_power }
    }
}

impl<F: PacketFlavor> ToDataElementBundle<F> for TxPowerDataElement {
    fn to_de_bundle(&self) -> DataElementBundle<F> {
        let tx_power = self.tx_power.as_i8();
        DataElementBundle::try_from(
            PlainDataElementType::TxPower,
            tx_power.to_be_bytes().as_slice(),
        )
        .expect("Length < max DE size")
    }
}

impl DataElement for TxPowerDataElement {
    const DE_TYPE_VARIANT: DataElementType = DataElementType::TxPower;

    fn supports_flavor(flavor: PacketFlavorEnum) -> bool {
        match flavor {
            PacketFlavorEnum::Plaintext => true,
            PacketFlavorEnum::Ciphertext => true,
        }
    }
    fn deserialize<F: PacketFlavor>(
        de_contents: &[u8],
    ) -> Result<Self, DataElementDeserializeError> {
        de_contents
            .try_into()
            .ok()
            .and_then(|arr: [u8; 1]| TxPower::try_from(i8::from_be_bytes(arr)).ok())
            .map(|tx_power| Self { tx_power })
            .ok_or(DataElementDeserializeError::DeserializeError {
                de_type: DataElementType::TxPower,
            })
    }
}

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

//! Data types shared between V0 and V1 advertisements

/// Power in dBm, calibrated as per
/// [Eddystone](https://github.com/google/eddystone/tree/master/eddystone-uid#tx-power)
#[derive(Debug, PartialEq, Eq, Clone)]
pub struct TxPower {
    /// Power in `[-100, 20]`
    power: i8,
}

impl TxPower {
    /// Power in `[-100, 20]`
    pub fn as_i8(&self) -> i8 {
        self.power
    }
}

impl TryFrom<i8> for TxPower {
    type Error = TxPowerOutOfRange;

    fn try_from(value: i8) -> Result<Self, Self::Error> {
        if !(-100..=20).contains(&value) {
            Err(TxPowerOutOfRange)
        } else {
            Ok(TxPower { power: value })
        }
    }
}

/// Tx power was out of the valid range `[-100, 20]`.
#[derive(Debug)]
pub struct TxPowerOutOfRange;

/// MDP's context sync number, used to inform other devices that they have stale context.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct ContextSyncSeqNum {
    /// 4-bit sequence
    num: u8,
}

impl ContextSyncSeqNum {
    /// Returns the context seq num as a u8
    pub fn as_u8(&self) -> u8 {
        self.num
    }
}

impl TryFrom<u8> for ContextSyncSeqNum {
    type Error = ContextSyncSeqNumOutOfRange;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        // must only have low nibble
        if value > 0x0F {
            Err(ContextSyncSeqNumOutOfRange)
        } else {
            Ok(ContextSyncSeqNum { num: value })
        }
    }
}

/// Seq num must be in `[0-15]`.
#[derive(Debug)]
pub struct ContextSyncSeqNumOutOfRange;

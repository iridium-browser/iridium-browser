// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use crate::NoPoisonMutex;

pub struct Mutex<T>(std::sync::Mutex<T>);

impl<T> NoPoisonMutex<T> for Mutex<T> {
    type MutexGuard<'a> = std::sync::MutexGuard<'a, T> where T: 'a;

    fn lock(&self) -> Self::MutexGuard<'_> {
        self.0.lock().unwrap_or_else(|poison| poison.into_inner())
    }

    fn try_lock(&self) -> Option<Self::MutexGuard<'_>> {
        match self.0.try_lock() {
            Ok(guard) => Some(guard),
            Err(std::sync::TryLockError::Poisoned(guard)) => Some(guard.into_inner()),
            Err(std::sync::TryLockError::WouldBlock) => None,
        }
    }

    fn new(value: T) -> Self {
        Self(std::sync::Mutex::new(value))
    }
}

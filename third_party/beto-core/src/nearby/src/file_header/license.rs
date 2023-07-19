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

//! Support for license-oriented usage of `file_header`.
use super::*;
use chrono::Datelike as _;

/// The Apache 2 license for the current year and provided `copyright_holder`.
pub fn apache_2(copyright_holder: &str) -> Header<impl HeaderChecker> {
    Header::new(
        asl2_checker(),
        format!(
            r#"Copyright {} {}

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License."#,
            chrono::prelude::Utc::now().year(),
            copyright_holder
        ),
    )
}

pub(crate) fn asl2_checker() -> impl HeaderChecker {
    SingleLineChecker::new("Licensed under the Apache License, Version 2.0".to_string(), 10)
}

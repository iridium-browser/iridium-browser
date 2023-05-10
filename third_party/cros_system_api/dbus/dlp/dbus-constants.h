// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SYSTEM_API_DBUS_DLP_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DLP_DBUS_CONSTANTS_H_

namespace dlp {
// DLP Daemon:
constexpr char kDlpInterface[] = "org.chromium.Dlp";
constexpr char kDlpServicePath[] = "/org/chromium/Dlp";
constexpr char kDlpServiceName[] = "org.chromium.Dlp";

// DLP Daemon methods:
constexpr char kSetDlpFilesPolicyMethod[] = "SetDlpFilesPolicy";
constexpr char kAddFileMethod[] = "AddFile";
constexpr char kRequestFileAccessMethod[] = "RequestFileAccess";
constexpr char kGetFilesSourcesMethod[] = "GetFilesSources";
constexpr char kCheckFilesTransferMethod[] = "CheckFilesTransfer";

// Errors returned by DLP Daemon mathods:
constexpr char kErrorFailedToCreatePipe[] = "FailedToCreatePipe";

// DLP service in Chrome:
constexpr char kDlpFilesPolicyServiceName[] =
    "org.chromium.DlpFilesPolicyService";
constexpr char kDlpFilesPolicyServicePath[] =
    "/org/chromium/DlpFilesPolicyService";
constexpr char kDlpFilesPolicyServiceInterface[] =
    "org.chromium.DlpFilesPolicyService";

// DLP service in Chrome methods:
constexpr char kDlpFilesPolicyServiceIsRestrictedMethod[] = "IsRestricted";
constexpr char kDlpFilesPolicyServiceIsDlpPolicyMatchedMethod[] =
    "IsDlpPolicyMatched";
constexpr char kDlpFilesPolicyServiceIsFilesTransferRestrictedMethod[] =
    "IsFilesTransferRestricted";

}  // namespace dlp
#endif  // SYSTEM_API_DBUS_DLP_DBUS_CONSTANTS_H_

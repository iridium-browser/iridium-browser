// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_enums.h"

#include <algorithm>
#include <cstring>

namespace {

// An array with all strings sorted in ASCII order
const char* const kAllStrings[] = {
    "1-to-n-order",
    "1.0",
    "1.1",
    "2.0",
    "2.1",
    "2.2",
    "Acknowledge-Document",
    "Acknowledge-Identify-Printer",
    "Acknowledge-Job",
    "Activate-Printer",
    "Add-Document-Images",
    "Allocate-Printer-Resources",
    "CUPS-Accept-Jobs",
    "CUPS-Add-Modify-Class",
    "CUPS-Add-Modify-Printer",
    "CUPS-Authenticate-Job",
    "CUPS-Create-Local-Printer",
    "CUPS-Delete-Class",
    "CUPS-Delete-Printer",
    "CUPS-Get-Classes",
    "CUPS-Get-Default",
    "CUPS-Get-Devices",
    "CUPS-Get-Document",
    "CUPS-Get-PPD",
    "CUPS-Get-PPDs",
    "CUPS-Get-Printers",
    "CUPS-Move-Job",
    "CUPS-Reject-Jobs",
    "CUPS-Set-Default",
    "Cancel-Current-Job",
    "Cancel-Document",
    "Cancel-Job",
    "Cancel-Jobs",
    "Cancel-My-Jobs",
    "Cancel-Resource",
    "Cancel-Subscription",
    "Close-Job",
    "Create-Job",
    "Create-Job-Subscriptions",
    "Create-Printer",
    "Create-Printer-Subscriptions",
    "Create-Resource",
    "Create-Resource-Subscriptions",
    "Create-System-Subscriptions",
    "Deactivate-Printer",
    "Deallocate-Printer-Resources",
    "Delete-Document",
    "Delete-Printer",
    "Deregister-Output-Device",
    "Disable-All-Printers",
    "Disable-Printer",
    "Enable-All-Printers",
    "Enable-Printer",
    "Fetch-Document",
    "Fetch-Job",
    "Get-Document-Attributes",
    "Get-Documents",
    "Get-Job-Attributes",
    "Get-Jobs",
    "Get-Next-Document-Data",
    "Get-Notifications",
    "Get-Output-Device-Attributes",
    "Get-Printer-Attributes",
    "Get-Printer-Resources",
    "Get-Printer-Supported-Values",
    "Get-Printers",
    "Get-Resource-Attributes",
    "Get-Resources",
    "Get-Subscription-Attributes",
    "Get-Subscriptions",
    "Get-System-Attributes",
    "Get-System-Supported-Values",
    "Get-User-Printer-Attributes",
    "Hold-Job",
    "Hold-New-Jobs",
    "Identify-Printer",
    "Install-Resource",
    "Pause-All-Printers",
    "Pause-All-Printers-After-Current-Job",
    "Pause-Printer",
    "Pause-Printer-After-Current-Job",
    "Print-Job",
    "Print-URI",
    "Promote-Job",
    "Purge-Jobs",
    "Register-Output-Device",
    "Release-Held-New-Jobs",
    "Release-Job",
    "Renew-Subscription",
    "Reprocess-Job",
    "Restart-Job",
    "Restart-One-Printer",
    "Restart-Printer",
    "Restart-System",
    "Resubmit-Job",
    "Resume-All-Printers",
    "Resume-Job",
    "Resume-Printer",
    "Schedule-Job-After",
    "Send-Document",
    "Send-Resource-Data",
    "Send-URI",
    "Set-Document-Attributes",
    "Set-Job-Attributes",
    "Set-Printer-Attributes",
    "Set-Resource-Attributes",
    "Set-System-Attributes",
    "Shutdown-All-Printers",
    "Shutdown-One-Printer",
    "Shutdown-Printer",
    "Startup-All-Printers",
    "Startup-One-Printer",
    "Startup-Printer",
    "Suspend-Current-Job",
    "Update-Active-Jobs",
    "Update-Document-Status",
    "Update-Job-Status",
    "Update-Output-Device-Attributes",
    "Validate-Document",
    "Validate-Job",
    "a",
    "a-translucent",
    "a-transparent",
    "a-white",
    "abort-job",
    "aborted",
    "aborted-by-system",
    "absolute",
    "account-authorization-failed",
    "account-closed",
    "account-info-needed",
    "account-limit-reached",
    "adhesive",
    "adobe-1.3",
    "adobe-1.4",
    "adobe-1.5",
    "adobe-1.6",
    "adobe-rgb_16",
    "adobe-rgb_8",
    "after-documents",
    "after-job",
    "after-sets",
    "after-sheets",
    "alert-removal-of-binary-change-entry",
    "all",
    "alternate",
    "alternate-roll",
    "aluminum",
    "always",
    "antique",
    "arch-a",
    "arch-a-translucent",
    "arch-a-transparent",
    "arch-a-white",
    "arch-axsynchro-translucent",
    "arch-axsynchro-transparent",
    "arch-axsynchro-white",
    "arch-b",
    "arch-b-translucent",
    "arch-b-transparent",
    "arch-b-white",
    "arch-bxsynchro-translucent",
    "arch-bxsynchro-transparent",
    "arch-bxsynchro-white",
    "arch-c",
    "arch-c-translucent",
    "arch-c-transparent",
    "arch-c-white",
    "arch-cxsynchro-translucent",
    "arch-cxsynchro-transparent",
    "arch-cxsynchro-white",
    "arch-d",
    "arch-d-translucent",
    "arch-d-transparent",
    "arch-d-white",
    "arch-dxsynchro-translucent",
    "arch-dxsynchro-transparent",
    "arch-dxsynchro-white",
    "arch-e",
    "arch-e-translucent",
    "arch-e-transparent",
    "arch-e-white",
    "arch-exsynchro-translucent",
    "arch-exsynchro-transparent",
    "arch-exsynchro-white",
    "archival",
    "archival-glossy",
    "archival-matte",
    "archival-semi-gloss",
    "asme_f_28x40in",
    "attempted",
    "attributes-charset",
    "attributes-natural-language",
    "auth-info",
    "auth-info-required",
    "auto",
    "auto-fit",
    "auto-fixed-size-translucent",
    "auto-fixed-size-transparent",
    "auto-fixed-size-white",
    "auto-monochrome",
    "auto-synchro-translucent",
    "auto-synchro-transparent",
    "auto-synchro-white",
    "auto-translucent",
    "auto-transparent",
    "auto-white",
    "automatic",
    "axsynchro-translucent",
    "axsynchro-transparent",
    "axsynchro-white",
    "b",
    "b-translucent",
    "b-transparent",
    "b-white",
    "back",
    "back-print-film",
    "bale",
    "baling",
    "baling-type",
    "baling-type-supported",
    "baling-when",
    "baling-when-supported",
    "band",
    "bander-added",
    "bander-almost-empty",
    "bander-almost-full",
    "bander-at-limit",
    "bander-closed",
    "bander-configuration-change",
    "bander-cover-closed",
    "bander-cover-open",
    "bander-empty",
    "bander-full",
    "bander-interlock-closed",
    "bander-interlock-open",
    "bander-jam",
    "bander-life-almost-over",
    "bander-life-over",
    "bander-memory-exhausted",
    "bander-missing",
    "bander-motor-failure",
    "bander-near-limit",
    "bander-offline",
    "bander-opened",
    "bander-over-temperature",
    "bander-power-saver",
    "bander-recoverable-failure",
    "bander-recoverable-storage",
    "bander-removed",
    "bander-resource-added",
    "bander-resource-removed",
    "bander-thermistor-failure",
    "bander-timing-failure",
    "bander-turned-off",
    "bander-turned-on",
    "bander-under-temperature",
    "bander-unrecoverable-failure",
    "bander-unrecoverable-storage-error",
    "bander-warming-up",
    "basic",
    "bi-level",
    "bind",
    "bind-bottom",
    "bind-left",
    "bind-right",
    "bind-top",
    "binder-added",
    "binder-almost-empty",
    "binder-almost-full",
    "binder-at-limit",
    "binder-closed",
    "binder-configuration-change",
    "binder-cover-closed",
    "binder-cover-open",
    "binder-empty",
    "binder-full",
    "binder-interlock-closed",
    "binder-interlock-open",
    "binder-jam",
    "binder-life-almost-over",
    "binder-life-over",
    "binder-memory-exhausted",
    "binder-missing",
    "binder-motor-failure",
    "binder-near-limit",
    "binder-offline",
    "binder-opened",
    "binder-over-temperature",
    "binder-power-saver",
    "binder-recoverable-failure",
    "binder-recoverable-storage",
    "binder-removed",
    "binder-resource-added",
    "binder-resource-removed",
    "binder-thermistor-failure",
    "binder-timing-failure",
    "binder-turned-off",
    "binder-turned-on",
    "binder-under-temperature",
    "binder-unrecoverable-failure",
    "binder-unrecoverable-storage-error",
    "binder-warming-up",
    "binding",
    "binding-reference-edge",
    "binding-reference-edge-supported",
    "binding-type",
    "binding-type-supported",
    "black",
    "black_1",
    "black_16",
    "black_8",
    "blank",
    "blue",
    "bond",
    "booklet-maker",
    "both",
    "both-sheets",
    "bottom",
    "brown",
    "buff",
    "bxsynchro-translucent",
    "bxsynchro-transparent",
    "bxsynchro-white",
    "by-pass-tray",
    "c",
    "c-translucent",
    "c-transparent",
    "c-white",
    "calendared",
    "camera-failure",
    "cancel-job",
    "canceled",
    "cardboard",
    "cardstock",
    "cd",
    "center",
    "certificate",
    "chamber-cooling",
    "chamber-failure",
    "chamber-heating",
    "chamber-temperature-high",
    "chamber-temperature-low",
    "charset-configured",
    "charset-supported",
    "choice_iso_a4_210x297mm_na_letter_8.5x11in",
    "cleaner-life-almost-over",
    "cleaner-life-over",
    "clear-black",
    "clear-blue",
    "clear-brown",
    "clear-buff",
    "clear-cyan",
    "clear-gold",
    "clear-goldenrod",
    "clear-gray",
    "clear-green",
    "clear-ivory",
    "clear-magenta",
    "clear-multi-color",
    "clear-mustard",
    "clear-orange",
    "clear-pink",
    "clear-red",
    "clear-silver",
    "clear-turquoise",
    "clear-violet",
    "clear-white",
    "clear-yellow",
    "client-error-account-authorization-failed",
    "client-error-account-closed",
    "client-error-account-info-needed",
    "client-error-account-limit-reached",
    "client-error-attributes-not-settable",
    "client-error-attributes-or-values-not-supported",
    "client-error-bad-request",
    "client-error-charset-not-supported",
    "client-error-compression-error",
    "client-error-compression-not-supported",
    "client-error-conflicting-attributes",
    "client-error-document-access-error",
    "client-error-document-format-error",
    "client-error-document-format-not-supported",
    "client-error-document-password-error",
    "client-error-document-permission-error",
    "client-error-document-security-error",
    "client-error-document-unprintable-error",
    "client-error-forbidden",
    "client-error-gone",
    "client-error-ignored-all-subscriptions",
    "client-error-not-authenticated",
    "client-error-not-authorized",
    "client-error-not-fetchable",
    "client-error-not-found",
    "client-error-not-possible",
    "client-error-request-entity-too-large",
    "client-error-request-value-too-long",
    "client-error-timeout",
    "client-error-too-many-subscriptions",
    "client-error-uri-scheme-not-supported",
    "cmyk_16",
    "cmyk_8",
    "coarse",
    "coat",
    "coating",
    "coating-sides",
    "coating-sides-supported",
    "coating-type",
    "coating-type-supported",
    "collated",
    "collated-documents",
    "color",
    "color-supported",
    "comb",
    "completed",
    "compress",
    "compression",
    "compression-error",
    "compression-supported",
    "configuration-change",
    "conflicting-attributes",
    "connected-to-destination",
    "connecting-to-destination",
    "connecting-to-device",
    "contact-name",
    "contact-uri",
    "contact-vcard",
    "continue-job",
    "continuous",
    "continuous-long",
    "continuous-short",
    "copies",
    "copies-actual",
    "copies-default",
    "copies-supported",
    "corrugated-board",
    "cover",
    "cover-back",
    "cover-back-actual",
    "cover-back-default",
    "cover-back-supported",
    "cover-front",
    "cover-front-actual",
    "cover-front-default",
    "cover-front-supported",
    "cover-open",
    "cover-type",
    "covering",
    "covering-name",
    "covering-name-supported",
    "crimp",
    "current-page-order",
    "custom1",
    "custom10",
    "custom2",
    "custom3",
    "custom4",
    "custom5",
    "custom6",
    "custom7",
    "custom8",
    "custom9",
    "cxsynchro-translucent",
    "cxsynchro-transparent",
    "cxsynchro-white",
    "cyan",
    "d",
    "d-translucent",
    "d-transparent",
    "d-white",
    "dark-blue",
    "dark-brown",
    "dark-buff",
    "dark-cyan",
    "dark-gold",
    "dark-goldenrod",
    "dark-gray",
    "dark-green",
    "dark-ivory",
    "dark-magenta",
    "dark-mustard",
    "dark-orange",
    "dark-pink",
    "dark-red",
    "dark-silver",
    "dark-turquoise",
    "dark-violet",
    "dark-yellow",
    "date-time-at-completed",
    "date-time-at-creation",
    "date-time-at-processing",
    "day-time",
    "deactivated",
    "default",
    "deflate",
    "deleted",
    "destination-uri-failed",
    "detailed-status-message",
    "developer-empty",
    "developer-low",
    "device-service-count",
    "device-uri",
    "device-uuid",
    "device10_16",
    "device10_8",
    "device11_16",
    "device11_8",
    "device12_16",
    "device12_8",
    "device13_16",
    "device13_8",
    "device14_16",
    "device14_8",
    "device15_16",
    "device15_8",
    "device1_16",
    "device1_8",
    "device2_16",
    "device2_8",
    "device3_16",
    "device3_8",
    "device4_16",
    "device4_8",
    "device5_16",
    "device5_8",
    "device6_16",
    "device6_8",
    "device7_16",
    "device7_8",
    "device8_16",
    "device8_8",
    "device9_16",
    "device9_8",
    "die-cutter-added",
    "die-cutter-almost-empty",
    "die-cutter-almost-full",
    "die-cutter-at-limit",
    "die-cutter-closed",
    "die-cutter-configuration-change",
    "die-cutter-cover-closed",
    "die-cutter-cover-open",
    "die-cutter-empty",
    "die-cutter-full",
    "die-cutter-interlock-closed",
    "die-cutter-interlock-open",
    "die-cutter-jam",
    "die-cutter-life-almost-over",
    "die-cutter-life-over",
    "die-cutter-memory-exhausted",
    "die-cutter-missing",
    "die-cutter-motor-failure",
    "die-cutter-near-limit",
    "die-cutter-offline",
    "die-cutter-opened",
    "die-cutter-over-temperature",
    "die-cutter-power-saver",
    "die-cutter-recoverable-failure",
    "die-cutter-recoverable-storage",
    "die-cutter-removed",
    "die-cutter-resource-added",
    "die-cutter-resource-removed",
    "die-cutter-thermistor-failure",
    "die-cutter-timing-failure",
    "die-cutter-turned-off",
    "die-cutter-turned-on",
    "die-cutter-under-temperature",
    "die-cutter-unrecoverable-failure",
    "die-cutter-unrecoverable-storage-error",
    "die-cutter-warming-up",
    "digest",
    "digital-signature-did-not-verify",
    "digital-signature-type-not-supported",
    "disc",
    "disc-glossy",
    "disc-high-gloss",
    "disc-matte",
    "disc-satin",
    "disc-semi-gloss",
    "display",
    "document-access-error",
    "document-attributes",
    "document-charset-default",
    "document-charset-supplied",
    "document-charset-supported",
    "document-completed",
    "document-config-changed",
    "document-copies",
    "document-created",
    "document-description",
    "document-digital-signature",
    "document-digital-signature-default",
    "document-digital-signature-supported",
    "document-fetchable",
    "document-format",
    "document-format-default",
    "document-format-details-default",
    "document-format-details-supplied",
    "document-format-details-supported",
    "document-format-device-id",
    "document-format-error",
    "document-format-ready",
    "document-format-supplied",
    "document-format-supported",
    "document-format-varying-attributes",
    "document-format-version",
    "document-format-version-default",
    "document-format-version-supplied",
    "document-format-version-supported",
    "document-message-supplied",
    "document-metadata",
    "document-name",
    "document-name-supplied",
    "document-natural-language",
    "document-natural-language-default",
    "document-natural-language-supplied",
    "document-natural-language-supported",
    "document-number",
    "document-numbers",
    "document-object",
    "document-password-error",
    "document-password-supported",
    "document-permission-error",
    "document-security-error",
    "document-source-application-name",
    "document-source-application-version",
    "document-source-os-name",
    "document-source-os-version",
    "document-state-changed",
    "document-stopped",
    "document-template",
    "document-unprintable-error",
    "document-uri",
    "domain",
    "door-open",
    "double-wall",
    "draft",
    "draw-line",
    "dry-film",
    "dss",
    "dvd",
    "dxsynchro-translucent",
    "dxsynchro-transparent",
    "dxsynchro-white",
    "e",
    "e-translucent",
    "e-transparent",
    "e-white",
    "edge-stitch",
    "edge-stitch-bottom",
    "edge-stitch-left",
    "edge-stitch-right",
    "edge-stitch-top",
    "embossing-foil",
    "end-board",
    "end-sheet",
    "envelope",
    "envelope-archival",
    "envelope-bond",
    "envelope-coated",
    "envelope-cotton",
    "envelope-fine",
    "envelope-heavyweight",
    "envelope-inkjet",
    "envelope-lightweight",
    "envelope-plain",
    "envelope-preprinted",
    "envelope-window",
    "errors-count",
    "errors-detected",
    "evening",
    "event-notification-attributes",
    "executive",
    "executive-white",
    "exsynchro-translucent",
    "exsynchro-transparent",
    "exsynchro-white",
    "extruder-cooling",
    "extruder-failure",
    "extruder-heating",
    "extruder-jam",
    "extruder-temperature-high",
    "extruder-temperature-low",
    "f",
    "fabric",
    "fabric-archival",
    "fabric-glossy",
    "fabric-high-gloss",
    "fabric-matte",
    "fabric-semi-gloss",
    "fabric-waterproof",
    "face-down",
    "face-up",
    "fan-failure",
    "faxout",
    "feed-orientation",
    "feed-orientation-supported",
    "fetchable",
    "fill",
    "film",
    "fine",
    "finishing-template",
    "finishing-template-supported",
    "finishings",
    "finishings-col",
    "finishings-col-actual",
    "finishings-col-database",
    "finishings-col-default",
    "finishings-col-ready",
    "finishings-default",
    "finishings-ready",
    "finishings-supported",
    "first-print-stream-page",
    "first-printer-name",
    "fit",
    "flash",
    "flat",
    "flexo-base",
    "flexo-photo-polymer",
    "flipped",
    "flute",
    "foil",
    "fold",
    "fold-accordion",
    "fold-double-gate",
    "fold-engineering-z",
    "fold-gate",
    "fold-half",
    "fold-half-z",
    "fold-left-gate",
    "fold-letter",
    "fold-parallel",
    "fold-poster",
    "fold-right-gate",
    "fold-z",
    "folder-added",
    "folder-almost-empty",
    "folder-almost-full",
    "folder-at-limit",
    "folder-closed",
    "folder-configuration-change",
    "folder-cover-closed",
    "folder-cover-open",
    "folder-empty",
    "folder-full",
    "folder-interlock-closed",
    "folder-interlock-open",
    "folder-jam",
    "folder-life-almost-over",
    "folder-life-over",
    "folder-memory-exhausted",
    "folder-missing",
    "folder-motor-failure",
    "folder-near-limit",
    "folder-offline",
    "folder-opened",
    "folder-over-temperature",
    "folder-power-saver",
    "folder-recoverable-failure",
    "folder-recoverable-storage",
    "folder-removed",
    "folder-resource-added",
    "folder-resource-removed",
    "folder-thermistor-failure",
    "folder-timing-failure",
    "folder-turned-off",
    "folder-turned-on",
    "folder-under-temperature",
    "folder-unrecoverable-failure",
    "folder-unrecoverable-storage-error",
    "folder-warming-up",
    "folding",
    "folding-direction",
    "folding-direction-supported",
    "folding-offset",
    "folding-offset-supported",
    "folding-reference-edge",
    "folding-reference-edge-supported",
    "folio",
    "folio-white",
    "font-name-requested",
    "font-name-requested-default",
    "font-name-requested-supported",
    "font-size-requested",
    "font-size-requested-default",
    "font-size-requested-supported",
    "force-front-side",
    "force-front-side-actual",
    "front",
    "full",
    "full-cut-tabs",
    "fuser-over-temp",
    "fuser-under-temp",
    "general",
    "generated-natural-language-supported",
    "glass",
    "glass-colored",
    "glass-opaque",
    "glass-surfaced",
    "glass-textured",
    "glossy",
    "gold",
    "goldenrod",
    "graphic",
    "gravure-cylinder",
    "gray",
    "green",
    "group",
    "guaranteed",
    "gzip",
    "hagaki",
    "heavyweight",
    "high",
    "high-gloss",
    "highlight",
    "hold-job",
    "hold-new-jobs",
    "icc-color-matching",
    "identify-actions",
    "identify-actions-default",
    "identify-actions-supported",
    "identify-printer-requested",
    "idle",
    "image-setter-paper",
    "imaging-cylinder",
    "imposition-template",
    "impressions-completed-current-copy",
    "imprinter-added",
    "imprinter-almost-empty",
    "imprinter-almost-full",
    "imprinter-at-limit",
    "imprinter-closed",
    "imprinter-configuration-change",
    "imprinter-cover-closed",
    "imprinter-cover-open",
    "imprinter-empty",
    "imprinter-full",
    "imprinter-interlock-closed",
    "imprinter-interlock-open",
    "imprinter-jam",
    "imprinter-life-almost-over",
    "imprinter-life-over",
    "imprinter-memory-exhausted",
    "imprinter-missing",
    "imprinter-motor-failure",
    "imprinter-near-limit",
    "imprinter-offline",
    "imprinter-opened",
    "imprinter-over-temperature",
    "imprinter-power-saver",
    "imprinter-recoverable-failure",
    "imprinter-recoverable-storage",
    "imprinter-removed",
    "imprinter-resource-added",
    "imprinter-resource-removed",
    "imprinter-thermistor-failure",
    "imprinter-timing-failure",
    "imprinter-turned-off",
    "imprinter-turned-on",
    "imprinter-under-temperature",
    "imprinter-unrecoverable-failure",
    "imprinter-unrecoverable-storage-error",
    "imprinter-warming-up",
    "indefinite",
    "infrastructure-printer",
    "input-cannot-feed-size-selected",
    "input-manual-input-request",
    "input-media-color-change",
    "input-media-form-parts-change",
    "input-media-size-change",
    "input-media-type-change",
    "input-media-weight-change",
    "input-orientation-requested",
    "input-quality",
    "input-sides",
    "input-tray-elevation-failure",
    "input-tray-missing",
    "input-tray-position-failure",
    "insert-after-page-number",
    "insert-after-page-number-supported",
    "insert-count",
    "insert-count-supported",
    "insert-sheet",
    "insert-sheet-actual",
    "insert-sheet-default",
    "inserter-added",
    "inserter-almost-empty",
    "inserter-almost-full",
    "inserter-at-limit",
    "inserter-closed",
    "inserter-configuration-change",
    "inserter-cover-closed",
    "inserter-cover-open",
    "inserter-empty",
    "inserter-full",
    "inserter-interlock-closed",
    "inserter-interlock-open",
    "inserter-jam",
    "inserter-life-almost-over",
    "inserter-life-over",
    "inserter-memory-exhausted",
    "inserter-missing",
    "inserter-motor-failure",
    "inserter-near-limit",
    "inserter-offline",
    "inserter-opened",
    "inserter-over-temperature",
    "inserter-power-saver",
    "inserter-recoverable-failure",
    "inserter-recoverable-storage",
    "inserter-removed",
    "inserter-resource-added",
    "inserter-resource-removed",
    "inserter-thermistor-failure",
    "inserter-timing-failure",
    "inserter-turned-off",
    "inserter-turned-on",
    "inserter-under-temperature",
    "inserter-unrecoverable-failure",
    "inserter-unrecoverable-storage-error",
    "inserter-warming-up",
    "interlock-closed",
    "interlock-open",
    "interpreter-cartridge-added",
    "interpreter-cartridge-deleted",
    "interpreter-complex-page-encountered",
    "interpreter-memory-decrease",
    "interpreter-memory-increase",
    "interpreter-resource-added",
    "interpreter-resource-deleted",
    "interpreter-resource-unavailable",
    "invoice",
    "invoice-white",
    "inward",
    "ipp-3d",
    "ipp-attribute-fidelity",
    "ipp-everywhere",
    "ipp-features-supported",
    "ipp-versions-supported",
    "ippget",
    "ippget-event-life",
    "iso-15930-1_2001",
    "iso-15930-3_2002",
    "iso-15930-4_2003",
    "iso-15930-6_2003",
    "iso-15930-7_2010",
    "iso-15930-8_2010",
    "iso-16612-2_2010",
    "iso-19005-1_2005",
    "iso-19005-2_2011",
    "iso-19005-3_2012",
    "iso-32000-1_2008",
    "iso-a0",
    "iso-a0-translucent",
    "iso-a0-transparent",
    "iso-a0-white",
    "iso-a0xsynchro-translucent",
    "iso-a0xsynchro-transparent",
    "iso-a0xsynchro-white",
    "iso-a1",
    "iso-a1-translucent",
    "iso-a1-transparent",
    "iso-a1-white",
    "iso-a10",
    "iso-a10-white",
    "iso-a1x3-translucent",
    "iso-a1x3-transparent",
    "iso-a1x3-white",
    "iso-a1x4-translucent",
    "iso-a1x4-transparent",
    "iso-a1x4-white",
    "iso-a1xsynchro-translucent",
    "iso-a1xsynchro-transparent",
    "iso-a1xsynchro-white",
    "iso-a2",
    "iso-a2-translucent",
    "iso-a2-transparent",
    "iso-a2-white",
    "iso-a2x3-translucent",
    "iso-a2x3-transparent",
    "iso-a2x3-white",
    "iso-a2x4-translucent",
    "iso-a2x4-transparent",
    "iso-a2x4-white",
    "iso-a2x5-translucent",
    "iso-a2x5-transparent",
    "iso-a2x5-white",
    "iso-a2xsynchro-translucent",
    "iso-a2xsynchro-transparent",
    "iso-a2xsynchro-white",
    "iso-a3",
    "iso-a3-colored",
    "iso-a3-translucent",
    "iso-a3-transparent",
    "iso-a3-white",
    "iso-a3x3-translucent",
    "iso-a3x3-transparent",
    "iso-a3x3-white",
    "iso-a3x4-translucent",
    "iso-a3x4-transparent",
    "iso-a3x4-white",
    "iso-a3x5-translucent",
    "iso-a3x5-transparent",
    "iso-a3x5-white",
    "iso-a3x6-translucent",
    "iso-a3x6-transparent",
    "iso-a3x6-white",
    "iso-a3x7-translucent",
    "iso-a3x7-transparent",
    "iso-a3x7-white",
    "iso-a3xsynchro-translucent",
    "iso-a3xsynchro-transparent",
    "iso-a3xsynchro-white",
    "iso-a4",
    "iso-a4-colored",
    "iso-a4-translucent",
    "iso-a4-transparent",
    "iso-a4-white",
    "iso-a4x3-translucent",
    "iso-a4x3-transparent",
    "iso-a4x3-white",
    "iso-a4x4-translucent",
    "iso-a4x4-transparent",
    "iso-a4x4-white",
    "iso-a4x5-translucent",
    "iso-a4x5-transparent",
    "iso-a4x5-white",
    "iso-a4x6-translucent",
    "iso-a4x6-transparent",
    "iso-a4x6-white",
    "iso-a4x7-translucent",
    "iso-a4x7-transparent",
    "iso-a4x7-white",
    "iso-a4x8-translucent",
    "iso-a4x8-transparent",
    "iso-a4x8-white",
    "iso-a4x9-translucent",
    "iso-a4x9-transparent",
    "iso-a4x9-white",
    "iso-a4xsynchro-translucent",
    "iso-a4xsynchro-transparent",
    "iso-a4xsynchro-white",
    "iso-a5",
    "iso-a5-colored",
    "iso-a5-translucent",
    "iso-a5-transparent",
    "iso-a5-white",
    "iso-a6",
    "iso-a6-white",
    "iso-a7",
    "iso-a7-white",
    "iso-a8",
    "iso-a8-white",
    "iso-a9",
    "iso-a9-white",
    "iso-b0",
    "iso-b0-white",
    "iso-b1",
    "iso-b1-white",
    "iso-b10",
    "iso-b10-white",
    "iso-b2",
    "iso-b2-white",
    "iso-b3",
    "iso-b3-white",
    "iso-b4",
    "iso-b4-colored",
    "iso-b4-envelope",
    "iso-b4-white",
    "iso-b5",
    "iso-b5-colored",
    "iso-b5-envelope",
    "iso-b5-white",
    "iso-b6",
    "iso-b6-white",
    "iso-b7",
    "iso-b7-white",
    "iso-b8",
    "iso-b8-white",
    "iso-b9",
    "iso-b9-white",
    "iso-c3",
    "iso-c3-envelope",
    "iso-c4",
    "iso-c4-envelope",
    "iso-c5",
    "iso-c5-envelope",
    "iso-c6",
    "iso-c6-envelope",
    "iso-designated-long",
    "iso-designated-long-envelope",
    "iso_2a0_1189x1682mm",
    "iso_a0_841x1189mm",
    "iso_a0x3_1189x2523mm",
    "iso_a10_26x37mm",
    "iso_a1_594x841mm",
    "iso_a1x3_841x1783mm",
    "iso_a1x4_841x2378mm",
    "iso_a2_420x594mm",
    "iso_a2x3_594x1261mm",
    "iso_a2x4_594x1682mm",
    "iso_a2x5_594x2102mm",
    "iso_a3-extra_322x445mm",
    "iso_a3_297x420mm",
    "iso_a3x3_420x891mm",
    "iso_a3x4_420x1189mm",
    "iso_a3x5_420x1486mm",
    "iso_a3x6_420x1783mm",
    "iso_a3x7_420x2080mm",
    "iso_a4-extra_235.5x322.3mm",
    "iso_a4-tab_225x297mm",
    "iso_a4_210x297mm",
    "iso_a4x3_297x630mm",
    "iso_a4x4_297x841mm",
    "iso_a4x5_297x1051mm",
    "iso_a4x6_297x1261mm",
    "iso_a4x7_297x1471mm",
    "iso_a4x8_297x1682mm",
    "iso_a4x9_297x1892mm",
    "iso_a5-extra_174x235mm",
    "iso_a5_148x210mm",
    "iso_a6_105x148mm",
    "iso_a7_74x105mm",
    "iso_a8_52x74mm",
    "iso_a9_37x52mm",
    "iso_b0_1000x1414mm",
    "iso_b10_31x44mm",
    "iso_b1_707x1000mm",
    "iso_b2_500x707mm",
    "iso_b3_353x500mm",
    "iso_b4_250x353mm",
    "iso_b5-extra_201x276mm",
    "iso_b5_176x250mm",
    "iso_b6_125x176mm",
    "iso_b6c4_125x324mm",
    "iso_b7_88x125mm",
    "iso_b8_62x88mm",
    "iso_b9_44x62mm",
    "iso_c0_917x1297mm",
    "iso_c10_28x40mm",
    "iso_c1_648x917mm",
    "iso_c2_458x648mm",
    "iso_c3_324x458mm",
    "iso_c4_229x324mm",
    "iso_c5_162x229mm",
    "iso_c6_114x162mm",
    "iso_c6c5_114x229mm",
    "iso_c7_81x114mm",
    "iso_c7c6_81x162mm",
    "iso_c8_57x81mm",
    "iso_c9_40x57mm",
    "iso_dl_110x220mm",
    "iso_id-1_53.98x85.6mm",
    "iso_id-3_88x125mm",
    "iso_ra0_860x1220mm",
    "iso_ra1_610x860mm",
    "iso_ra2_430x610mm",
    "iso_ra3_305x430mm",
    "iso_ra4_215x305mm",
    "iso_sra0_900x1280mm",
    "iso_sra1_640x900mm",
    "iso_sra2_450x640mm",
    "iso_sra3_320x450mm",
    "iso_sra4_225x320mm",
    "ivory",
    "jdf-f10-1",
    "jdf-f10-2",
    "jdf-f10-3",
    "jdf-f12-1",
    "jdf-f12-10",
    "jdf-f12-11",
    "jdf-f12-12",
    "jdf-f12-13",
    "jdf-f12-14",
    "jdf-f12-2",
    "jdf-f12-3",
    "jdf-f12-4",
    "jdf-f12-5",
    "jdf-f12-6",
    "jdf-f12-7",
    "jdf-f12-8",
    "jdf-f12-9",
    "jdf-f14-1",
    "jdf-f16-1",
    "jdf-f16-10",
    "jdf-f16-11",
    "jdf-f16-12",
    "jdf-f16-13",
    "jdf-f16-14",
    "jdf-f16-2",
    "jdf-f16-3",
    "jdf-f16-4",
    "jdf-f16-5",
    "jdf-f16-6",
    "jdf-f16-7",
    "jdf-f16-8",
    "jdf-f16-9",
    "jdf-f18-1",
    "jdf-f18-2",
    "jdf-f18-3",
    "jdf-f18-4",
    "jdf-f18-5",
    "jdf-f18-6",
    "jdf-f18-7",
    "jdf-f18-8",
    "jdf-f18-9",
    "jdf-f2-1",
    "jdf-f20-1",
    "jdf-f20-2",
    "jdf-f24-1",
    "jdf-f24-10",
    "jdf-f24-11",
    "jdf-f24-2",
    "jdf-f24-3",
    "jdf-f24-4",
    "jdf-f24-5",
    "jdf-f24-6",
    "jdf-f24-7",
    "jdf-f24-8",
    "jdf-f24-9",
    "jdf-f28-1",
    "jdf-f32-1",
    "jdf-f32-2",
    "jdf-f32-3",
    "jdf-f32-4",
    "jdf-f32-5",
    "jdf-f32-6",
    "jdf-f32-7",
    "jdf-f32-8",
    "jdf-f32-9",
    "jdf-f36-1",
    "jdf-f36-2",
    "jdf-f4-1",
    "jdf-f4-2",
    "jdf-f40-1",
    "jdf-f48-1",
    "jdf-f48-2",
    "jdf-f6-1",
    "jdf-f6-2",
    "jdf-f6-3",
    "jdf-f6-4",
    "jdf-f6-5",
    "jdf-f6-6",
    "jdf-f6-7",
    "jdf-f6-8",
    "jdf-f64-1",
    "jdf-f64-2",
    "jdf-f8-1",
    "jdf-f8-2",
    "jdf-f8-3",
    "jdf-f8-4",
    "jdf-f8-5",
    "jdf-f8-6",
    "jdf-f8-7",
    "jis-b0",
    "jis-b0-translucent",
    "jis-b0-transparent",
    "jis-b0-white",
    "jis-b1",
    "jis-b1-translucent",
    "jis-b1-transparent",
    "jis-b1-white",
    "jis-b10",
    "jis-b10-white",
    "jis-b2",
    "jis-b2-translucent",
    "jis-b2-transparent",
    "jis-b2-white",
    "jis-b3",
    "jis-b3-translucent",
    "jis-b3-transparent",
    "jis-b3-white",
    "jis-b4",
    "jis-b4-colored",
    "jis-b4-translucent",
    "jis-b4-transparent",
    "jis-b4-white",
    "jis-b5",
    "jis-b5-colored",
    "jis-b5-translucent",
    "jis-b5-transparent",
    "jis-b5-white",
    "jis-b6",
    "jis-b6-white",
    "jis-b7",
    "jis-b7-white",
    "jis-b8",
    "jis-b8-white",
    "jis-b9",
    "jis-b9-white",
    "jis_b0_1030x1456mm",
    "jis_b10_32x45mm",
    "jis_b1_728x1030mm",
    "jis_b2_515x728mm",
    "jis_b3_364x515mm",
    "jis_b4_257x364mm",
    "jis_b5_182x257mm",
    "jis_b6_128x182mm",
    "jis_b7_91x128mm",
    "jis_b8_64x91mm",
    "jis_b9_45x64mm",
    "jis_exec_216x330mm",
    "job-account-id",
    "job-account-id-actual",
    "job-account-id-default",
    "job-account-id-supported",
    "job-account-type",
    "job-account-type-default",
    "job-account-type-supported",
    "job-accounting-output-bin",
    "job-accounting-sheets",
    "job-accounting-sheets-actual",
    "job-accounting-sheets-default",
    "job-accounting-sheets-type",
    "job-accounting-user-id",
    "job-accounting-user-id-actual",
    "job-accounting-user-id-default",
    "job-accounting-user-id-supported",
    "job-actuals",
    "job-attribute-fidelity",
    "job-attributes",
    "job-authorization-uri-supported",
    "job-both-sheet",
    "job-canceled-at-device",
    "job-canceled-by-operator",
    "job-canceled-by-user",
    "job-charge-info",
    "job-collation-type",
    "job-completed",
    "job-completed-successfully",
    "job-completed-with-errors",
    "job-completed-with-warnings",
    "job-config-changed",
    "job-constraints-supported",
    "job-copies",
    "job-copies-actual",
    "job-copies-default",
    "job-copies-supported",
    "job-cover-back",
    "job-cover-back-actual",
    "job-cover-back-default",
    "job-cover-back-supported",
    "job-cover-front",
    "job-cover-front-actual",
    "job-cover-front-default",
    "job-cover-front-supported",
    "job-created",
    "job-data-insufficient",
    "job-delay-output-until",
    "job-delay-output-until-default",
    "job-delay-output-until-specified",
    "job-delay-output-until-supported",
    "job-delay-output-until-time",
    "job-delay-output-until-time-supported",
    "job-description",
    "job-detailed-status-messages",
    "job-digital-signature-wait",
    "job-document-access-errors",
    "job-end-sheet",
    "job-error-action",
    "job-error-action-default",
    "job-error-action-supported",
    "job-error-sheet",
    "job-error-sheet-actual",
    "job-error-sheet-default",
    "job-error-sheet-type",
    "job-error-sheet-when",
    "job-fetchable",
    "job-finishings",
    "job-finishings-col",
    "job-finishings-col-actual",
    "job-finishings-col-default",
    "job-finishings-col-ready",
    "job-finishings-default",
    "job-finishings-ready",
    "job-finishings-supported",
    "job-held-for-review",
    "job-hold-until",
    "job-hold-until-default",
    "job-hold-until-specified",
    "job-hold-until-supported",
    "job-hold-until-time",
    "job-hold-until-time-supported",
    "job-id",
    "job-ids-supported",
    "job-impressions",
    "job-impressions-completed",
    "job-impressions-supported",
    "job-incoming",
    "job-interpreting",
    "job-k-octets",
    "job-k-octets-processed",
    "job-k-octets-supported",
    "job-mandatory-attributes",
    "job-media-sheets",
    "job-media-sheets-completed",
    "job-media-sheets-supported",
    "job-message-from-operator",
    "job-message-to-operator",
    "job-message-to-operator-actual",
    "job-message-to-operator-default",
    "job-message-to-operator-supported",
    "job-more-info",
    "job-name",
    "job-originating-user-name",
    "job-originating-user-uri",
    "job-outgoing",
    "job-pages",
    "job-pages-completed",
    "job-pages-completed-current-copy",
    "job-pages-per-set",
    "job-pages-per-set-supported",
    "job-password-encryption",
    "job-password-encryption-supported",
    "job-password-supported",
    "job-password-wait",
    "job-phone-number",
    "job-phone-number-default",
    "job-phone-number-supported",
    "job-printed-successfully",
    "job-printed-with-errors",
    "job-printed-with-warnings",
    "job-printer-up-time",
    "job-printer-uri",
    "job-printing",
    "job-priority",
    "job-priority-actual",
    "job-priority-default",
    "job-priority-supported",
    "job-progress",
    "job-queued",
    "job-queued-for-marker",
    "job-recipient-name",
    "job-recipient-name-default",
    "job-recipient-name-supported",
    "job-release-wait",
    "job-resolvers-supported",
    "job-resource-ids",
    "job-restartable",
    "job-resuming",
    "job-save",
    "job-save-disposition",
    "job-save-printer-make-and-model",
    "job-saved-successfully",
    "job-saved-with-errors",
    "job-saved-with-warnings",
    "job-saving",
    "job-sheet-message",
    "job-sheet-message-actual",
    "job-sheet-message-default",
    "job-sheet-message-supported",
    "job-sheets",
    "job-sheets-col",
    "job-sheets-col-actual",
    "job-sheets-col-default",
    "job-sheets-default",
    "job-sheets-supported",
    "job-spooling",
    "job-spooling-supported",
    "job-start-sheet",
    "job-state",
    "job-state-changed",
    "job-state-message",
    "job-state-reasons",
    "job-stopped",
    "job-streaming",
    "job-suspended",
    "job-suspended-by-operator",
    "job-suspended-by-system",
    "job-suspended-by-user",
    "job-suspending",
    "job-template",
    "job-transferring",
    "job-transforming",
    "job-uri",
    "job-uuid",
    "jog-offset",
    "jpeg-k-octets-supported",
    "jpeg-x-dimension-supported",
    "jpeg-y-dimension-supported",
    "jpn_chou2_111.1x146mm",
    "jpn_chou3_120x235mm",
    "jpn_chou4_90x205mm",
    "jpn_hagaki_100x148mm",
    "jpn_kahu_240x322.1mm",
    "jpn_kaku1_270x382mm",
    "jpn_kaku2_240x332mm",
    "jpn_kaku3_216x277mm",
    "jpn_kaku4_197x267mm",
    "jpn_kaku5_190x240mm",
    "jpn_kaku7_142x205mm",
    "jpn_kaku8_119x197mm",
    "jpn_oufuku_148x200mm",
    "jpn_you4_105x235mm",
    "labels",
    "labels-colored",
    "labels-glossy",
    "labels-high-gloss",
    "labels-inkjet",
    "labels-matte",
    "labels-permanent",
    "labels-satin",
    "labels-security",
    "labels-semi-gloss",
    "laminate",
    "laminating",
    "laminating-foil",
    "laminating-sides",
    "laminating-sides-supported",
    "laminating-type",
    "laminating-type-supported",
    "lamp-at-eol",
    "lamp-failure",
    "lamp-near-eol",
    "landscape",
    "large-capacity",
    "laser-at-eol",
    "laser-failure",
    "laser-near-eol",
    "last-document",
    "ledger",
    "ledger-white",
    "left",
    "letter-head",
    "letterhead",
    "light-black",
    "light-blue",
    "light-brown",
    "light-buff",
    "light-cyan",
    "light-gold",
    "light-goldenrod",
    "light-gray",
    "light-green",
    "light-ivory",
    "light-magenta",
    "light-mustard",
    "light-orange",
    "light-pink",
    "light-red",
    "light-silver",
    "light-turquoise",
    "light-violet",
    "light-yellow",
    "limit",
    "linen",
    "long-edge-first",
    "magenta",
    "mailbox-1",
    "mailbox-10",
    "mailbox-2",
    "mailbox-3",
    "mailbox-4",
    "mailbox-5",
    "mailbox-6",
    "mailbox-7",
    "mailbox-8",
    "mailbox-9",
    "main",
    "main-roll",
    "make-envelope-added",
    "make-envelope-almost-empty",
    "make-envelope-almost-full",
    "make-envelope-at-limit",
    "make-envelope-closed",
    "make-envelope-configuration-change",
    "make-envelope-cover-closed",
    "make-envelope-cover-open",
    "make-envelope-empty",
    "make-envelope-full",
    "make-envelope-interlock-closed",
    "make-envelope-interlock-open",
    "make-envelope-jam",
    "make-envelope-life-almost-over",
    "make-envelope-life-over",
    "make-envelope-memory-exhausted",
    "make-envelope-missing",
    "make-envelope-motor-failure",
    "make-envelope-near-limit",
    "make-envelope-offline",
    "make-envelope-opened",
    "make-envelope-over-temperature",
    "make-envelope-power-saver",
    "make-envelope-recoverable-failure",
    "make-envelope-recoverable-storage",
    "make-envelope-removed",
    "make-envelope-resource-added",
    "make-envelope-resource-removed",
    "make-envelope-thermistor-failure",
    "make-envelope-timing-failure",
    "make-envelope-turned-off",
    "make-envelope-turned-on",
    "make-envelope-under-temperature",
    "make-envelope-unrecoverable-failure",
    "make-envelope-unrecoverable-storage-error",
    "make-envelope-warming-up",
    "manual",
    "manual-tumble",
    "marker-adjusting-print-quality",
    "marker-developer-almost-empty",
    "marker-developer-empty",
    "marker-fuser-thermistor-failure",
    "marker-fuser-timing-failure",
    "marker-ink-almost-empty",
    "marker-ink-empty",
    "marker-print-ribbon-almost-empty",
    "marker-print-ribbon-empty",
    "marker-supply-empty",
    "marker-supply-low",
    "marker-toner-cartridge-missing",
    "marker-waste-almost-full",
    "marker-waste-full",
    "marker-waste-ink-receptacle-almost-full",
    "marker-waste-ink-receptacle-full",
    "marker-waste-toner-receptacle-almost-full",
    "marker-waste-toner-receptacle-full",
    "material-color",
    "material-empty",
    "material-low",
    "material-needed",
    "matte",
    "max-save-info-supported",
    "max-stitching-locations-supported",
    "md2",
    "md4",
    "md5",
    "media",
    "media-back-coating",
    "media-back-coating-supported",
    "media-bottom-margin",
    "media-bottom-margin-supported",
    "media-col",
    "media-col-actual",
    "media-col-database",
    "media-col-default",
    "media-col-ready",
    "media-color",
    "media-color-supported",
    "media-default",
    "media-empty",
    "media-front-coating",
    "media-front-coating-supported",
    "media-grain",
    "media-grain-supported",
    "media-hole-count",
    "media-hole-count-supported",
    "media-info",
    "media-info-supported",
    "media-input-tray-check",
    "media-jam",
    "media-key",
    "media-left-margin",
    "media-left-margin-supported",
    "media-low",
    "media-needed",
    "media-order-count",
    "media-order-count-supported",
    "media-path-cannot-duplex-media-selected",
    "media-path-media-tray-almost-full",
    "media-path-media-tray-full",
    "media-path-media-tray-missing",
    "media-pre-printed",
    "media-pre-printed-supported",
    "media-ready",
    "media-recycled",
    "media-recycled-supported",
    "media-right-margin",
    "media-right-margin-supported",
    "media-sheets-supported",
    "media-size",
    "media-size-name",
    "media-size-supported",
    "media-source",
    "media-source-feed-direction",
    "media-source-feed-orientation",
    "media-source-properties",
    "media-source-supported",
    "media-supported",
    "media-thickness",
    "media-thickness-supported",
    "media-tooth",
    "media-tooth-supported",
    "media-top-margin",
    "media-top-margin-supported",
    "media-type",
    "media-type-supported",
    "media-weight-metric",
    "media-weight-metric-supported",
    "medium",
    "member-names",
    "member-uris",
    "message",
    "metal",
    "metal-glossy",
    "metal-high-gloss",
    "metal-matte",
    "metal-satin",
    "metal-semi-gloss",
    "middle",
    "monarch",
    "monarch-envelope",
    "monochrome",
    "motor-failure",
    "mounting-tape",
    "moving-to-paused",
    "multi-color",
    "multi-layer",
    "multi-part-form",
    "multiple-document-handling",
    "multiple-document-handling-default",
    "multiple-document-handling-supported",
    "multiple-document-jobs-supported",
    "multiple-operation-time-out",
    "multiple-operation-time-out-action",
    "mustard",
    "my-jobs",
    "my-mailbox",
    "n-to-1-order",
    "na-10x13",
    "na-10x13-envelope",
    "na-10x14",
    "na-10x14-envelope",
    "na-10x15",
    "na-10x15-envelope",
    "na-5x7",
    "na-6x9",
    "na-6x9-envelope",
    "na-7x9",
    "na-7x9-envelope",
    "na-8x10",
    "na-9x11",
    "na-9x11-envelope",
    "na-9x12",
    "na-9x12-envelope",
    "na-legal",
    "na-legal-colored",
    "na-legal-white",
    "na-letter",
    "na-letter-colored",
    "na-letter-transparent",
    "na-letter-white",
    "na-number-10",
    "na-number-10-envelope",
    "na-number-9",
    "na-number-9-envelope",
    "na_10x11_10x11in",
    "na_10x13_10x13in",
    "na_10x14_10x14in",
    "na_10x15_10x15in",
    "na_11x12_11x12in",
    "na_11x15_11x15in",
    "na_12x19_12x19in",
    "na_5x7_5x7in",
    "na_6x9_6x9in",
    "na_7x9_7x9in",
    "na_9x11_9x11in",
    "na_a2_4.375x5.75in",
    "na_arch-a_9x12in",
    "na_arch-b_12x18in",
    "na_arch-c_18x24in",
    "na_arch-d_24x36in",
    "na_arch-e2_26x38in",
    "na_arch-e3_27x39in",
    "na_arch-e_36x48in",
    "na_b-plus_12x19.17in",
    "na_c5_6.5x9.5in",
    "na_c_17x22in",
    "na_d_22x34in",
    "na_e_34x44in",
    "na_edp_11x14in",
    "na_eur-edp_12x14in",
    "na_executive_7.25x10.5in",
    "na_f_44x68in",
    "na_fanfold-eur_8.5x12in",
    "na_fanfold-us_11x14.875in",
    "na_foolscap_8.5x13in",
    "na_govt-legal_8x13in",
    "na_govt-letter_8x10in",
    "na_index-3x5_3x5in",
    "na_index-4x6-ext_6x8in",
    "na_index-4x6_4x6in",
    "na_index-5x8_5x8in",
    "na_invoice_5.5x8.5in",
    "na_ledger_11x17in",
    "na_legal-extra_9.5x15in",
    "na_legal_8.5x14in",
    "na_letter-extra_9.5x12in",
    "na_letter-plus_8.5x12.69in",
    "na_letter_8.5x11in",
    "na_monarch_3.875x7.5in",
    "na_number-10_4.125x9.5in",
    "na_number-11_4.5x10.375in",
    "na_number-12_4.75x11in",
    "na_number-14_5x11.5in",
    "na_number-9_3.875x8.875in",
    "na_oficio_8.5x13.4in",
    "na_personal_3.625x6.5in",
    "na_quarto_8.5x10.83in",
    "na_super-a_8.94x14in",
    "na_super-b_13x19in",
    "na_wide-format_30x42in",
    "natural-language-configured",
    "negotiate",
    "night",
    "no-color",
    "no-cover",
    "no-delay-output",
    "no-hold",
    "none",
    "normal",
    "not-attempted",
    "not-completed",
    "notify-events",
    "notify-events-default",
    "notify-events-supported",
    "notify-lease-duration-default",
    "notify-lease-duration-supported",
    "notify-pull-method",
    "notify-pull-method-supported",
    "notify-schemes-supported",
    "number-of-documents",
    "number-of-intervening-jobs",
    "number-up",
    "number-up-actual",
    "number-up-default",
    "number-up-supported",
    "oauth",
    "oauth-authorization-server-uri",
    "oe_12x16_12x16in",
    "oe_14x17_14x17in",
    "oe_18x22_18x22in",
    "oe_a2plus_17x24in",
    "oe_business-card_2x3.5in",
    "oe_photo-10r_10x12in",
    "oe_photo-20r_20x24in",
    "oe_photo-l_3.5x5in",
    "oe_photo-s10r_10x15in",
    "oe_square-photo_4x4in",
    "oe_square-photo_5x5in",
    "om_16k_184x260mm",
    "om_16k_195x270mm",
    "om_business-card_55x85mm",
    "om_business-card_55x91mm",
    "om_card_54x86mm",
    "om_dai-pa-kai_275x395mm",
    "om_dsc-photo_89x119mm",
    "om_folio-sp_215x315mm",
    "om_folio_210x330mm",
    "om_invite_220x220mm",
    "om_italian_110x230mm",
    "om_juuro-ku-kai_198x275mm",
    "om_large-photo_200x300",
    "om_medium-photo_130x180mm",
    "om_pa-kai_267x389mm",
    "om_postfix_114x229mm",
    "om_small-photo_100x150mm",
    "om_square-photo_89x89mm",
    "om_wide-photo_100x200mm",
    "on-error",
    "one-sided",
    "opc-life-over",
    "opc-near-eol",
    "operation-attributes",
    "operations-supported",
    "orange",
    "orientation-requested",
    "orientation-requested-default",
    "orientation-requested-supported",
    "original-requesting-user-name",
    "other",
    "output-area-almost-full",
    "output-area-full",
    "output-bin",
    "output-bin-default",
    "output-bin-supported",
    "output-device",
    "output-device-actual",
    "output-device-assigned",
    "output-device-job-state-message",
    "output-device-supported",
    "output-device-uuid-assigned",
    "output-device-uuid-supported",
    "output-mailbox-select-failure",
    "output-tray-missing",
    "outward",
    "overrides",
    "overrides-actual",
    "padding",
    "page-delivery",
    "page-delivery-default",
    "page-delivery-supported",
    "page-order-received",
    "page-order-received-default",
    "page-order-received-supported",
    "page-overrides",
    "page-ranges",
    "page-ranges-actual",
    "page-ranges-supported",
    "pages",
    "pages-per-minute",
    "pages-per-minute-color",
    "pages-per-subset",
    "pages-per-subset-supported",
    "paper",
    "parent-printers-supported",
    "partial",
    "password",
    "paused",
    "pdf-k-octets-supported",
    "pdf-versions-supported",
    "pdl-init-file",
    "pdl-init-file-default",
    "pdl-init-file-entry",
    "pdl-init-file-entry-supported",
    "pdl-init-file-location",
    "pdl-init-file-location-supported",
    "pdl-init-file-name",
    "pdl-init-file-name-subdirectory-supported",
    "pdl-init-file-name-supported",
    "pdl-init-file-supported",
    "pdl-override-supported",
    "pending",
    "pending-held",
    "perceptual",
    "perfect",
    "perforate",
    "perforater-added",
    "perforater-almost-empty",
    "perforater-almost-full",
    "perforater-at-limit",
    "perforater-closed",
    "perforater-configuration-change",
    "perforater-cover-closed",
    "perforater-cover-open",
    "perforater-empty",
    "perforater-full",
    "perforater-interlock-closed",
    "perforater-interlock-open",
    "perforater-jam",
    "perforater-life-almost-over",
    "perforater-life-over",
    "perforater-memory-exhausted",
    "perforater-missing",
    "perforater-motor-failure",
    "perforater-near-limit",
    "perforater-offline",
    "perforater-opened",
    "perforater-over-temperature",
    "perforater-power-saver",
    "perforater-recoverable-failure",
    "perforater-recoverable-storage",
    "perforater-removed",
    "perforater-resource-added",
    "perforater-resource-removed",
    "perforater-thermistor-failure",
    "perforater-timing-failure",
    "perforater-turned-off",
    "perforater-turned-on",
    "perforater-under-temperature",
    "perforater-unrecoverable-failure",
    "perforater-unrecoverable-storage-error",
    "perforater-warming-up",
    "pgp",
    "photo",
    "photographic",
    "photographic-archival",
    "photographic-film",
    "photographic-glossy",
    "photographic-high-gloss",
    "photographic-matte",
    "photographic-satin",
    "photographic-semi-gloss",
    "pink",
    "plain",
    "plastic",
    "plastic-archival",
    "plastic-colored",
    "plastic-glossy",
    "plastic-high-gloss",
    "plastic-matte",
    "plastic-satin",
    "plastic-semi-gloss",
    "plate",
    "platform-cooling",
    "platform-failure",
    "platform-heating",
    "platform-temperature-high",
    "platform-temperature-low",
    "polyester",
    "portrait",
    "power-down",
    "power-up",
    "prc_10_324x458mm",
    "prc_16k_146x215mm",
    "prc_1_102x165mm",
    "prc_2_102x176mm",
    "prc_32k_97x151mm",
    "prc_3_125x176mm",
    "prc_4_110x208mm",
    "prc_5_110x220mm",
    "prc_6_120x320mm",
    "prc_7_160x230mm",
    "prc_8_120x309mm",
    "pre-cut",
    "pre-cut-tabs",
    "pre-printed",
    "pre-punched",
    "preferred-attributes-supported",
    "presentation-direction-number-up",
    "presentation-direction-number-up-default",
    "presentation-direction-number-up-supported",
    "print-back",
    "print-both",
    "print-color-mode",
    "print-color-mode-default",
    "print-color-mode-supported",
    "print-content-optimize",
    "print-content-optimize-default",
    "print-content-optimize-supported",
    "print-front",
    "print-none",
    "print-quality",
    "print-quality-default",
    "print-quality-supported",
    "print-rendering-intent",
    "print-rendering-intent-default",
    "print-rendering-intent-supported",
    "print-save",
    "print-scaling",
    "printer-alert",
    "printer-alert-description",
    "printer-attributes",
    "printer-charge-info",
    "printer-charge-info-uri",
    "printer-config-change-date-time",
    "printer-config-change-time",
    "printer-config-changed",
    "printer-config-changes",
    "printer-contact-col",
    "printer-created",
    "printer-current-time",
    "printer-deleted",
    "printer-description",
    "printer-detailed-status-messages",
    "printer-device-id",
    "printer-dns-sd-name",
    "printer-driver-installer",
    "printer-finisher",
    "printer-finisher-description",
    "printer-finisher-supplies",
    "printer-finisher-supplies-description",
    "printer-finishings-changed",
    "printer-geo-location",
    "printer-icc-profiles",
    "printer-icons",
    "printer-id",
    "printer-impressions-completed",
    "printer-info",
    "printer-input-tray",
    "printer-is-accepting-jobs",
    "printer-location",
    "printer-make-and-model",
    "printer-manual-reset",
    "printer-media-changed",
    "printer-media-sheets-completed",
    "printer-message-date-time",
    "printer-message-from-operator",
    "printer-message-time",
    "printer-more-info",
    "printer-more-info-manufacturer",
    "printer-name",
    "printer-nms-reset",
    "printer-organization",
    "printer-organizational-unit",
    "printer-output-tray",
    "printer-pages-completed",
    "printer-queue-order-changed",
    "printer-ready-to-print",
    "printer-resolution",
    "printer-resolution-actual",
    "printer-resolution-default",
    "printer-resolution-supported",
    "printer-restarted",
    "printer-shutdown",
    "printer-state",
    "printer-state-change-date-time",
    "printer-state-change-time",
    "printer-state-changed",
    "printer-state-message",
    "printer-state-reasons",
    "printer-static-resource-directory-uri",
    "printer-static-resource-k-octets-free",
    "printer-static-resource-k-octets-supported",
    "printer-stopped",
    "printer-stopped-partly",
    "printer-strings-languages-supported",
    "printer-strings-uri",
    "printer-supply",
    "printer-supply-description",
    "printer-supply-info-uri",
    "printer-type",
    "printer-type-mask",
    "printer-up-time",
    "printer-uri",
    "printer-uri-supported",
    "printer-uuid",
    "printer-xri-supported",
    "process-bi-level",
    "process-job",
    "process-monochrome",
    "processing",
    "processing-stopped",
    "processing-to-stop-point",
    "profile-name",
    "profile-url",
    "proof-print",
    "proof-print-copies",
    "proof-print-default",
    "proof-print-supported",
    "punch",
    "punch-bottom-left",
    "punch-bottom-right",
    "punch-dual-bottom",
    "punch-dual-left",
    "punch-dual-right",
    "punch-dual-top",
    "punch-multiple-bottom",
    "punch-multiple-left",
    "punch-multiple-right",
    "punch-multiple-top",
    "punch-quad-bottom",
    "punch-quad-left",
    "punch-quad-right",
    "punch-quad-top",
    "punch-top-left",
    "punch-top-right",
    "punch-triple-bottom",
    "punch-triple-left",
    "punch-triple-right",
    "punch-triple-top",
    "puncher-added",
    "puncher-almost-empty",
    "puncher-almost-full",
    "puncher-at-limit",
    "puncher-closed",
    "puncher-configuration-change",
    "puncher-cover-closed",
    "puncher-cover-open",
    "puncher-empty",
    "puncher-full",
    "puncher-interlock-closed",
    "puncher-interlock-open",
    "puncher-jam",
    "puncher-life-almost-over",
    "puncher-life-over",
    "puncher-memory-exhausted",
    "puncher-missing",
    "puncher-motor-failure",
    "puncher-near-limit",
    "puncher-offline",
    "puncher-opened",
    "puncher-over-temperature",
    "puncher-power-saver",
    "puncher-recoverable-failure",
    "puncher-recoverable-storage",
    "puncher-removed",
    "puncher-resource-added",
    "puncher-resource-removed",
    "puncher-thermistor-failure",
    "puncher-timing-failure",
    "puncher-turned-off",
    "puncher-turned-on",
    "puncher-under-temperature",
    "puncher-unrecoverable-failure",
    "puncher-unrecoverable-storage-error",
    "puncher-warming-up",
    "punching",
    "punching-hole-diameter-configured",
    "punching-locations",
    "punching-locations-supported",
    "punching-offset",
    "punching-offset-supported",
    "punching-reference-edge",
    "punching-reference-edge-supported",
    "pwg-5102.3",
    "pwg-raster-document-resolution-supported",
    "pwg-raster-document-sheet-back",
    "pwg-raster-document-type-supported",
    "quarto",
    "quarto-white",
    "queued-in-device",
    "queued-job-count",
    "rear",
    "recycled",
    "red",
    "reference-uri-schemes-supported",
    "relative",
    "relative-bpc",
    "requested-attributes",
    "requested-user-name",
    "requesting-user-name",
    "requesting-user-name-allowed",
    "requesting-user-name-denied",
    "requesting-user-uri-supported",
    "resolver-name",
    "resource-attributes",
    "resource-canceled",
    "resource-config-changed",
    "resource-created",
    "resource-description",
    "resource-installed",
    "resource-state-changed",
    "resource-status",
    "resource-template",
    "resources-are-not-ready",
    "resources-are-not-supported",
    "resuming",
    "reverse-landscape",
    "reverse-order-face-down",
    "reverse-order-face-up",
    "reverse-portrait",
    "rgb_16",
    "rgb_8",
    "right",
    "roc_16k_7.75x10.75in",
    "roc_8k_10.75x15.5in",
    "roll",
    "roll-1",
    "roll-10",
    "roll-2",
    "roll-3",
    "roll-4",
    "roll-5",
    "roll-6",
    "roll-7",
    "roll-8",
    "roll-9",
    "rotated",
    "saddle-stitch",
    "same-order-face-down",
    "same-order-face-up",
    "satin",
    "saturation",
    "save-disposition",
    "save-disposition-supported",
    "save-document-format",
    "save-document-format-default",
    "save-document-format-supported",
    "save-info",
    "save-location",
    "save-location-default",
    "save-location-supported",
    "save-name",
    "save-name-subdirectory-supported",
    "save-name-supported",
    "save-only",
    "saved",
    "scan",
    "score",
    "screen",
    "screen-paged",
    "second-shift",
    "self-adhesive",
    "self-adhesive-film",
    "semi-gloss",
    "separate-documents-collated-copies",
    "separate-documents-uncollated-copies",
    "separation-cutter-added",
    "separation-cutter-almost-empty",
    "separation-cutter-almost-full",
    "separation-cutter-at-limit",
    "separation-cutter-closed",
    "separation-cutter-configuration-change",
    "separation-cutter-cover-closed",
    "separation-cutter-cover-open",
    "separation-cutter-empty",
    "separation-cutter-full",
    "separation-cutter-interlock-closed",
    "separation-cutter-interlock-open",
    "separation-cutter-jam",
    "separation-cutter-life-almost-over",
    "separation-cutter-life-over",
    "separation-cutter-memory-exhausted",
    "separation-cutter-missing",
    "separation-cutter-motor-failure",
    "separation-cutter-near-limit",
    "separation-cutter-offline",
    "separation-cutter-opened",
    "separation-cutter-over-temperature",
    "separation-cutter-power-saver",
    "separation-cutter-recoverable-failure",
    "separation-cutter-recoverable-storage",
    "separation-cutter-removed",
    "separation-cutter-resource-added",
    "separation-cutter-resource-removed",
    "separation-cutter-thermistor-failure",
    "separation-cutter-timing-failure",
    "separation-cutter-turned-off",
    "separation-cutter-turned-on",
    "separation-cutter-under-temperature",
    "separation-cutter-unrecoverable-failure",
    "separation-cutter-unrecoverable-storage-error",
    "separation-cutter-warming-up",
    "separator-sheets",
    "separator-sheets-actual",
    "separator-sheets-default",
    "separator-sheets-type",
    "server-error-busy",
    "server-error-device-error",
    "server-error-internal-error",
    "server-error-job-canceled",
    "server-error-multiple-document-jobs-not-supported",
    "server-error-not-accepting-jobs",
    "server-error-operation-not-supported",
    "server-error-printer-is-deactivated",
    "server-error-service-unavailable",
    "server-error-temporary-error",
    "server-error-too-many-documents",
    "server-error-too-many-jobs",
    "server-error-version-not-supported",
    "service-off-line",
    "sgray_1",
    "sgray_16",
    "sgray_8",
    "sha",
    "sha2-224",
    "sha2-256",
    "sha2-384",
    "sha2-512",
    "sha2-512_224",
    "sha2-512_256",
    "sha3-224",
    "sha3-256",
    "sha3-384",
    "sha3-512",
    "sha3-512_224",
    "sha3-512_256",
    "shake-128",
    "shake-256",
    "sheet-collate",
    "sheet-collate-default",
    "sheet-collate-supported",
    "sheet-completed-copy-number",
    "sheet-completed-document-number",
    "sheet-rotator-added",
    "sheet-rotator-almost-empty",
    "sheet-rotator-almost-full",
    "sheet-rotator-at-limit",
    "sheet-rotator-closed",
    "sheet-rotator-configuration-change",
    "sheet-rotator-cover-closed",
    "sheet-rotator-cover-open",
    "sheet-rotator-empty",
    "sheet-rotator-full",
    "sheet-rotator-interlock-closed",
    "sheet-rotator-interlock-open",
    "sheet-rotator-jam",
    "sheet-rotator-life-almost-over",
    "sheet-rotator-life-over",
    "sheet-rotator-memory-exhausted",
    "sheet-rotator-missing",
    "sheet-rotator-motor-failure",
    "sheet-rotator-near-limit",
    "sheet-rotator-offline",
    "sheet-rotator-opened",
    "sheet-rotator-over-temperature",
    "sheet-rotator-power-saver",
    "sheet-rotator-recoverable-failure",
    "sheet-rotator-recoverable-storage",
    "sheet-rotator-removed",
    "sheet-rotator-resource-added",
    "sheet-rotator-resource-removed",
    "sheet-rotator-thermistor-failure",
    "sheet-rotator-timing-failure",
    "sheet-rotator-turned-off",
    "sheet-rotator-turned-on",
    "sheet-rotator-under-temperature",
    "sheet-rotator-unrecoverable-failure",
    "sheet-rotator-unrecoverable-storage-error",
    "sheet-rotator-warming-up",
    "short-edge-first",
    "shrink-foil",
    "shrink-wrap",
    "shutdown",
    "side",
    "sides",
    "sides-default",
    "sides-supported",
    "signature",
    "silicone",
    "silver",
    "single-document",
    "single-document-new-sheet",
    "single-face",
    "single-wall",
    "sleeve",
    "slip-sheets",
    "slitter-added",
    "slitter-almost-empty",
    "slitter-almost-full",
    "slitter-at-limit",
    "slitter-closed",
    "slitter-configuration-change",
    "slitter-cover-closed",
    "slitter-cover-open",
    "slitter-empty",
    "slitter-full",
    "slitter-interlock-closed",
    "slitter-interlock-open",
    "slitter-jam",
    "slitter-life-almost-over",
    "slitter-life-over",
    "slitter-memory-exhausted",
    "slitter-missing",
    "slitter-motor-failure",
    "slitter-near-limit",
    "slitter-offline",
    "slitter-opened",
    "slitter-over-temperature",
    "slitter-power-saver",
    "slitter-recoverable-failure",
    "slitter-recoverable-storage",
    "slitter-removed",
    "slitter-resource-added",
    "slitter-resource-removed",
    "slitter-thermistor-failure",
    "slitter-timing-failure",
    "slitter-turned-off",
    "slitter-turned-on",
    "slitter-under-temperature",
    "slitter-unrecoverable-failure",
    "slitter-unrecoverable-storage-error",
    "slitter-warming-up",
    "smime",
    "smooth",
    "sound",
    "speak",
    "spiral",
    "spool",
    "spool-area-full",
    "srgb_16",
    "srgb_8",
    "ssl3",
    "stacker-1",
    "stacker-10",
    "stacker-2",
    "stacker-3",
    "stacker-4",
    "stacker-5",
    "stacker-6",
    "stacker-7",
    "stacker-8",
    "stacker-9",
    "stacker-added",
    "stacker-almost-empty",
    "stacker-almost-full",
    "stacker-at-limit",
    "stacker-closed",
    "stacker-configuration-change",
    "stacker-cover-closed",
    "stacker-cover-open",
    "stacker-empty",
    "stacker-full",
    "stacker-interlock-closed",
    "stacker-interlock-open",
    "stacker-jam",
    "stacker-life-almost-over",
    "stacker-life-over",
    "stacker-memory-exhausted",
    "stacker-missing",
    "stacker-motor-failure",
    "stacker-near-limit",
    "stacker-offline",
    "stacker-opened",
    "stacker-over-temperature",
    "stacker-power-saver",
    "stacker-recoverable-failure",
    "stacker-recoverable-storage",
    "stacker-removed",
    "stacker-resource-added",
    "stacker-resource-removed",
    "stacker-thermistor-failure",
    "stacker-timing-failure",
    "stacker-turned-off",
    "stacker-turned-on",
    "stacker-under-temperature",
    "stacker-unrecoverable-failure",
    "stacker-unrecoverable-storage-error",
    "stacker-warming-up",
    "standard",
    "staple",
    "staple-bottom-left",
    "staple-bottom-right",
    "staple-dual-bottom",
    "staple-dual-left",
    "staple-dual-right",
    "staple-dual-top",
    "staple-top-left",
    "staple-top-right",
    "staple-triple-bottom",
    "staple-triple-left",
    "staple-triple-right",
    "staple-triple-top",
    "stapler-added",
    "stapler-almost-empty",
    "stapler-almost-full",
    "stapler-at-limit",
    "stapler-closed",
    "stapler-configuration-change",
    "stapler-cover-closed",
    "stapler-cover-open",
    "stapler-empty",
    "stapler-full",
    "stapler-interlock-closed",
    "stapler-interlock-open",
    "stapler-jam",
    "stapler-life-almost-over",
    "stapler-life-over",
    "stapler-memory-exhausted",
    "stapler-missing",
    "stapler-motor-failure",
    "stapler-near-limit",
    "stapler-offline",
    "stapler-opened",
    "stapler-over-temperature",
    "stapler-power-saver",
    "stapler-recoverable-failure",
    "stapler-recoverable-storage",
    "stapler-removed",
    "stapler-resource-added",
    "stapler-resource-removed",
    "stapler-thermistor-failure",
    "stapler-timing-failure",
    "stapler-turned-off",
    "stapler-turned-on",
    "stapler-under-temperature",
    "stapler-unrecoverable-failure",
    "stapler-unrecoverable-storage-error",
    "stapler-warming-up",
    "start-sheet",
    "stationery",
    "stationery-archival",
    "stationery-coated",
    "stationery-cotton",
    "stationery-fine",
    "stationery-heavyweight",
    "stationery-heavyweight-coated",
    "stationery-inkjet",
    "stationery-letterhead",
    "stationery-lightweight",
    "stationery-preprinted",
    "stationery-prepunched",
    "status-code",
    "status-message",
    "stipple",
    "stitcher-added",
    "stitcher-almost-empty",
    "stitcher-almost-full",
    "stitcher-at-limit",
    "stitcher-closed",
    "stitcher-configuration-change",
    "stitcher-cover-closed",
    "stitcher-cover-open",
    "stitcher-empty",
    "stitcher-full",
    "stitcher-interlock-closed",
    "stitcher-interlock-open",
    "stitcher-jam",
    "stitcher-life-almost-over",
    "stitcher-life-over",
    "stitcher-memory-exhausted",
    "stitcher-missing",
    "stitcher-motor-failure",
    "stitcher-near-limit",
    "stitcher-offline",
    "stitcher-opened",
    "stitcher-over-temperature",
    "stitcher-power-saver",
    "stitcher-recoverable-failure",
    "stitcher-recoverable-storage",
    "stitcher-removed",
    "stitcher-resource-added",
    "stitcher-resource-removed",
    "stitcher-thermistor-failure",
    "stitcher-timing-failure",
    "stitcher-turned-off",
    "stitcher-turned-on",
    "stitcher-under-temperature",
    "stitcher-unrecoverable-failure",
    "stitcher-unrecoverable-storage-error",
    "stitcher-warming-up",
    "stitching",
    "stitching-angle",
    "stitching-angle-supported",
    "stitching-locations",
    "stitching-locations-supported",
    "stitching-method",
    "stitching-method-supported",
    "stitching-offset",
    "stitching-offset-supported",
    "stitching-reference-edge",
    "stitching-reference-edge-supported",
    "stopped",
    "stopped-partly",
    "stopping",
    "stream",
    "submission-interrupted",
    "subordinate-printers-supported",
    "subscription-attributes",
    "subscription-description",
    "subscription-object",
    "subscription-template",
    "subunit-added",
    "subunit-almost-empty",
    "subunit-almost-full",
    "subunit-at-limit",
    "subunit-closed",
    "subunit-cooling-down",
    "subunit-empty",
    "subunit-full",
    "subunit-life-almost-over",
    "subunit-life-over",
    "subunit-memory-exhausted",
    "subunit-missing",
    "subunit-motor-failure",
    "subunit-near-limit",
    "subunit-offline",
    "subunit-opened",
    "subunit-over-temperature",
    "subunit-power-saver",
    "subunit-recoverable-failure",
    "subunit-recoverable-storage",
    "subunit-removed",
    "subunit-resource-added",
    "subunit-resource-removed",
    "subunit-thermistor-failure",
    "subunit-timing-Failure",
    "subunit-turned-off",
    "subunit-turned-on",
    "subunit-under-temperature",
    "subunit-unrecoverable-failure",
    "subunit-unrecoverable-storage",
    "subunit-warming-up",
    "successful-ok",
    "successful-ok-conflicting-attributes",
    "successful-ok-events-complete",
    "successful-ok-ignored-or-substituted-attributes",
    "successful-ok-ignored-subscriptions",
    "successful-ok-too-many-events",
    "super-b",
    "suspend-job",
    "system-attributes",
    "system-config-changed",
    "system-description",
    "system-restarted",
    "system-shutdown",
    "system-specified",
    "system-state-changed",
    "system-status",
    "system-stopped",
    "tab",
    "tab-stock",
    "tabloid",
    "tape",
    "text",
    "text-and-graphic",
    "third-shift",
    "time-at-completed",
    "time-at-creation",
    "time-at-processing",
    "timed-out",
    "tls",
    "tobottom-toleft",
    "tobottom-toright",
    "toleft-tobottom",
    "toleft-totop",
    "toner-empty",
    "toner-low",
    "top",
    "toright-tobottom",
    "toright-totop",
    "totop-toleft",
    "totop-toright",
    "tractor",
    "transfer",
    "translucent",
    "transparency",
    "tray-1",
    "tray-10",
    "tray-11",
    "tray-12",
    "tray-13",
    "tray-14",
    "tray-15",
    "tray-16",
    "tray-17",
    "tray-18",
    "tray-19",
    "tray-2",
    "tray-20",
    "tray-3",
    "tray-4",
    "tray-5",
    "tray-6",
    "tray-7",
    "tray-8",
    "tray-9",
    "trim",
    "trim-after-copies",
    "trim-after-documents",
    "trim-after-job",
    "trim-after-pages",
    "trimmer-added",
    "trimmer-almost-empty",
    "trimmer-almost-full",
    "trimmer-at-limit",
    "trimmer-closed",
    "trimmer-configuration-change",
    "trimmer-cover-closed",
    "trimmer-cover-open",
    "trimmer-empty",
    "trimmer-full",
    "trimmer-interlock-closed",
    "trimmer-interlock-open",
    "trimmer-jam",
    "trimmer-life-almost-over",
    "trimmer-life-over",
    "trimmer-memory-exhausted",
    "trimmer-missing",
    "trimmer-motor-failure",
    "trimmer-near-limit",
    "trimmer-offline",
    "trimmer-opened",
    "trimmer-over-temperature",
    "trimmer-power-saver",
    "trimmer-recoverable-failure",
    "trimmer-recoverable-storage",
    "trimmer-removed",
    "trimmer-resource-added",
    "trimmer-resource-removed",
    "trimmer-thermistor-failure",
    "trimmer-timing-failure",
    "trimmer-turned-off",
    "trimmer-turned-on",
    "trimmer-under-temperature",
    "trimmer-unrecoverable-failure",
    "trimmer-unrecoverable-storage-error",
    "trimmer-warming-up",
    "trimming",
    "trimming-offset",
    "trimming-offset-supported",
    "trimming-reference-edge",
    "trimming-reference-edge-supported",
    "trimming-type",
    "trimming-type-supported",
    "trimming-when",
    "trimming-when-supported",
    "triple-wall",
    "turquoise",
    "two-sided-long-edge",
    "two-sided-short-edge",
    "uncalendared",
    "uncollated",
    "uncollated-documents",
    "uncollated-sheets",
    "unknown",
    "unsupported-attributes",
    "unsupported-attributes-or-values",
    "unsupported-compression",
    "unsupported-document-format",
    "uri-authentication-supported",
    "uri-security-supported",
    "username",
    "vellum",
    "velo",
    "violet",
    "waiting-for-user-action",
    "warnings-count",
    "warnings-detected",
    "weekend",
    "wet-film",
    "which-jobs",
    "which-jobs-supported",
    "white",
    "wire",
    "wrap",
    "wrapper-added",
    "wrapper-almost-empty",
    "wrapper-almost-full",
    "wrapper-at-limit",
    "wrapper-closed",
    "wrapper-configuration-change",
    "wrapper-cover-closed",
    "wrapper-cover-open",
    "wrapper-empty",
    "wrapper-full",
    "wrapper-interlock-closed",
    "wrapper-interlock-open",
    "wrapper-jam",
    "wrapper-life-almost-over",
    "wrapper-life-over",
    "wrapper-memory-exhausted",
    "wrapper-missing",
    "wrapper-motor-failure",
    "wrapper-near-limit",
    "wrapper-offline",
    "wrapper-opened",
    "wrapper-over-temperature",
    "wrapper-power-saver",
    "wrapper-recoverable-failure",
    "wrapper-recoverable-storage",
    "wrapper-removed",
    "wrapper-resource-added",
    "wrapper-resource-removed",
    "wrapper-thermistor-failure",
    "wrapper-timing-failure",
    "wrapper-turned-off",
    "wrapper-turned-on",
    "wrapper-under-temperature",
    "wrapper-unrecoverable-failure",
    "wrapper-unrecoverable-storage-error",
    "wrapper-warming-up",
    "x-dimension",
    "x-direction",
    "x-image-position",
    "x-image-position-default",
    "x-image-position-supported",
    "x-image-shift",
    "x-image-shift-actual",
    "x-image-shift-default",
    "x-image-shift-supported",
    "x-side1-image-shift",
    "x-side1-image-shift-actual",
    "x-side1-image-shift-default",
    "x-side1-image-shift-supported",
    "x-side2-image-shift",
    "x-side2-image-shift-actual",
    "x-side2-image-shift-default",
    "x-side2-image-shift-supported",
    "xmldsig",
    "xri-authentication",
    "xri-authentication-supported",
    "xri-security",
    "xri-security-supported",
    "xri-uri",
    "xri-uri-scheme-supported",
    "y-dimension",
    "y-direction",
    "y-image-position",
    "y-image-position-default",
    "y-image-position-supported",
    "y-image-shift",
    "y-image-shift-actual",
    "y-image-shift-default",
    "y-image-shift-supported",
    "y-side1-image-shift",
    "y-side1-image-shift-actual",
    "y-side1-image-shift-default",
    "y-side1-image-shift-supported",
    "y-side2-image-shift",
    "y-side2-image-shift-actual",
    "y-side2-image-shift-default",
    "y-side2-image-shift-supported",
    "yellow"};

// Represents single pair (enum_value,string_index), string_index is an index
// from the kAllStrings array.
struct Val2str {
  uint16_t val;
  uint16_t str;
  bool operator<(const Val2str& o) const { return (val < o.val); }
};

// Represents single pair (string_index,enum_value), string_index is an index
// from the kAllStrings array.
struct Str2val {
  uint16_t str;
  uint16_t val;
  bool operator<(const Str2val& o) const { return (str < o.str); }
};

// Arrays with conversions value->string and string->value for every enum,
// numbers in names are enums ids.
const Val2str kVS_0[] = {};
const Str2val kSV_0[] = {};
const Val2str kVS_1[] = {{1, 1861}, {2, 1319}, {4, 2030}, {5, 2765}, {6, 2607},
                         {7, 670},  {8, 2204}, {9, 580},  {10, 2650}};
const Str2val kSV_1[] = {{580, 9},  {670, 7},  {1319, 2},  {1861, 1}, {2030, 4},
                         {2204, 8}, {2607, 6}, {2650, 10}, {2765, 5}};
const Val2str kVS_2[] = {
    {1, 191},    {2, 192},    {3, 193},    {4, 194},    {5, 218},
    {6, 219},    {7, 220},    {8, 221},    {9, 222},    {10, 303},
    {11, 304},   {12, 305},   {13, 306},   {14, 307},   {15, 343},
    {16, 344},   {17, 404},   {18, 405},   {19, 406},   {20, 407},
    {21, 408},   {22, 412},   {23, 416},   {24, 418},   {25, 424},
    {26, 425},   {27, 426},   {28, 431},   {29, 432},   {30, 433},
    {31, 434},   {32, 437},   {33, 438},   {34, 439},   {35, 440},
    {36, 441},   {37, 442},   {38, 443},   {39, 444},   {40, 446},
    {41, 447},   {42, 448},   {43, 449},   {44, 451},   {45, 488},
    {46, 489},   {47, 490},   {48, 497},   {49, 500},   {50, 501},
    {51, 502},   {52, 579},   {53, 581},   {54, 582},   {55, 583},
    {56, 586},   {57, 589},   {58, 590},   {59, 591},   {60, 593},
    {61, 594},   {62, 595},   {63, 596},   {64, 597},   {65, 598},
    {66, 600},   {67, 601},   {68, 602},   {69, 603},   {70, 604},
    {71, 605},   {72, 606},   {73, 607},   {74, 608},   {75, 609},
    {76, 610},   {77, 611},   {78, 612},   {79, 613},   {80, 614},
    {81, 615},   {82, 616},   {83, 617},   {84, 620},   {85, 623},
    {86, 624},   {87, 625},   {88, 626},   {89, 631},   {90, 667},
    {91, 694},   {92, 695},   {93, 700},   {94, 701},   {95, 702},
    {96, 703},   {97, 704},   {98, 705},   {99, 706},   {100, 707},
    {101, 708},  {102, 709},  {103, 710},  {104, 712},  {105, 770},
    {106, 771},  {107, 772},  {108, 773},  {109, 774},  {110, 775},
    {111, 776},  {112, 779},  {113, 780},  {114, 781},  {115, 782},
    {116, 783},  {117, 784},  {118, 785},  {119, 786},  {120, 793},
    {121, 817},  {122, 818},  {123, 819},  {124, 824},  {125, 825},
    {126, 871},  {127, 872},  {128, 873},  {129, 877},  {130, 878},
    {131, 879},  {132, 880},  {133, 881},  {134, 882},  {135, 883},
    {136, 934},  {137, 936},  {138, 937},  {139, 939},  {140, 1301},
    {141, 1302}, {142, 1303}, {143, 1304}, {144, 1305}, {145, 1306},
    {146, 1307}, {147, 1308}, {148, 1309}, {149, 1310}, {150, 1311},
    {151, 1312}, {152, 1313}, {153, 1314}, {154, 1315}, {155, 1316},
    {156, 1318}, {157, 1320}, {158, 1325}, {159, 1326}, {160, 1332},
    {161, 1333}, {162, 1334}, {163, 1335}, {164, 1336}, {165, 1337},
    {166, 1338}, {167, 1339}, {168, 1340}, {169, 1341}, {170, 1342},
    {171, 1343}, {172, 1344}, {173, 1347}, {174, 1348}, {175, 1350},
    {176, 1351}, {177, 1352}, {178, 1354}, {179, 1356}, {180, 1358},
    {181, 1359}, {182, 1360}, {183, 1361}, {184, 1362}, {185, 1363},
    {186, 1364}, {187, 1365}, {188, 1367}, {189, 1368}, {190, 1369},
    {191, 1370}, {192, 1371}, {193, 1372}, {194, 1373}, {195, 1374},
    {196, 1376}, {197, 1377}, {198, 1379}, {199, 1380}, {200, 1381},
    {201, 1382}, {202, 1383}, {203, 1384}, {204, 1385}, {205, 1386},
    {206, 1389}, {207, 1390}, {208, 1391}, {209, 1392}, {210, 1393},
    {211, 1394}, {212, 1395}, {213, 1396}, {214, 1397}, {215, 1398},
    {216, 1399}, {217, 1400}, {218, 1401}, {219, 1402}, {220, 1403},
    {221, 1404}, {222, 1406}, {223, 1407}, {224, 1408}, {225, 1409},
    {226, 1410}, {227, 1411}, {228, 1412}, {229, 1413}, {230, 1415},
    {231, 1416}, {232, 1417}, {233, 1421}, {234, 1422}, {235, 1424},
    {236, 1425}, {237, 1426}, {238, 1427}, {239, 1431}, {240, 1432},
    {241, 1433}, {242, 1435}, {243, 1436}, {244, 1440}, {245, 1441},
    {246, 1446}, {247, 1447}, {248, 1448}, {249, 1449}, {250, 1450},
    {251, 1451}, {252, 1452}, {253, 1453}, {254, 1454}, {255, 1455},
    {256, 1457}, {257, 1459}, {258, 1461}, {259, 1462}, {260, 1473},
    {261, 1474}, {262, 1476}, {263, 1477}, {264, 1478}, {265, 1504},
    {266, 1506}, {267, 1507}, {268, 1508}, {269, 1509}, {270, 1518},
    {271, 1543}, {272, 1615}, {273, 1620}, {274, 1621}, {275, 1625},
    {276, 1626}, {277, 1627}, {278, 1628}, {279, 1629}, {280, 1630},
    {281, 1631}, {282, 1632}, {283, 1633}, {284, 1634}, {285, 1635},
    {286, 1636}, {287, 1637}, {288, 1639}, {289, 1640}, {290, 1641},
    {291, 1642}, {292, 1643}, {293, 1644}, {294, 1645}, {295, 1646},
    {296, 1647}, {297, 1649}, {298, 1650}, {299, 1651}, {300, 1654},
    {301, 1655}, {302, 1660}, {303, 1661}, {304, 1662}, {305, 1663},
    {306, 1664}, {307, 1665}, {308, 1666}, {309, 1667}, {310, 1668},
    {311, 1669}, {312, 1670}, {313, 1671}, {314, 1672}, {315, 1673},
    {316, 1674}, {317, 1675}, {318, 1676}, {319, 1677}, {320, 1678},
    {321, 1679}, {322, 1680}, {323, 1681}, {324, 1682}, {325, 1683},
    {326, 1684}, {327, 1685}, {328, 1686}, {329, 1688}, {330, 1689},
    {331, 1690}, {332, 1707}, {333, 1708}, {334, 1709}, {335, 1710},
    {336, 1711}, {337, 1712}, {338, 1714}, {339, 1800}, {340, 1811},
    {341, 1812}, {342, 1813}, {343, 1814}, {344, 1815}, {345, 1816},
    {346, 1817}, {347, 1818}, {348, 1819}, {349, 1820}, {350, 1821},
    {351, 1822}, {352, 1823}, {353, 1824}, {354, 1826}, {355, 1862},
    {356, 1864}, {357, 1865}, {358, 1866}, {359, 1867}, {360, 1871},
    {361, 1872}, {362, 1873}, {363, 1874}, {364, 1875}, {365, 1876},
    {366, 1877}, {367, 1878}, {368, 1879}, {369, 1880}, {370, 1884},
    {371, 1885}, {372, 1887}, {373, 1888}, {374, 1889}, {375, 1890},
    {376, 1891}, {377, 1892}, {378, 1894}, {379, 1895}, {380, 1896},
    {381, 1897}, {382, 1898}, {383, 1899}, {384, 1900}, {385, 1901},
    {386, 1903}, {387, 1907}, {388, 1908}, {389, 1909}, {390, 1910},
    {391, 1911}, {392, 1912}, {393, 1913}, {394, 1914}, {395, 1915},
    {396, 1916}, {397, 1917}, {398, 1918}, {399, 1919}, {400, 2006},
    {401, 2007}, {402, 2008}, {403, 2009}, {404, 2012}, {405, 2013},
    {406, 2014}, {407, 2015}, {408, 2016}, {409, 2017}, {410, 2020},
    {411, 2021}, {412, 2022}, {413, 2023}, {414, 2024}, {415, 2025},
    {416, 2027}, {417, 2028}, {418, 2029}, {419, 2031}, {420, 2032},
    {421, 2033}, {422, 2034}, {423, 2036}, {424, 2037}, {425, 2039},
    {426, 2042}, {427, 2043}, {428, 2044}, {429, 2045}, {430, 2046},
    {431, 2047}, {432, 2048}, {433, 2049}, {434, 2051}, {435, 2052},
    {436, 2053}, {437, 2054}, {438, 2055}, {439, 2056}, {440, 2057},
    {441, 2058}, {442, 2059}, {443, 2060}, {444, 2063}, {445, 2064},
    {446, 2065}, {447, 2066}, {448, 2067}, {449, 2068}, {450, 2069},
    {451, 2071}, {452, 2072}, {453, 2073}, {454, 2074}, {455, 2077},
    {456, 2078}, {457, 2079}, {458, 2080}, {459, 2083}, {460, 2084},
    {461, 2085}, {462, 2087}, {463, 2088}, {464, 2089}, {465, 2090},
    {466, 2091}, {467, 2094}, {468, 2095}, {469, 2096}, {470, 2097},
    {471, 2098}, {472, 2099}, {473, 2100}, {474, 2101}, {475, 2102},
    {476, 2103}, {477, 2104}, {478, 2105}, {479, 2112}, {480, 2113},
    {481, 2114}, {482, 2115}, {483, 2116}, {484, 2117}, {485, 2175},
    {486, 2176}, {487, 2177}, {488, 2178}, {489, 2179}, {490, 2180},
    {491, 2181}, {492, 2182}, {493, 2184}, {494, 2185}, {495, 2186},
    {496, 2190}, {497, 2194}, {498, 2197}, {499, 2198}, {500, 2199},
    {501, 2200}, {502, 2201}, {503, 2202}, {504, 2203}, {505, 2242},
    {506, 2243}, {507, 2244}, {508, 2245}, {509, 2246}, {510, 2247},
    {511, 2248}, {512, 2249}, {513, 2250}, {514, 2251}, {515, 2252},
    {516, 2253}, {517, 2302}, {518, 2303}, {519, 2304}, {520, 2305},
    {521, 2338}, {522, 2339}, {523, 2340}, {524, 2341}, {525, 2342},
    {526, 2384}, {527, 2385}, {528, 2386}, {529, 2551}, {530, 2552},
    {531, 2590}, {532, 2591}, {533, 2592}, {534, 2593}, {535, 2594},
    {536, 2595}, {537, 2596}, {538, 2597}, {539, 2598}, {540, 2599},
    {541, 2600}, {542, 2606}, {543, 2666}, {544, 2667}, {545, 2668},
    {546, 2747}, {547, 2748}, {548, 2749}, {549, 2750}, {550, 2751},
    {551, 2752}, {552, 2753}, {553, 2754}, {554, 2755}, {555, 2769},
    {556, 2770}, {557, 2776}, {558, 2780}, {559, 2781}, {560, 2821},
    {561, 2823}, {562, 2824}, {563, 2825}, {564, 2826}, {565, 2827},
    {566, 2828}, {567, 2829}, {568, 2830}, {569, 2831}, {570, 2832},
    {571, 2833}, {572, 2834}, {573, 2835}, {574, 2836}, {575, 2837},
    {576, 2839}, {577, 2840}, {578, 2841}, {579, 2842}, {580, 2843},
    {581, 2844}, {582, 2845}, {583, 2847}, {584, 2848}, {585, 2849},
    {586, 2850}, {587, 2851}, {588, 2852}, {589, 2853}, {590, 2854},
    {591, 2855}, {592, 2856}, {593, 2857}, {594, 2858}, {595, 2859},
    {596, 2860}, {597, 2861}};
const Str2val kSV_2[] = {
    {191, 1},    {192, 2},    {193, 3},    {194, 4},    {218, 5},
    {219, 6},    {220, 7},    {221, 8},    {222, 9},    {303, 10},
    {304, 11},   {305, 12},   {306, 13},   {307, 14},   {343, 15},
    {344, 16},   {404, 17},   {405, 18},   {406, 19},   {407, 20},
    {408, 21},   {412, 22},   {416, 23},   {418, 24},   {424, 25},
    {425, 26},   {426, 27},   {431, 28},   {432, 29},   {433, 30},
    {434, 31},   {437, 32},   {438, 33},   {439, 34},   {440, 35},
    {441, 36},   {442, 37},   {443, 38},   {444, 39},   {446, 40},
    {447, 41},   {448, 42},   {449, 43},   {451, 44},   {488, 45},
    {489, 46},   {490, 47},   {497, 48},   {500, 49},   {501, 50},
    {502, 51},   {579, 52},   {581, 53},   {582, 54},   {583, 55},
    {586, 56},   {589, 57},   {590, 58},   {591, 59},   {593, 60},
    {594, 61},   {595, 62},   {596, 63},   {597, 64},   {598, 65},
    {600, 66},   {601, 67},   {602, 68},   {603, 69},   {604, 70},
    {605, 71},   {606, 72},   {607, 73},   {608, 74},   {609, 75},
    {610, 76},   {611, 77},   {612, 78},   {613, 79},   {614, 80},
    {615, 81},   {616, 82},   {617, 83},   {620, 84},   {623, 85},
    {624, 86},   {625, 87},   {626, 88},   {631, 89},   {667, 90},
    {694, 91},   {695, 92},   {700, 93},   {701, 94},   {702, 95},
    {703, 96},   {704, 97},   {705, 98},   {706, 99},   {707, 100},
    {708, 101},  {709, 102},  {710, 103},  {712, 104},  {770, 105},
    {771, 106},  {772, 107},  {773, 108},  {774, 109},  {775, 110},
    {776, 111},  {779, 112},  {780, 113},  {781, 114},  {782, 115},
    {783, 116},  {784, 117},  {785, 118},  {786, 119},  {793, 120},
    {817, 121},  {818, 122},  {819, 123},  {824, 124},  {825, 125},
    {871, 126},  {872, 127},  {873, 128},  {877, 129},  {878, 130},
    {879, 131},  {880, 132},  {881, 133},  {882, 134},  {883, 135},
    {934, 136},  {936, 137},  {937, 138},  {939, 139},  {1301, 140},
    {1302, 141}, {1303, 142}, {1304, 143}, {1305, 144}, {1306, 145},
    {1307, 146}, {1308, 147}, {1309, 148}, {1310, 149}, {1311, 150},
    {1312, 151}, {1313, 152}, {1314, 153}, {1315, 154}, {1316, 155},
    {1318, 156}, {1320, 157}, {1325, 158}, {1326, 159}, {1332, 160},
    {1333, 161}, {1334, 162}, {1335, 163}, {1336, 164}, {1337, 165},
    {1338, 166}, {1339, 167}, {1340, 168}, {1341, 169}, {1342, 170},
    {1343, 171}, {1344, 172}, {1347, 173}, {1348, 174}, {1350, 175},
    {1351, 176}, {1352, 177}, {1354, 178}, {1356, 179}, {1358, 180},
    {1359, 181}, {1360, 182}, {1361, 183}, {1362, 184}, {1363, 185},
    {1364, 186}, {1365, 187}, {1367, 188}, {1368, 189}, {1369, 190},
    {1370, 191}, {1371, 192}, {1372, 193}, {1373, 194}, {1374, 195},
    {1376, 196}, {1377, 197}, {1379, 198}, {1380, 199}, {1381, 200},
    {1382, 201}, {1383, 202}, {1384, 203}, {1385, 204}, {1386, 205},
    {1389, 206}, {1390, 207}, {1391, 208}, {1392, 209}, {1393, 210},
    {1394, 211}, {1395, 212}, {1396, 213}, {1397, 214}, {1398, 215},
    {1399, 216}, {1400, 217}, {1401, 218}, {1402, 219}, {1403, 220},
    {1404, 221}, {1406, 222}, {1407, 223}, {1408, 224}, {1409, 225},
    {1410, 226}, {1411, 227}, {1412, 228}, {1413, 229}, {1415, 230},
    {1416, 231}, {1417, 232}, {1421, 233}, {1422, 234}, {1424, 235},
    {1425, 236}, {1426, 237}, {1427, 238}, {1431, 239}, {1432, 240},
    {1433, 241}, {1435, 242}, {1436, 243}, {1440, 244}, {1441, 245},
    {1446, 246}, {1447, 247}, {1448, 248}, {1449, 249}, {1450, 250},
    {1451, 251}, {1452, 252}, {1453, 253}, {1454, 254}, {1455, 255},
    {1457, 256}, {1459, 257}, {1461, 258}, {1462, 259}, {1473, 260},
    {1474, 261}, {1476, 262}, {1477, 263}, {1478, 264}, {1504, 265},
    {1506, 266}, {1507, 267}, {1508, 268}, {1509, 269}, {1518, 270},
    {1543, 271}, {1615, 272}, {1620, 273}, {1621, 274}, {1625, 275},
    {1626, 276}, {1627, 277}, {1628, 278}, {1629, 279}, {1630, 280},
    {1631, 281}, {1632, 282}, {1633, 283}, {1634, 284}, {1635, 285},
    {1636, 286}, {1637, 287}, {1639, 288}, {1640, 289}, {1641, 290},
    {1642, 291}, {1643, 292}, {1644, 293}, {1645, 294}, {1646, 295},
    {1647, 296}, {1649, 297}, {1650, 298}, {1651, 299}, {1654, 300},
    {1655, 301}, {1660, 302}, {1661, 303}, {1662, 304}, {1663, 305},
    {1664, 306}, {1665, 307}, {1666, 308}, {1667, 309}, {1668, 310},
    {1669, 311}, {1670, 312}, {1671, 313}, {1672, 314}, {1673, 315},
    {1674, 316}, {1675, 317}, {1676, 318}, {1677, 319}, {1678, 320},
    {1679, 321}, {1680, 322}, {1681, 323}, {1682, 324}, {1683, 325},
    {1684, 326}, {1685, 327}, {1686, 328}, {1688, 329}, {1689, 330},
    {1690, 331}, {1707, 332}, {1708, 333}, {1709, 334}, {1710, 335},
    {1711, 336}, {1712, 337}, {1714, 338}, {1800, 339}, {1811, 340},
    {1812, 341}, {1813, 342}, {1814, 343}, {1815, 344}, {1816, 345},
    {1817, 346}, {1818, 347}, {1819, 348}, {1820, 349}, {1821, 350},
    {1822, 351}, {1823, 352}, {1824, 353}, {1826, 354}, {1862, 355},
    {1864, 356}, {1865, 357}, {1866, 358}, {1867, 359}, {1871, 360},
    {1872, 361}, {1873, 362}, {1874, 363}, {1875, 364}, {1876, 365},
    {1877, 366}, {1878, 367}, {1879, 368}, {1880, 369}, {1884, 370},
    {1885, 371}, {1887, 372}, {1888, 373}, {1889, 374}, {1890, 375},
    {1891, 376}, {1892, 377}, {1894, 378}, {1895, 379}, {1896, 380},
    {1897, 381}, {1898, 382}, {1899, 383}, {1900, 384}, {1901, 385},
    {1903, 386}, {1907, 387}, {1908, 388}, {1909, 389}, {1910, 390},
    {1911, 391}, {1912, 392}, {1913, 393}, {1914, 394}, {1915, 395},
    {1916, 396}, {1917, 397}, {1918, 398}, {1919, 399}, {2006, 400},
    {2007, 401}, {2008, 402}, {2009, 403}, {2012, 404}, {2013, 405},
    {2014, 406}, {2015, 407}, {2016, 408}, {2017, 409}, {2020, 410},
    {2021, 411}, {2022, 412}, {2023, 413}, {2024, 414}, {2025, 415},
    {2027, 416}, {2028, 417}, {2029, 418}, {2031, 419}, {2032, 420},
    {2033, 421}, {2034, 422}, {2036, 423}, {2037, 424}, {2039, 425},
    {2042, 426}, {2043, 427}, {2044, 428}, {2045, 429}, {2046, 430},
    {2047, 431}, {2048, 432}, {2049, 433}, {2051, 434}, {2052, 435},
    {2053, 436}, {2054, 437}, {2055, 438}, {2056, 439}, {2057, 440},
    {2058, 441}, {2059, 442}, {2060, 443}, {2063, 444}, {2064, 445},
    {2065, 446}, {2066, 447}, {2067, 448}, {2068, 449}, {2069, 450},
    {2071, 451}, {2072, 452}, {2073, 453}, {2074, 454}, {2077, 455},
    {2078, 456}, {2079, 457}, {2080, 458}, {2083, 459}, {2084, 460},
    {2085, 461}, {2087, 462}, {2088, 463}, {2089, 464}, {2090, 465},
    {2091, 466}, {2094, 467}, {2095, 468}, {2096, 469}, {2097, 470},
    {2098, 471}, {2099, 472}, {2100, 473}, {2101, 474}, {2102, 475},
    {2103, 476}, {2104, 477}, {2105, 478}, {2112, 479}, {2113, 480},
    {2114, 481}, {2115, 482}, {2116, 483}, {2117, 484}, {2175, 485},
    {2176, 486}, {2177, 487}, {2178, 488}, {2179, 489}, {2180, 490},
    {2181, 491}, {2182, 492}, {2184, 493}, {2185, 494}, {2186, 495},
    {2190, 496}, {2194, 497}, {2197, 498}, {2198, 499}, {2199, 500},
    {2200, 501}, {2201, 502}, {2202, 503}, {2203, 504}, {2242, 505},
    {2243, 506}, {2244, 507}, {2245, 508}, {2246, 509}, {2247, 510},
    {2248, 511}, {2249, 512}, {2250, 513}, {2251, 514}, {2252, 515},
    {2253, 516}, {2302, 517}, {2303, 518}, {2304, 519}, {2305, 520},
    {2338, 521}, {2339, 522}, {2340, 523}, {2341, 524}, {2342, 525},
    {2384, 526}, {2385, 527}, {2386, 528}, {2551, 529}, {2552, 530},
    {2590, 531}, {2591, 532}, {2592, 533}, {2593, 534}, {2594, 535},
    {2595, 536}, {2596, 537}, {2597, 538}, {2598, 539}, {2599, 540},
    {2600, 541}, {2606, 542}, {2666, 543}, {2667, 544}, {2668, 545},
    {2747, 546}, {2748, 547}, {2749, 548}, {2750, 549}, {2751, 550},
    {2752, 551}, {2753, 552}, {2754, 553}, {2755, 554}, {2769, 555},
    {2770, 556}, {2776, 557}, {2780, 558}, {2781, 559}, {2821, 560},
    {2823, 561}, {2824, 562}, {2825, 563}, {2826, 564}, {2827, 565},
    {2828, 566}, {2829, 567}, {2830, 568}, {2831, 569}, {2832, 570},
    {2833, 571}, {2834, 572}, {2835, 573}, {2836, 574}, {2837, 575},
    {2839, 576}, {2840, 577}, {2841, 578}, {2842, 579}, {2843, 580},
    {2844, 581}, {2845, 582}, {2847, 583}, {2848, 584}, {2849, 585},
    {2850, 586}, {2851, 587}, {2852, 588}, {2853, 589}, {2854, 590},
    {2855, 591}, {2856, 592}, {2857, 593}, {2858, 594}, {2859, 595},
    {2860, 596}, {2861, 597}};
const Val2str kVS_3[] = {{0, 632}, {1, 1807}, {2, 1905}, {3, 2771}};
const Str2val kSV_3[] = {{632, 0}, {1807, 1}, {1905, 2}, {2771, 3}};
const Val2str kVS_4[] = {{0, 223}, {1, 2381}, {2, 2784}};
const Str2val kSV_4[] = {{223, 0}, {2381, 1}, {2784, 2}};
const Val2str kVS_5[] = {{0, 140}, {1, 141}};
const Str2val kSV_5[] = {{140, 0}, {141, 1}};
const Val2str kVS_6[] = {{0, 318}, {1, 1521}, {2, 2222}, {3, 2677}};
const Str2val kSV_6[] = {{318, 0}, {1521, 1}, {2222, 2}, {2677, 3}};
const Val2str kVS_7[] = {{0, 132},  {1, 413},  {2, 715},  {3, 1886},
                         {4, 1923}, {5, 2436}, {6, 2662}, {7, 2773}};
const Str2val kSV_7[] = {{132, 0},  {413, 1},  {715, 2},  {1886, 3},
                         {1923, 4}, {2436, 5}, {2662, 6}, {2773, 7}};
const Val2str kVS_8[] = {{0, 215}, {1, 316}, {2, 787}};
const Str2val kSV_8[] = {{215, 0}, {316, 1}, {787, 2}};
const Val2str kVS_9[] = {{0, 185}, {1, 186},  {2, 187},  {3, 188},  {4, 799},
                         {5, 812}, {6, 1619}, {7, 2263}, {8, 2388}, {9, 2684}};
const Str2val kSV_9[] = {{185, 0}, {186, 1},  {187, 2},  {188, 3},  {799, 4},
                         {812, 5}, {1619, 6}, {2263, 7}, {2388, 8}, {2684, 9}};
const Val2str kVS_10[] = {{0, 415}, {1, 494}, {2, 808}, {3, 1807}};
const Str2val kSV_10[] = {{415, 0}, {494, 1}, {808, 2}, {1807, 3}};
const Val2str kVS_11[] = {{0, 446}, {1, 1625}, {2, 1630}};
const Str2val kSV_11[] = {{446, 0}, {1625, 1}, {1630, 2}};
const Val2str kVS_12[] = {
    {0, 1804}, {1, 2010}, {2, 2011}, {3, 2018}, {4, 2019}};
const Str2val kSV_12[] = {
    {1804, 0}, {2010, 1}, {2011, 2}, {2018, 3}, {2019, 4}};
const Val2str kVS_13[] = {{0, 1972}, {1, 2002}, {2, 2004}};
const Str2val kSV_13[] = {{1972, 0}, {2002, 1}, {2004, 2}};
const Val2str kVS_14[] = {{0, 0}, {1, 1716}};
const Str2val kSV_14[] = {{0, 0}, {1716, 1}};
const Val2str kVS_15[] = {{0, 638}, {1, 1807}, {2, 1961}, {3, 2432}, {4, 2838}};
const Str2val kSV_15[] = {{638, 0}, {1807, 1}, {1961, 2}, {2432, 3}, {2838, 4}};
const Val2str kVS_16[] = {{0, 593}, {1, 598}, {2, 604}, {3, 612},
                          {4, 623}, {5, 624}, {6, 625}, {7, 626}};
const Str2val kSV_16[] = {{593, 0}, {598, 1}, {604, 2}, {612, 3},
                          {623, 4}, {624, 5}, {625, 6}, {626, 7}};
const Val2str kVS_17[] = {{0, 1807}};
const Str2val kSV_17[] = {{1807, 0}};
const Val2str kVS_18[] = {{0, 1545}, {1, 2379}};
const Str2val kSV_18[] = {{1545, 0}, {2379, 1}};
const Val2str kVS_19[] = {
    {0, 217},    {1, 262},    {2, 263},    {3, 264},    {4, 265},
    {5, 266},    {6, 315},    {7, 403},    {8, 436},    {9, 647},
    {10, 648},   {11, 649},   {12, 650},   {13, 651},   {14, 721},
    {15, 722},   {16, 723},   {17, 724},   {18, 725},   {19, 726},
    {20, 727},   {21, 728},   {22, 729},   {23, 730},   {24, 731},
    {25, 732},   {26, 733},   {27, 1205},  {28, 1231},  {29, 1232},
    {30, 1236},  {31, 1237},  {32, 1238},  {33, 1239},  {34, 1240},
    {35, 1241},  {36, 1242},  {37, 1243},  {38, 1246},  {39, 1247},
    {40, 1248},  {41, 1249},  {42, 1250},  {43, 1251},  {44, 1252},
    {45, 1164},  {46, 1165},  {47, 1166},  {48, 1167},  {49, 1173},
    {50, 1174},  {51, 1175},  {52, 1176},  {53, 1177},  {54, 1178},
    {55, 1179},  {56, 1180},  {57, 1168},  {58, 1169},  {59, 1170},
    {60, 1171},  {61, 1172},  {62, 1181},  {63, 1182},  {64, 1188},
    {65, 1189},  {66, 1190},  {67, 1191},  {68, 1192},  {69, 1193},
    {70, 1194},  {71, 1195},  {72, 1183},  {73, 1184},  {74, 1185},
    {75, 1186},  {76, 1187},  {77, 1196},  {78, 1197},  {79, 1198},
    {80, 1199},  {81, 1200},  {82, 1201},  {83, 1202},  {84, 1203},
    {85, 1204},  {86, 1206},  {87, 1207},  {88, 1208},  {89, 1211},
    {90, 1212},  {91, 1213},  {92, 1214},  {93, 1215},  {94, 1216},
    {95, 1217},  {96, 1218},  {97, 1209},  {98, 1210},  {99, 1219},
    {100, 1220}, {101, 1221}, {102, 1222}, {103, 1223}, {104, 1224},
    {105, 1225}, {106, 1226}, {107, 1227}, {108, 1228}, {109, 1229},
    {110, 1230}, {111, 1233}, {112, 1234}, {113, 1235}, {114, 1244},
    {115, 1245}, {116, 1475}, {117, 1503}, {118, 2118}, {119, 2119},
    {120, 2120}, {121, 2121}, {122, 2122}, {123, 2123}, {124, 2124},
    {125, 2125}, {126, 2126}, {127, 2127}, {128, 2128}, {129, 2129},
    {130, 2130}, {131, 2131}, {132, 2132}, {133, 2133}, {134, 2134},
    {135, 2135}, {136, 2136}, {137, 2137}, {138, 2138}, {139, 2237},
    {140, 2489}, {141, 2490}, {142, 2491}, {143, 2492}, {144, 2493},
    {145, 2494}, {146, 2495}, {147, 2496}, {148, 2497}, {149, 2498},
    {150, 2499}, {151, 2500}, {152, 2501}, {153, 2706}, {154, 2707},
    {155, 2708}, {156, 2709}, {157, 2710}};
const Str2val kSV_19[] = {
    {217, 0},    {262, 1},    {263, 2},    {264, 3},    {265, 4},
    {266, 5},    {315, 6},    {403, 7},    {436, 8},    {647, 9},
    {648, 10},   {649, 11},   {650, 12},   {651, 13},   {721, 14},
    {722, 15},   {723, 16},   {724, 17},   {725, 18},   {726, 19},
    {727, 20},   {728, 21},   {729, 22},   {730, 23},   {731, 24},
    {732, 25},   {733, 26},   {1164, 45},  {1165, 46},  {1166, 47},
    {1167, 48},  {1168, 57},  {1169, 58},  {1170, 59},  {1171, 60},
    {1172, 61},  {1173, 49},  {1174, 50},  {1175, 51},  {1176, 52},
    {1177, 53},  {1178, 54},  {1179, 55},  {1180, 56},  {1181, 62},
    {1182, 63},  {1183, 72},  {1184, 73},  {1185, 74},  {1186, 75},
    {1187, 76},  {1188, 64},  {1189, 65},  {1190, 66},  {1191, 67},
    {1192, 68},  {1193, 69},  {1194, 70},  {1195, 71},  {1196, 77},
    {1197, 78},  {1198, 79},  {1199, 80},  {1200, 81},  {1201, 82},
    {1202, 83},  {1203, 84},  {1204, 85},  {1205, 27},  {1206, 86},
    {1207, 87},  {1208, 88},  {1209, 97},  {1210, 98},  {1211, 89},
    {1212, 90},  {1213, 91},  {1214, 92},  {1215, 93},  {1216, 94},
    {1217, 95},  {1218, 96},  {1219, 99},  {1220, 100}, {1221, 101},
    {1222, 102}, {1223, 103}, {1224, 104}, {1225, 105}, {1226, 106},
    {1227, 107}, {1228, 108}, {1229, 109}, {1230, 110}, {1231, 28},
    {1232, 29},  {1233, 111}, {1234, 112}, {1235, 113}, {1236, 30},
    {1237, 31},  {1238, 32},  {1239, 33},  {1240, 34},  {1241, 35},
    {1242, 36},  {1243, 37},  {1244, 114}, {1245, 115}, {1246, 38},
    {1247, 39},  {1248, 40},  {1249, 41},  {1250, 42},  {1251, 43},
    {1252, 44},  {1475, 116}, {1503, 117}, {2118, 118}, {2119, 119},
    {2120, 120}, {2121, 121}, {2122, 122}, {2123, 123}, {2124, 124},
    {2125, 125}, {2126, 126}, {2127, 127}, {2128, 128}, {2129, 129},
    {2130, 130}, {2131, 131}, {2132, 132}, {2133, 133}, {2134, 134},
    {2135, 135}, {2136, 136}, {2137, 137}, {2138, 138}, {2237, 139},
    {2489, 140}, {2490, 141}, {2491, 142}, {2492, 143}, {2493, 144},
    {2494, 145}, {2495, 146}, {2496, 147}, {2497, 148}, {2498, 149},
    {2499, 150}, {2500, 151}, {2501, 152}, {2706, 153}, {2707, 154},
    {2708, 155}, {2709, 156}, {2710, 157}};
const Val2str kVS_20[] = {
    {3, 1807},  {4, 2489},  {5, 2118},  {6, 436},   {7, 262},   {8, 2237},
    {9, 647},   {10, 721},  {11, 2706}, {12, 217},  {13, 315},  {14, 1475},
    {15, 403},  {16, 1503}, {20, 2496}, {21, 2490}, {22, 2497}, {23, 2491},
    {24, 649},  {25, 651},  {26, 650},  {27, 648},  {28, 2493}, {29, 2495},
    {30, 2494}, {31, 2492}, {32, 2499}, {33, 2501}, {34, 2500}, {35, 2498},
    {50, 264},  {51, 266},  {52, 265},  {53, 263},  {60, 2710}, {61, 2708},
    {62, 2707}, {63, 2709}, {70, 2133}, {71, 2119}, {72, 2134}, {73, 2120},
    {74, 2122}, {75, 2124}, {76, 2123}, {77, 2121}, {78, 2136}, {79, 2138},
    {80, 2137}, {81, 2135}, {82, 2130}, {83, 2132}, {84, 2131}, {85, 2129},
    {86, 2126}, {87, 2128}, {88, 2127}, {89, 2125}, {90, 722},  {91, 723},
    {92, 725},  {93, 726},  {94, 727},  {95, 728},  {96, 729},  {97, 730},
    {98, 731},  {99, 732},  {100, 733}, {101, 724}};
const Str2val kSV_20[] = {
    {217, 12},  {262, 7},   {263, 53},  {264, 50},  {265, 52},  {266, 51},
    {315, 13},  {403, 15},  {436, 6},   {647, 9},   {648, 27},  {649, 24},
    {650, 26},  {651, 25},  {721, 10},  {722, 90},  {723, 91},  {724, 101},
    {725, 92},  {726, 93},  {727, 94},  {728, 95},  {729, 96},  {730, 97},
    {731, 98},  {732, 99},  {733, 100}, {1475, 14}, {1503, 16}, {1807, 3},
    {2118, 5},  {2119, 71}, {2120, 73}, {2121, 77}, {2122, 74}, {2123, 76},
    {2124, 75}, {2125, 89}, {2126, 86}, {2127, 88}, {2128, 87}, {2129, 85},
    {2130, 82}, {2131, 84}, {2132, 83}, {2133, 70}, {2134, 72}, {2135, 81},
    {2136, 78}, {2137, 80}, {2138, 79}, {2237, 8},  {2489, 4},  {2490, 21},
    {2491, 23}, {2492, 31}, {2493, 28}, {2494, 30}, {2495, 29}, {2496, 20},
    {2497, 22}, {2498, 35}, {2499, 32}, {2500, 34}, {2501, 33}, {2706, 11},
    {2707, 62}, {2708, 61}, {2709, 63}, {2710, 60}};
const Val2str kVS_21[] = {{0, 932}, {1, 1883}};
const Str2val kSV_21[] = {{932, 0}, {1883, 1}};
const Val2str kVS_22[] = {{0, 578}, {1, 714}, {2, 2434}, {3, 2435}};
const Str2val kSV_22[] = {{578, 0}, {714, 1}, {2434, 2}, {2435, 3}};
const Val2str kVS_23[] = {{0, 1807}, {1, 2387}};
const Str2val kSV_23[] = {{1807, 0}, {2387, 1}};
const Val2str kVS_24[] = {
    {3, 1988}, {4, 1513}, {5, 2216}, {6, 2219}, {7, 1807}};
const Str2val kSV_24[] = {
    {1513, 4}, {1807, 7}, {1988, 3}, {2216, 5}, {2219, 6}};
const Val2str kVS_25[] = {{3, 635}, {4, 1808}, {5, 811}};
const Str2val kSV_25[] = {{635, 3}, {811, 5}, {1808, 4}};
const Val2str kVS_26[] = {{0, 1858}, {1, 2758}, {2, 2759}};
const Str2val kSV_26[] = {{1858, 0}, {2758, 1}, {2759, 2}};
const Val2str kVS_27[] = {{0, 618},  {1, 693},  {2, 816},   {3, 863},
                          {4, 933},  {5, 935},  {6, 1439},  {7, 1807},
                          {8, 1893}, {9, 2114}, {10, 2256}, {11, 2609}};
const Str2val kSV_27[] = {{618, 0},  {693, 1},  {816, 2},   {863, 3},
                          {933, 4},  {935, 5},  {1439, 6},  {1807, 7},
                          {1893, 8}, {2114, 9}, {2256, 10}, {2609, 11}};
const Val2str kVS_28[] = {{256, 1}, {257, 2}, {512, 3}, {513, 4}, {514, 5}};
const Str2val kSV_28[] = {{1, 256}, {2, 257}, {3, 512}, {4, 513}, {5, 514}};
const Val2str kVS_29[] = {{0, 792}, {1, 806}, {2, 1807}};
const Str2val kSV_29[] = {{792, 0}, {806, 1}, {1807, 2}};
const Val2str kVS_30[] = {
    {0, 195},   {1, 318},   {2, 336},   {3, 690},   {4, 691},   {5, 1514},
    {6, 1521},  {7, 1547},  {8, 1549},  {9, 1550},  {10, 1551}, {11, 1552},
    {12, 1553}, {13, 1554}, {14, 1555}, {15, 1556}, {16, 1548}, {17, 1697},
    {18, 1715}, {19, 2191}, {20, 2222}, {21, 2383}, {22, 2442}, {23, 2444},
    {24, 2445}, {25, 2446}, {26, 2447}, {27, 2448}, {28, 2449}, {29, 2450},
    {30, 2451}, {31, 2443}, {32, 2677}, {33, 2686}, {34, 2697}, {35, 2699},
    {36, 2700}, {37, 2701}, {38, 2702}, {39, 2703}, {40, 2704}, {41, 2705},
    {42, 2687}};
const Str2val kSV_30[] = {
    {195, 0},   {318, 1},   {336, 2},   {690, 3},   {691, 4},   {1514, 5},
    {1521, 6},  {1547, 7},  {1548, 16}, {1549, 8},  {1550, 9},  {1551, 10},
    {1552, 11}, {1553, 12}, {1554, 13}, {1555, 14}, {1556, 15}, {1697, 17},
    {1715, 18}, {2191, 19}, {2222, 20}, {2383, 21}, {2442, 22}, {2443, 31},
    {2444, 23}, {2445, 24}, {2446, 25}, {2447, 26}, {2448, 27}, {2449, 28},
    {2450, 29}, {2451, 30}, {2677, 32}, {2686, 33}, {2687, 42}, {2697, 34},
    {2699, 35}, {2700, 36}, {2701, 37}, {2702, 38}, {2703, 39}, {2704, 40},
    {2705, 41}};
const Val2str kVS_31[] = {{0, 1807}, {1, 2488}};
const Str2val kSV_31[] = {{1807, 0}, {2488, 1}};
const Val2str kVS_32[] = {{3, 2763}, {4, 410}, {5, 2762}};
const Str2val kSV_32[] = {{410, 4}, {2762, 5}, {2763, 3}};
const Val2str kVS_33[] = {{0, 491},  {1, 669},  {2, 862},  {3, 1802},
                          {4, 1805}, {5, 2260}, {6, 2665}, {7, 2778}};
const Str2val kSV_33[] = {{491, 0},  {669, 1},  {862, 2},  {1802, 3},
                          {1805, 4}, {2260, 5}, {2665, 6}, {2778, 7}};
const Val2str kVS_34[] = {{0, 124}, {1, 331}, {2, 427}, {3, 2649}};
const Str2val kSV_34[] = {{124, 0}, {331, 1}, {427, 2}, {2649, 3}};
const Val2str kVS_35[] = {{0, 148}, {1, 1857}};
const Str2val kSV_35[] = {{148, 0}, {1857, 1}};
const Val2str kVS_36[] = {{0, 491},  {1, 669},  {2, 862},  {3, 1802},
                          {4, 1806}, {5, 2260}, {6, 2665}, {7, 2778}};
const Str2val kSV_36[] = {{491, 0},  {669, 1},  {862, 2},  {1802, 3},
                          {1806, 4}, {2260, 5}, {2665, 6}, {2778, 7}};
const Val2str kVS_37[] = {};
const Str2val kSV_37[] = {};
const Val2str kVS_38[] = {{0, 1622},  {1, 1623},  {2, 1624},  {3, 1807},
                          {4, 2323},  {5, 2324},  {6, 2325},  {7, 2326},
                          {8, 2327},  {9, 2328},  {10, 2329}, {11, 2330},
                          {12, 2331}, {13, 2332}, {14, 2333}, {15, 2334},
                          {16, 2335}, {17, 2336}, {18, 2337}};
const Str2val kSV_38[] = {{1622, 0},  {1623, 1},  {1624, 2},  {1807, 3},
                          {2323, 4},  {2324, 5},  {2325, 6},  {2326, 7},
                          {2327, 8},  {2328, 9},  {2329, 10}, {2330, 11},
                          {2331, 12}, {2332, 13}, {2333, 14}, {2334, 15},
                          {2335, 16}, {2336, 17}, {2337, 18}};
const Val2str kVS_39[] = {{0, 711},  {1, 1321}, {2, 1357},
                          {3, 1458}, {4, 1807}, {5, 2488}};
const Str2val kSV_39[] = {{711, 0},  {1321, 1}, {1357, 2},
                          {1458, 3}, {1807, 4}, {2488, 5}};
const Val2str kVS_40[] = {{0, 207}, {1, 2437}, {2, 2604}};
const Str2val kSV_40[] = {{207, 0}, {2437, 1}, {2604, 2}};
const Val2str kVS_41[] = {{3, 1920}, {4, 1921}, {5, 2109}, {6, 2110},
                          {7, 332},  {8, 125},  {9, 414}};
const Str2val kSV_41[] = {{125, 8},  {332, 7},  {414, 9}, {1920, 3},
                          {1921, 4}, {2109, 5}, {2110, 6}};
const Val2str kVS_42[] = {
    {0, 126},   {1, 128},   {2, 129},   {3, 130},   {4, 131},   {5, 417},
    {6, 420},   {7, 421},   {8, 422},   {9, 496},   {10, 570},  {11, 571},
    {12, 579},  {13, 599},  {14, 619},  {15, 621},  {16, 622},  {17, 630},
    {18, 668},  {19, 1322}, {20, 1323}, {21, 1324}, {22, 1328}, {23, 1329},
    {24, 1330}, {25, 1346}, {26, 1349}, {27, 1355}, {28, 1366}, {29, 1375},
    {30, 1378}, {31, 1387}, {32, 1388}, {33, 1405}, {34, 1414}, {35, 1418},
    {36, 1419}, {37, 1420}, {38, 1423}, {39, 1429}, {40, 1430}, {41, 1434},
    {42, 1437}, {43, 1438}, {44, 1442}, {45, 1443}, {46, 1444}, {47, 1445},
    {48, 1456}, {49, 1464}, {50, 1465}, {51, 1466}, {52, 1467}, {53, 1468},
    {54, 1469}, {55, 1471}, {56, 1472}, {57, 1807}, {58, 2092}, {59, 2093},
    {60, 2111}, {61, 2189}, {62, 2213}, {63, 2214}, {64, 2319}, {65, 2605},
    {66, 2766}, {67, 2767}, {68, 2768}, {69, 2775}, {70, 2777}};
const Str2val kSV_42[] = {
    {126, 0},   {128, 1},   {129, 2},   {130, 3},   {131, 4},   {417, 5},
    {420, 6},   {421, 7},   {422, 8},   {496, 9},   {570, 10},  {571, 11},
    {579, 12},  {599, 13},  {619, 14},  {621, 15},  {622, 16},  {630, 17},
    {668, 18},  {1322, 19}, {1323, 20}, {1324, 21}, {1328, 22}, {1329, 23},
    {1330, 24}, {1346, 25}, {1349, 26}, {1355, 27}, {1366, 28}, {1375, 29},
    {1378, 30}, {1387, 31}, {1388, 32}, {1405, 33}, {1414, 34}, {1418, 35},
    {1419, 36}, {1420, 37}, {1423, 38}, {1429, 39}, {1430, 40}, {1434, 41},
    {1437, 42}, {1438, 43}, {1442, 44}, {1443, 45}, {1444, 46}, {1445, 47},
    {1456, 48}, {1464, 49}, {1465, 50}, {1466, 51}, {1467, 52}, {1468, 53},
    {1469, 54}, {1471, 55}, {1472, 56}, {1807, 57}, {2092, 58}, {2093, 59},
    {2111, 60}, {2189, 61}, {2213, 62}, {2214, 63}, {2319, 64}, {2605, 65},
    {2766, 66}, {2767, 67}, {2768, 68}, {2775, 69}, {2777, 70}};
const Val2str kVS_43[] = {{0, 185},  {1, 799},  {2, 812},
                          {3, 1619}, {4, 2263}, {5, 2684}};
const Str2val kSV_43[] = {{185, 0},  {799, 1},  {812, 2},
                          {1619, 3}, {2263, 4}, {2684, 5}};
const Val2str kVS_44[] = {
    {0, 308},   {1, 313},   {2, 319},   {3, 320},   {4, 348},   {5, 349},
    {6, 350},   {7, 351},   {8, 352},   {9, 353},   {10, 354},  {11, 355},
    {12, 356},  {13, 357},  {14, 358},  {15, 359},  {16, 360},  {17, 361},
    {18, 362},  {19, 363},  {20, 364},  {21, 365},  {22, 366},  {23, 367},
    {24, 368},  {25, 465},  {26, 470},  {27, 471},  {28, 472},  {29, 473},
    {30, 474},  {31, 475},  {32, 476},  {33, 477},  {34, 478},  {35, 479},
    {36, 480},  {37, 481},  {38, 482},  {39, 483},  {40, 484},  {41, 485},
    {42, 486},  {43, 487},  {44, 800},  {45, 801},  {46, 804},  {47, 805},
    {48, 1163}, {49, 1524}, {50, 1525}, {51, 1526}, {52, 1527}, {53, 1528},
    {54, 1529}, {55, 1530}, {56, 1531}, {57, 1532}, {58, 1533}, {59, 1534},
    {60, 1535}, {61, 1536}, {62, 1537}, {63, 1538}, {64, 1539}, {65, 1540},
    {66, 1541}, {67, 1542}, {68, 1546}, {69, 1704}, {70, 1713}, {71, 1803},
    {72, 1863}, {73, 1971}, {74, 2193}, {75, 2389}, {76, 2757}, {77, 2774},
    {78, 2782}, {79, 2862}};
const Str2val kSV_44[] = {
    {308, 0},   {313, 1},   {319, 2},   {320, 3},   {348, 4},   {349, 5},
    {350, 6},   {351, 7},   {352, 8},   {353, 9},   {354, 10},  {355, 11},
    {356, 12},  {357, 13},  {358, 14},  {359, 15},  {360, 16},  {361, 17},
    {362, 18},  {363, 19},  {364, 20},  {365, 21},  {366, 22},  {367, 23},
    {368, 24},  {465, 25},  {470, 26},  {471, 27},  {472, 28},  {473, 29},
    {474, 30},  {475, 31},  {476, 32},  {477, 33},  {478, 34},  {479, 35},
    {480, 36},  {481, 37},  {482, 38},  {483, 39},  {484, 40},  {485, 41},
    {486, 42},  {487, 43},  {800, 44},  {801, 45},  {804, 46},  {805, 47},
    {1163, 48}, {1524, 49}, {1525, 50}, {1526, 51}, {1527, 52}, {1528, 53},
    {1529, 54}, {1530, 55}, {1531, 56}, {1532, 57}, {1533, 58}, {1534, 59},
    {1535, 60}, {1536, 61}, {1537, 62}, {1538, 63}, {1539, 64}, {1540, 65},
    {1541, 66}, {1542, 67}, {1546, 68}, {1704, 69}, {1713, 70}, {1803, 71},
    {1863, 72}, {1971, 73}, {2193, 74}, {2389, 75}, {2757, 76}, {2774, 77},
    {2782, 78}, {2862, 79}};
const Val2str kVS_45[] = {
    {0, 120},    {1, 121},    {2, 122},    {3, 123},    {4, 150},
    {5, 151},    {6, 152},    {7, 153},    {8, 154},    {9, 155},
    {10, 156},   {11, 157},   {12, 158},   {13, 159},   {14, 160},
    {15, 161},   {16, 162},   {17, 163},   {18, 164},   {19, 165},
    {20, 166},   {21, 167},   {22, 168},   {23, 169},   {24, 170},
    {25, 171},   {26, 172},   {27, 173},   {28, 174},   {29, 175},
    {30, 176},   {31, 177},   {32, 178},   {33, 179},   {34, 180},
    {35, 181},   {36, 182},   {37, 183},   {38, 184},   {39, 189},
    {40, 197},   {41, 198},   {42, 199},   {43, 201},   {44, 202},
    {45, 203},   {46, 204},   {47, 205},   {48, 206},   {49, 208},
    {50, 209},   {51, 210},   {52, 211},   {53, 212},   {54, 213},
    {55, 214},   {56, 314},   {57, 318},   {58, 321},   {59, 322},
    {60, 323},   {61, 324},   {62, 325},   {63, 326},   {64, 327},
    {65, 328},   {66, 345},   {67, 452},   {68, 454},   {69, 455},
    {70, 456},   {71, 457},   {72, 458},   {73, 459},   {74, 460},
    {75, 461},   {76, 453},   {77, 462},   {78, 463},   {79, 464},
    {80, 466},   {81, 467},   {82, 468},   {83, 469},   {84, 493},
    {85, 640},   {86, 641},   {87, 642},   {88, 643},   {89, 644},
    {90, 645},   {91, 646},   {92, 655},   {93, 671},   {94, 672},
    {95, 673},   {96, 674},   {97, 675},   {98, 682},   {99, 777},
    {100, 778},  {101, 810},  {102, 930},  {103, 931},  {104, 951},
    {105, 952},  {106, 953},  {107, 954},  {108, 955},  {109, 956},
    {110, 957},  {111, 958},  {112, 959},  {113, 960},  {114, 961},
    {115, 964},  {116, 965},  {117, 966},  {118, 967},  {119, 968},
    {120, 969},  {121, 970},  {122, 971},  {123, 972},  {124, 973},
    {125, 974},  {126, 975},  {127, 976},  {128, 977},  {129, 978},
    {130, 979},  {131, 980},  {132, 981},  {133, 982},  {134, 983},
    {135, 984},  {136, 985},  {137, 986},  {138, 987},  {139, 988},
    {140, 989},  {141, 990},  {142, 991},  {143, 992},  {144, 993},
    {145, 994},  {146, 995},  {147, 996},  {148, 997},  {149, 998},
    {150, 999},  {151, 1000}, {152, 1001}, {153, 1002}, {154, 1003},
    {155, 1004}, {156, 1005}, {157, 1006}, {158, 1007}, {159, 1008},
    {160, 1009}, {161, 1010}, {162, 1011}, {163, 1012}, {164, 1013},
    {165, 1014}, {166, 1015}, {167, 1016}, {168, 1017}, {169, 1018},
    {170, 1019}, {171, 1020}, {172, 1021}, {173, 1022}, {174, 1023},
    {175, 1024}, {176, 1025}, {177, 1026}, {178, 1027}, {179, 1028},
    {180, 1029}, {181, 1030}, {182, 1031}, {183, 1032}, {184, 1033},
    {185, 1034}, {186, 1035}, {187, 1036}, {188, 1037}, {189, 1038},
    {190, 1039}, {191, 1040}, {192, 1041}, {193, 1042}, {194, 1043},
    {195, 1044}, {196, 1045}, {197, 1046}, {198, 1047}, {199, 1048},
    {200, 1049}, {201, 1050}, {202, 1051}, {203, 1052}, {204, 1053},
    {205, 962},  {206, 963},  {207, 1054}, {208, 1055}, {209, 1056},
    {210, 1057}, {211, 1060}, {212, 1061}, {213, 1062}, {214, 1063},
    {215, 1064}, {216, 1065}, {217, 1066}, {218, 1067}, {219, 1068},
    {220, 1069}, {221, 1070}, {222, 1071}, {223, 1072}, {224, 1073},
    {225, 1074}, {226, 1075}, {227, 1076}, {228, 1077}, {229, 1078},
    {230, 1079}, {231, 1058}, {232, 1059}, {233, 1080}, {234, 1081},
    {235, 1082}, {236, 1083}, {237, 1084}, {238, 1085}, {239, 1086},
    {240, 1087}, {241, 1088}, {242, 1089}, {243, 1090}, {244, 1091},
    {245, 1094}, {246, 1095}, {247, 1096}, {248, 1097}, {249, 1098},
    {250, 1099}, {251, 1100}, {252, 1101}, {253, 1102}, {254, 1092},
    {255, 1103}, {256, 1104}, {257, 1105}, {258, 1106}, {259, 1107},
    {260, 1108}, {261, 1109}, {262, 1110}, {263, 1111}, {264, 1112},
    {265, 1113}, {266, 1114}, {267, 1115}, {268, 1116}, {269, 1117},
    {270, 1118}, {271, 1119}, {272, 1120}, {273, 1121}, {274, 1122},
    {275, 1123}, {276, 1093}, {277, 1124}, {278, 1126}, {279, 1127},
    {280, 1128}, {281, 1129}, {282, 1130}, {283, 1131}, {284, 1132},
    {285, 1133}, {286, 1134}, {287, 1135}, {288, 1136}, {289, 1125},
    {290, 1137}, {291, 1139}, {292, 1140}, {293, 1141}, {294, 1142},
    {295, 1143}, {296, 1144}, {297, 1145}, {298, 1146}, {299, 1147},
    {300, 1148}, {301, 1149}, {302, 1138}, {303, 1150}, {304, 1151},
    {305, 1152}, {306, 1153}, {307, 1154}, {308, 1155}, {309, 1156},
    {310, 1157}, {311, 1158}, {312, 1159}, {313, 1160}, {314, 1161},
    {315, 1162}, {316, 1253}, {317, 1254}, {318, 1255}, {319, 1256},
    {320, 1257}, {321, 1258}, {322, 1259}, {323, 1260}, {324, 1263},
    {325, 1264}, {326, 1265}, {327, 1266}, {328, 1267}, {329, 1268},
    {330, 1269}, {331, 1270}, {332, 1271}, {333, 1272}, {334, 1273},
    {335, 1274}, {336, 1275}, {337, 1276}, {338, 1277}, {339, 1278},
    {340, 1279}, {341, 1280}, {342, 1281}, {343, 1282}, {344, 1283},
    {345, 1284}, {346, 1285}, {347, 1286}, {348, 1287}, {349, 1288},
    {350, 1261}, {351, 1262}, {352, 1289}, {353, 1291}, {354, 1292},
    {355, 1293}, {356, 1294}, {357, 1295}, {358, 1296}, {359, 1297},
    {360, 1298}, {361, 1299}, {362, 1290}, {363, 1300}, {364, 1479},
    {365, 1480}, {366, 1481}, {367, 1482}, {368, 1483}, {369, 1484},
    {370, 1485}, {371, 1486}, {372, 1487}, {373, 1488}, {374, 1489},
    {375, 1490}, {376, 1491}, {377, 1492}, {378, 1493}, {379, 1514},
    {380, 1519}, {381, 1520}, {382, 1523}, {383, 1557}, {384, 1595},
    {385, 1697}, {386, 1698}, {387, 1699}, {388, 1723}, {389, 1724},
    {390, 1725}, {391, 1726}, {392, 1727}, {393, 1728}, {394, 1729},
    {395, 1730}, {396, 1731}, {397, 1732}, {398, 1717}, {399, 1718},
    {400, 1719}, {401, 1720}, {402, 1721}, {403, 1722}, {404, 1733},
    {405, 1734}, {406, 1735}, {407, 1736}, {408, 1737}, {409, 1738},
    {410, 1739}, {411, 1742}, {412, 1743}, {413, 1740}, {414, 1741},
    {415, 1751}, {416, 1752}, {417, 1753}, {418, 1754}, {419, 1744},
    {420, 1745}, {421, 1746}, {422, 1747}, {423, 1748}, {424, 1749},
    {425, 1750}, {426, 1755}, {427, 1756}, {428, 1757}, {429, 1758},
    {430, 1759}, {431, 1760}, {432, 1761}, {433, 1762}, {434, 1763},
    {435, 1764}, {436, 1765}, {437, 1766}, {438, 1767}, {439, 1768},
    {440, 1769}, {441, 1770}, {442, 1771}, {443, 1772}, {444, 1773},
    {445, 1774}, {446, 1775}, {447, 1776}, {448, 1777}, {449, 1778},
    {450, 1779}, {451, 1780}, {452, 1781}, {453, 1782}, {454, 1783},
    {455, 1784}, {456, 1785}, {457, 1786}, {458, 1787}, {459, 1788},
    {460, 1793}, {461, 1789}, {462, 1790}, {463, 1791}, {464, 1792},
    {465, 1794}, {466, 1795}, {467, 1796}, {468, 1797}, {469, 1798},
    {470, 1799}, {471, 1827}, {472, 1828}, {473, 1829}, {474, 1830},
    {475, 1831}, {476, 1832}, {477, 1833}, {478, 1834}, {479, 1835},
    {480, 1836}, {481, 1837}, {482, 1838}, {483, 1839}, {484, 1840},
    {485, 1841}, {486, 1842}, {487, 1843}, {488, 1844}, {489, 1845},
    {490, 1846}, {491, 1847}, {492, 1848}, {493, 1849}, {494, 1850},
    {495, 1851}, {496, 1852}, {497, 1853}, {498, 1854}, {499, 1855},
    {500, 1856}, {501, 1972}, {502, 1993}, {503, 1994}, {504, 1996},
    {505, 1997}, {506, 1998}, {507, 1999}, {508, 2000}, {509, 2001},
    {510, 1991}, {511, 1992}, {512, 1995}, {513, 2004}, {514, 2005},
    {515, 2187}, {516, 2188}, {517, 2192}, {518, 2224}, {519, 2223},
    {520, 2383}, {521, 2648}, {522, 2661}, {523, 2677}, {524, 2685},
    {525, 2686}, {526, 2697}, {527, 2699}, {528, 2700}, {529, 2701},
    {530, 2702}, {531, 2703}, {532, 2704}, {533, 2705}, {534, 2687}};
const Str2val kSV_45[] = {
    {120, 0},    {121, 1},    {122, 2},    {123, 3},    {150, 4},
    {151, 5},    {152, 6},    {153, 7},    {154, 8},    {155, 9},
    {156, 10},   {157, 11},   {158, 12},   {159, 13},   {160, 14},
    {161, 15},   {162, 16},   {163, 17},   {164, 18},   {165, 19},
    {166, 20},   {167, 21},   {168, 22},   {169, 23},   {170, 24},
    {171, 25},   {172, 26},   {173, 27},   {174, 28},   {175, 29},
    {176, 30},   {177, 31},   {178, 32},   {179, 33},   {180, 34},
    {181, 35},   {182, 36},   {183, 37},   {184, 38},   {189, 39},
    {197, 40},   {198, 41},   {199, 42},   {201, 43},   {202, 44},
    {203, 45},   {204, 46},   {205, 47},   {206, 48},   {208, 49},
    {209, 50},   {210, 51},   {211, 52},   {212, 53},   {213, 54},
    {214, 55},   {314, 56},   {318, 57},   {321, 58},   {322, 59},
    {323, 60},   {324, 61},   {325, 62},   {326, 63},   {327, 64},
    {328, 65},   {345, 66},   {452, 67},   {453, 76},   {454, 68},
    {455, 69},   {456, 70},   {457, 71},   {458, 72},   {459, 73},
    {460, 74},   {461, 75},   {462, 77},   {463, 78},   {464, 79},
    {466, 80},   {467, 81},   {468, 82},   {469, 83},   {493, 84},
    {640, 85},   {641, 86},   {642, 87},   {643, 88},   {644, 89},
    {645, 90},   {646, 91},   {655, 92},   {671, 93},   {672, 94},
    {673, 95},   {674, 96},   {675, 97},   {682, 98},   {777, 99},
    {778, 100},  {810, 101},  {930, 102},  {931, 103},  {951, 104},
    {952, 105},  {953, 106},  {954, 107},  {955, 108},  {956, 109},
    {957, 110},  {958, 111},  {959, 112},  {960, 113},  {961, 114},
    {962, 205},  {963, 206},  {964, 115},  {965, 116},  {966, 117},
    {967, 118},  {968, 119},  {969, 120},  {970, 121},  {971, 122},
    {972, 123},  {973, 124},  {974, 125},  {975, 126},  {976, 127},
    {977, 128},  {978, 129},  {979, 130},  {980, 131},  {981, 132},
    {982, 133},  {983, 134},  {984, 135},  {985, 136},  {986, 137},
    {987, 138},  {988, 139},  {989, 140},  {990, 141},  {991, 142},
    {992, 143},  {993, 144},  {994, 145},  {995, 146},  {996, 147},
    {997, 148},  {998, 149},  {999, 150},  {1000, 151}, {1001, 152},
    {1002, 153}, {1003, 154}, {1004, 155}, {1005, 156}, {1006, 157},
    {1007, 158}, {1008, 159}, {1009, 160}, {1010, 161}, {1011, 162},
    {1012, 163}, {1013, 164}, {1014, 165}, {1015, 166}, {1016, 167},
    {1017, 168}, {1018, 169}, {1019, 170}, {1020, 171}, {1021, 172},
    {1022, 173}, {1023, 174}, {1024, 175}, {1025, 176}, {1026, 177},
    {1027, 178}, {1028, 179}, {1029, 180}, {1030, 181}, {1031, 182},
    {1032, 183}, {1033, 184}, {1034, 185}, {1035, 186}, {1036, 187},
    {1037, 188}, {1038, 189}, {1039, 190}, {1040, 191}, {1041, 192},
    {1042, 193}, {1043, 194}, {1044, 195}, {1045, 196}, {1046, 197},
    {1047, 198}, {1048, 199}, {1049, 200}, {1050, 201}, {1051, 202},
    {1052, 203}, {1053, 204}, {1054, 207}, {1055, 208}, {1056, 209},
    {1057, 210}, {1058, 231}, {1059, 232}, {1060, 211}, {1061, 212},
    {1062, 213}, {1063, 214}, {1064, 215}, {1065, 216}, {1066, 217},
    {1067, 218}, {1068, 219}, {1069, 220}, {1070, 221}, {1071, 222},
    {1072, 223}, {1073, 224}, {1074, 225}, {1075, 226}, {1076, 227},
    {1077, 228}, {1078, 229}, {1079, 230}, {1080, 233}, {1081, 234},
    {1082, 235}, {1083, 236}, {1084, 237}, {1085, 238}, {1086, 239},
    {1087, 240}, {1088, 241}, {1089, 242}, {1090, 243}, {1091, 244},
    {1092, 254}, {1093, 276}, {1094, 245}, {1095, 246}, {1096, 247},
    {1097, 248}, {1098, 249}, {1099, 250}, {1100, 251}, {1101, 252},
    {1102, 253}, {1103, 255}, {1104, 256}, {1105, 257}, {1106, 258},
    {1107, 259}, {1108, 260}, {1109, 261}, {1110, 262}, {1111, 263},
    {1112, 264}, {1113, 265}, {1114, 266}, {1115, 267}, {1116, 268},
    {1117, 269}, {1118, 270}, {1119, 271}, {1120, 272}, {1121, 273},
    {1122, 274}, {1123, 275}, {1124, 277}, {1125, 289}, {1126, 278},
    {1127, 279}, {1128, 280}, {1129, 281}, {1130, 282}, {1131, 283},
    {1132, 284}, {1133, 285}, {1134, 286}, {1135, 287}, {1136, 288},
    {1137, 290}, {1138, 302}, {1139, 291}, {1140, 292}, {1141, 293},
    {1142, 294}, {1143, 295}, {1144, 296}, {1145, 297}, {1146, 298},
    {1147, 299}, {1148, 300}, {1149, 301}, {1150, 303}, {1151, 304},
    {1152, 305}, {1153, 306}, {1154, 307}, {1155, 308}, {1156, 309},
    {1157, 310}, {1158, 311}, {1159, 312}, {1160, 313}, {1161, 314},
    {1162, 315}, {1253, 316}, {1254, 317}, {1255, 318}, {1256, 319},
    {1257, 320}, {1258, 321}, {1259, 322}, {1260, 323}, {1261, 350},
    {1262, 351}, {1263, 324}, {1264, 325}, {1265, 326}, {1266, 327},
    {1267, 328}, {1268, 329}, {1269, 330}, {1270, 331}, {1271, 332},
    {1272, 333}, {1273, 334}, {1274, 335}, {1275, 336}, {1276, 337},
    {1277, 338}, {1278, 339}, {1279, 340}, {1280, 341}, {1281, 342},
    {1282, 343}, {1283, 344}, {1284, 345}, {1285, 346}, {1286, 347},
    {1287, 348}, {1288, 349}, {1289, 352}, {1290, 362}, {1291, 353},
    {1292, 354}, {1293, 355}, {1294, 356}, {1295, 357}, {1296, 358},
    {1297, 359}, {1298, 360}, {1299, 361}, {1300, 363}, {1479, 364},
    {1480, 365}, {1481, 366}, {1482, 367}, {1483, 368}, {1484, 369},
    {1485, 370}, {1486, 371}, {1487, 372}, {1488, 373}, {1489, 374},
    {1490, 375}, {1491, 376}, {1492, 377}, {1493, 378}, {1514, 379},
    {1519, 380}, {1520, 381}, {1523, 382}, {1557, 383}, {1595, 384},
    {1697, 385}, {1698, 386}, {1699, 387}, {1717, 398}, {1718, 399},
    {1719, 400}, {1720, 401}, {1721, 402}, {1722, 403}, {1723, 388},
    {1724, 389}, {1725, 390}, {1726, 391}, {1727, 392}, {1728, 393},
    {1729, 394}, {1730, 395}, {1731, 396}, {1732, 397}, {1733, 404},
    {1734, 405}, {1735, 406}, {1736, 407}, {1737, 408}, {1738, 409},
    {1739, 410}, {1740, 413}, {1741, 414}, {1742, 411}, {1743, 412},
    {1744, 419}, {1745, 420}, {1746, 421}, {1747, 422}, {1748, 423},
    {1749, 424}, {1750, 425}, {1751, 415}, {1752, 416}, {1753, 417},
    {1754, 418}, {1755, 426}, {1756, 427}, {1757, 428}, {1758, 429},
    {1759, 430}, {1760, 431}, {1761, 432}, {1762, 433}, {1763, 434},
    {1764, 435}, {1765, 436}, {1766, 437}, {1767, 438}, {1768, 439},
    {1769, 440}, {1770, 441}, {1771, 442}, {1772, 443}, {1773, 444},
    {1774, 445}, {1775, 446}, {1776, 447}, {1777, 448}, {1778, 449},
    {1779, 450}, {1780, 451}, {1781, 452}, {1782, 453}, {1783, 454},
    {1784, 455}, {1785, 456}, {1786, 457}, {1787, 458}, {1788, 459},
    {1789, 461}, {1790, 462}, {1791, 463}, {1792, 464}, {1793, 460},
    {1794, 465}, {1795, 466}, {1796, 467}, {1797, 468}, {1798, 469},
    {1799, 470}, {1827, 471}, {1828, 472}, {1829, 473}, {1830, 474},
    {1831, 475}, {1832, 476}, {1833, 477}, {1834, 478}, {1835, 479},
    {1836, 480}, {1837, 481}, {1838, 482}, {1839, 483}, {1840, 484},
    {1841, 485}, {1842, 486}, {1843, 487}, {1844, 488}, {1845, 489},
    {1846, 490}, {1847, 491}, {1848, 492}, {1849, 493}, {1850, 494},
    {1851, 495}, {1852, 496}, {1853, 497}, {1854, 498}, {1855, 499},
    {1856, 500}, {1972, 501}, {1991, 510}, {1992, 511}, {1993, 502},
    {1994, 503}, {1995, 512}, {1996, 504}, {1997, 505}, {1998, 506},
    {1999, 507}, {2000, 508}, {2001, 509}, {2004, 513}, {2005, 514},
    {2187, 515}, {2188, 516}, {2192, 517}, {2223, 519}, {2224, 518},
    {2383, 520}, {2648, 521}, {2661, 522}, {2677, 523}, {2685, 524},
    {2686, 525}, {2687, 534}, {2697, 526}, {2699, 527}, {2700, 528},
    {2701, 529}, {2702, 530}, {2703, 531}, {2704, 532}, {2705, 533}};
const Val2str kVS_46[] = {{0, 799},  {1, 812},  {2, 1619},
                          {3, 1807}, {4, 2240}, {5, 2263}};
const Str2val kSV_46[] = {{799, 0},  {812, 1},  {1619, 2},
                          {1807, 3}, {2240, 4}, {2263, 5}};
const Val2str kVS_47[] = {{0, 2822}, {1, 2846}};
const Str2val kSV_47[] = {{2822, 0}, {2846, 1}};
const Val2str kVS_48[] = {{0, 318},   {1, 324},   {2, 655},   {3, 1514},
                          {4, 1557},  {5, 1595},  {6, 1697},  {7, 2383},
                          {8, 2677},  {9, 2686},  {10, 2697}, {11, 2699},
                          {12, 2700}, {13, 2701}, {14, 2702}, {15, 2703},
                          {16, 2704}, {17, 2705}, {18, 2687}};
const Str2val kSV_48[] = {{318, 0},   {324, 1},   {655, 2},   {1514, 3},
                          {1557, 4},  {1595, 5},  {1697, 6},  {2383, 7},
                          {2677, 8},  {2686, 9},  {2687, 18}, {2697, 10},
                          {2699, 11}, {2700, 12}, {2701, 13}, {2702, 14},
                          {2703, 15}, {2704, 16}, {2705, 17}};
const Val2str kVS_49[] = {};
const Str2val kSV_49[] = {};
const Val2str kVS_50[] = {{0, 312}, {1, 1522}, {2, 2004}};
const Str2val kSV_50[] = {{312, 0}, {1522, 1}, {2004, 2}};
const Val2str kVS_51[] = {
    {0, 120},    {1, 121},    {2, 122},    {3, 123},    {4, 150},
    {5, 151},    {6, 152},    {7, 153},    {8, 154},    {9, 155},
    {10, 156},   {11, 157},   {12, 158},   {13, 159},   {14, 160},
    {15, 161},   {16, 162},   {17, 163},   {18, 164},   {19, 165},
    {20, 166},   {21, 167},   {22, 168},   {23, 169},   {24, 170},
    {25, 171},   {26, 172},   {27, 173},   {28, 174},   {29, 175},
    {30, 176},   {31, 177},   {32, 178},   {33, 179},   {34, 180},
    {35, 181},   {36, 182},   {37, 183},   {38, 184},   {39, 189},
    {40, 197},   {41, 198},   {42, 199},   {43, 201},   {44, 202},
    {45, 203},   {46, 204},   {47, 205},   {48, 206},   {49, 208},
    {50, 209},   {51, 210},   {52, 211},   {53, 212},   {54, 213},
    {55, 214},   {56, 321},   {57, 322},   {58, 323},   {59, 325},
    {60, 326},   {61, 327},   {62, 328},   {63, 345},   {64, 452},
    {65, 454},   {66, 455},   {67, 456},   {68, 457},   {69, 458},
    {70, 459},   {71, 460},   {72, 461},   {73, 453},   {74, 462},
    {75, 463},   {76, 464},   {77, 466},   {78, 467},   {79, 468},
    {80, 469},   {81, 493},   {82, 640},   {83, 641},   {84, 642},
    {85, 643},   {86, 644},   {87, 645},   {88, 646},   {89, 671},
    {90, 672},   {91, 673},   {92, 674},   {93, 675},   {94, 682},
    {95, 777},   {96, 778},   {97, 930},   {98, 931},   {99, 951},
    {100, 952},  {101, 953},  {102, 954},  {103, 955},  {104, 956},
    {105, 957},  {106, 958},  {107, 959},  {108, 960},  {109, 961},
    {110, 964},  {111, 965},  {112, 966},  {113, 967},  {114, 968},
    {115, 969},  {116, 970},  {117, 971},  {118, 972},  {119, 973},
    {120, 974},  {121, 975},  {122, 976},  {123, 977},  {124, 978},
    {125, 979},  {126, 980},  {127, 981},  {128, 982},  {129, 983},
    {130, 984},  {131, 985},  {132, 986},  {133, 987},  {134, 988},
    {135, 989},  {136, 990},  {137, 991},  {138, 992},  {139, 993},
    {140, 994},  {141, 995},  {142, 996},  {143, 997},  {144, 998},
    {145, 999},  {146, 1000}, {147, 1001}, {148, 1002}, {149, 1003},
    {150, 1004}, {151, 1005}, {152, 1006}, {153, 1007}, {154, 1008},
    {155, 1009}, {156, 1010}, {157, 1011}, {158, 1012}, {159, 1013},
    {160, 1014}, {161, 1015}, {162, 1016}, {163, 1017}, {164, 1018},
    {165, 1019}, {166, 1020}, {167, 1021}, {168, 1022}, {169, 1023},
    {170, 1024}, {171, 1025}, {172, 1026}, {173, 1027}, {174, 1028},
    {175, 1029}, {176, 1030}, {177, 1031}, {178, 1032}, {179, 1033},
    {180, 1034}, {181, 1035}, {182, 1036}, {183, 1037}, {184, 1038},
    {185, 1039}, {186, 1040}, {187, 1041}, {188, 1042}, {189, 1043},
    {190, 1044}, {191, 1045}, {192, 1046}, {193, 1047}, {194, 1048},
    {195, 1049}, {196, 1050}, {197, 1051}, {198, 1052}, {199, 1053},
    {200, 962},  {201, 963},  {202, 1054}, {203, 1055}, {204, 1056},
    {205, 1057}, {206, 1060}, {207, 1061}, {208, 1062}, {209, 1063},
    {210, 1064}, {211, 1065}, {212, 1067}, {213, 1068}, {214, 1069},
    {215, 1071}, {216, 1072}, {217, 1073}, {218, 1074}, {219, 1075},
    {220, 1076}, {221, 1077}, {222, 1078}, {223, 1079}, {224, 1058},
    {225, 1059}, {226, 1080}, {227, 1082}, {228, 1084}, {229, 1086},
    {230, 1088}, {231, 1090}, {232, 1091}, {233, 1094}, {234, 1095},
    {235, 1096}, {236, 1097}, {237, 1098}, {238, 1099}, {239, 1100},
    {240, 1101}, {241, 1102}, {242, 1092}, {243, 1103}, {244, 1104},
    {245, 1105}, {246, 1106}, {247, 1107}, {248, 1108}, {249, 1109},
    {250, 1110}, {251, 1111}, {252, 1112}, {253, 1113}, {254, 1114},
    {255, 1115}, {256, 1116}, {257, 1117}, {258, 1118}, {259, 1119},
    {260, 1120}, {261, 1121}, {262, 1122}, {263, 1123}, {264, 1093},
    {265, 1124}, {266, 1126}, {267, 1127}, {268, 1128}, {269, 1129},
    {270, 1130}, {271, 1131}, {272, 1132}, {273, 1133}, {274, 1134},
    {275, 1135}, {276, 1136}, {277, 1125}, {278, 1137}, {279, 1139},
    {280, 1140}, {281, 1141}, {282, 1142}, {283, 1143}, {284, 1144},
    {285, 1145}, {286, 1146}, {287, 1147}, {288, 1148}, {289, 1149},
    {290, 1138}, {291, 1150}, {292, 1151}, {293, 1152}, {294, 1153},
    {295, 1154}, {296, 1155}, {297, 1156}, {298, 1157}, {299, 1158},
    {300, 1159}, {301, 1160}, {302, 1161}, {303, 1162}, {304, 1253},
    {305, 1254}, {306, 1255}, {307, 1256}, {308, 1257}, {309, 1258},
    {310, 1259}, {311, 1260}, {312, 1263}, {313, 1264}, {314, 1265},
    {315, 1266}, {316, 1267}, {317, 1268}, {318, 1269}, {319, 1270},
    {320, 1271}, {321, 1272}, {322, 1273}, {323, 1274}, {324, 1275},
    {325, 1276}, {326, 1277}, {327, 1278}, {328, 1279}, {329, 1280},
    {330, 1281}, {331, 1282}, {332, 1283}, {333, 1284}, {334, 1285},
    {335, 1286}, {336, 1287}, {337, 1288}, {338, 1261}, {339, 1262},
    {340, 1289}, {341, 1291}, {342, 1292}, {343, 1293}, {344, 1294},
    {345, 1295}, {346, 1296}, {347, 1297}, {348, 1298}, {349, 1299},
    {350, 1290}, {351, 1300}, {352, 1479}, {353, 1480}, {354, 1481},
    {355, 1482}, {356, 1483}, {357, 1484}, {358, 1485}, {359, 1486},
    {360, 1487}, {361, 1488}, {362, 1489}, {363, 1490}, {364, 1491},
    {365, 1492}, {366, 1519}, {367, 1520}, {368, 1698}, {369, 1723},
    {370, 1724}, {371, 1726}, {372, 1728}, {373, 1729}, {374, 1731},
    {375, 1717}, {376, 1719}, {377, 1721}, {378, 1733}, {379, 1734},
    {380, 1735}, {381, 1736}, {382, 1737}, {383, 1738}, {384, 1739},
    {385, 1742}, {386, 1740}, {387, 1751}, {388, 1752}, {389, 1753},
    {390, 1754}, {391, 1744}, {392, 1745}, {393, 1746}, {394, 1747},
    {395, 1748}, {396, 1749}, {397, 1750}, {398, 1755}, {399, 1756},
    {400, 1757}, {401, 1758}, {402, 1759}, {403, 1760}, {404, 1761},
    {405, 1762}, {406, 1763}, {407, 1764}, {408, 1765}, {409, 1766},
    {410, 1767}, {411, 1768}, {412, 1769}, {413, 1770}, {414, 1771},
    {415, 1772}, {416, 1773}, {417, 1774}, {418, 1775}, {419, 1776},
    {420, 1777}, {421, 1778}, {422, 1779}, {423, 1780}, {424, 1781},
    {425, 1782}, {426, 1783}, {427, 1784}, {428, 1785}, {429, 1786},
    {430, 1787}, {431, 1788}, {432, 1793}, {433, 1789}, {434, 1790},
    {435, 1791}, {436, 1792}, {437, 1794}, {438, 1795}, {439, 1796},
    {440, 1797}, {441, 1798}, {442, 1799}, {443, 1827}, {444, 1828},
    {445, 1829}, {446, 1830}, {447, 1831}, {448, 1832}, {449, 1833},
    {450, 1834}, {451, 1835}, {452, 1836}, {453, 1837}, {454, 1838},
    {455, 1839}, {456, 1840}, {457, 1841}, {458, 1842}, {459, 1843},
    {460, 1844}, {461, 1845}, {462, 1846}, {463, 1847}, {464, 1848},
    {465, 1849}, {466, 1850}, {467, 1851}, {468, 1852}, {469, 1853},
    {470, 1854}, {471, 1855}, {472, 1856}, {473, 1993}, {474, 1994},
    {475, 1996}, {476, 1997}, {477, 1998}, {478, 1999}, {479, 2000},
    {480, 2001}, {481, 1991}, {482, 1992}, {483, 1995}, {484, 2187},
    {485, 2188}, {486, 2224}, {487, 2223}, {488, 2648}, {489, 2661}};
const Str2val kSV_51[] = {
    {120, 0},    {121, 1},    {122, 2},    {123, 3},    {150, 4},
    {151, 5},    {152, 6},    {153, 7},    {154, 8},    {155, 9},
    {156, 10},   {157, 11},   {158, 12},   {159, 13},   {160, 14},
    {161, 15},   {162, 16},   {163, 17},   {164, 18},   {165, 19},
    {166, 20},   {167, 21},   {168, 22},   {169, 23},   {170, 24},
    {171, 25},   {172, 26},   {173, 27},   {174, 28},   {175, 29},
    {176, 30},   {177, 31},   {178, 32},   {179, 33},   {180, 34},
    {181, 35},   {182, 36},   {183, 37},   {184, 38},   {189, 39},
    {197, 40},   {198, 41},   {199, 42},   {201, 43},   {202, 44},
    {203, 45},   {204, 46},   {205, 47},   {206, 48},   {208, 49},
    {209, 50},   {210, 51},   {211, 52},   {212, 53},   {213, 54},
    {214, 55},   {321, 56},   {322, 57},   {323, 58},   {325, 59},
    {326, 60},   {327, 61},   {328, 62},   {345, 63},   {452, 64},
    {453, 73},   {454, 65},   {455, 66},   {456, 67},   {457, 68},
    {458, 69},   {459, 70},   {460, 71},   {461, 72},   {462, 74},
    {463, 75},   {464, 76},   {466, 77},   {467, 78},   {468, 79},
    {469, 80},   {493, 81},   {640, 82},   {641, 83},   {642, 84},
    {643, 85},   {644, 86},   {645, 87},   {646, 88},   {671, 89},
    {672, 90},   {673, 91},   {674, 92},   {675, 93},   {682, 94},
    {777, 95},   {778, 96},   {930, 97},   {931, 98},   {951, 99},
    {952, 100},  {953, 101},  {954, 102},  {955, 103},  {956, 104},
    {957, 105},  {958, 106},  {959, 107},  {960, 108},  {961, 109},
    {962, 200},  {963, 201},  {964, 110},  {965, 111},  {966, 112},
    {967, 113},  {968, 114},  {969, 115},  {970, 116},  {971, 117},
    {972, 118},  {973, 119},  {974, 120},  {975, 121},  {976, 122},
    {977, 123},  {978, 124},  {979, 125},  {980, 126},  {981, 127},
    {982, 128},  {983, 129},  {984, 130},  {985, 131},  {986, 132},
    {987, 133},  {988, 134},  {989, 135},  {990, 136},  {991, 137},
    {992, 138},  {993, 139},  {994, 140},  {995, 141},  {996, 142},
    {997, 143},  {998, 144},  {999, 145},  {1000, 146}, {1001, 147},
    {1002, 148}, {1003, 149}, {1004, 150}, {1005, 151}, {1006, 152},
    {1007, 153}, {1008, 154}, {1009, 155}, {1010, 156}, {1011, 157},
    {1012, 158}, {1013, 159}, {1014, 160}, {1015, 161}, {1016, 162},
    {1017, 163}, {1018, 164}, {1019, 165}, {1020, 166}, {1021, 167},
    {1022, 168}, {1023, 169}, {1024, 170}, {1025, 171}, {1026, 172},
    {1027, 173}, {1028, 174}, {1029, 175}, {1030, 176}, {1031, 177},
    {1032, 178}, {1033, 179}, {1034, 180}, {1035, 181}, {1036, 182},
    {1037, 183}, {1038, 184}, {1039, 185}, {1040, 186}, {1041, 187},
    {1042, 188}, {1043, 189}, {1044, 190}, {1045, 191}, {1046, 192},
    {1047, 193}, {1048, 194}, {1049, 195}, {1050, 196}, {1051, 197},
    {1052, 198}, {1053, 199}, {1054, 202}, {1055, 203}, {1056, 204},
    {1057, 205}, {1058, 224}, {1059, 225}, {1060, 206}, {1061, 207},
    {1062, 208}, {1063, 209}, {1064, 210}, {1065, 211}, {1067, 212},
    {1068, 213}, {1069, 214}, {1071, 215}, {1072, 216}, {1073, 217},
    {1074, 218}, {1075, 219}, {1076, 220}, {1077, 221}, {1078, 222},
    {1079, 223}, {1080, 226}, {1082, 227}, {1084, 228}, {1086, 229},
    {1088, 230}, {1090, 231}, {1091, 232}, {1092, 242}, {1093, 264},
    {1094, 233}, {1095, 234}, {1096, 235}, {1097, 236}, {1098, 237},
    {1099, 238}, {1100, 239}, {1101, 240}, {1102, 241}, {1103, 243},
    {1104, 244}, {1105, 245}, {1106, 246}, {1107, 247}, {1108, 248},
    {1109, 249}, {1110, 250}, {1111, 251}, {1112, 252}, {1113, 253},
    {1114, 254}, {1115, 255}, {1116, 256}, {1117, 257}, {1118, 258},
    {1119, 259}, {1120, 260}, {1121, 261}, {1122, 262}, {1123, 263},
    {1124, 265}, {1125, 277}, {1126, 266}, {1127, 267}, {1128, 268},
    {1129, 269}, {1130, 270}, {1131, 271}, {1132, 272}, {1133, 273},
    {1134, 274}, {1135, 275}, {1136, 276}, {1137, 278}, {1138, 290},
    {1139, 279}, {1140, 280}, {1141, 281}, {1142, 282}, {1143, 283},
    {1144, 284}, {1145, 285}, {1146, 286}, {1147, 287}, {1148, 288},
    {1149, 289}, {1150, 291}, {1151, 292}, {1152, 293}, {1153, 294},
    {1154, 295}, {1155, 296}, {1156, 297}, {1157, 298}, {1158, 299},
    {1159, 300}, {1160, 301}, {1161, 302}, {1162, 303}, {1253, 304},
    {1254, 305}, {1255, 306}, {1256, 307}, {1257, 308}, {1258, 309},
    {1259, 310}, {1260, 311}, {1261, 338}, {1262, 339}, {1263, 312},
    {1264, 313}, {1265, 314}, {1266, 315}, {1267, 316}, {1268, 317},
    {1269, 318}, {1270, 319}, {1271, 320}, {1272, 321}, {1273, 322},
    {1274, 323}, {1275, 324}, {1276, 325}, {1277, 326}, {1278, 327},
    {1279, 328}, {1280, 329}, {1281, 330}, {1282, 331}, {1283, 332},
    {1284, 333}, {1285, 334}, {1286, 335}, {1287, 336}, {1288, 337},
    {1289, 340}, {1290, 350}, {1291, 341}, {1292, 342}, {1293, 343},
    {1294, 344}, {1295, 345}, {1296, 346}, {1297, 347}, {1298, 348},
    {1299, 349}, {1300, 351}, {1479, 352}, {1480, 353}, {1481, 354},
    {1482, 355}, {1483, 356}, {1484, 357}, {1485, 358}, {1486, 359},
    {1487, 360}, {1488, 361}, {1489, 362}, {1490, 363}, {1491, 364},
    {1492, 365}, {1519, 366}, {1520, 367}, {1698, 368}, {1717, 375},
    {1719, 376}, {1721, 377}, {1723, 369}, {1724, 370}, {1726, 371},
    {1728, 372}, {1729, 373}, {1731, 374}, {1733, 378}, {1734, 379},
    {1735, 380}, {1736, 381}, {1737, 382}, {1738, 383}, {1739, 384},
    {1740, 386}, {1742, 385}, {1744, 391}, {1745, 392}, {1746, 393},
    {1747, 394}, {1748, 395}, {1749, 396}, {1750, 397}, {1751, 387},
    {1752, 388}, {1753, 389}, {1754, 390}, {1755, 398}, {1756, 399},
    {1757, 400}, {1758, 401}, {1759, 402}, {1760, 403}, {1761, 404},
    {1762, 405}, {1763, 406}, {1764, 407}, {1765, 408}, {1766, 409},
    {1767, 410}, {1768, 411}, {1769, 412}, {1770, 413}, {1771, 414},
    {1772, 415}, {1773, 416}, {1774, 417}, {1775, 418}, {1776, 419},
    {1777, 420}, {1778, 421}, {1779, 422}, {1780, 423}, {1781, 424},
    {1782, 425}, {1783, 426}, {1784, 427}, {1785, 428}, {1786, 429},
    {1787, 430}, {1788, 431}, {1789, 433}, {1790, 434}, {1791, 435},
    {1792, 436}, {1793, 432}, {1794, 437}, {1795, 438}, {1796, 439},
    {1797, 440}, {1798, 441}, {1799, 442}, {1827, 443}, {1828, 444},
    {1829, 445}, {1830, 446}, {1831, 447}, {1832, 448}, {1833, 449},
    {1834, 450}, {1835, 451}, {1836, 452}, {1837, 453}, {1838, 454},
    {1839, 455}, {1840, 456}, {1841, 457}, {1842, 458}, {1843, 459},
    {1844, 460}, {1845, 461}, {1846, 462}, {1847, 463}, {1848, 464},
    {1849, 465}, {1850, 466}, {1851, 467}, {1852, 468}, {1853, 469},
    {1854, 470}, {1855, 471}, {1856, 472}, {1991, 481}, {1992, 482},
    {1993, 473}, {1994, 474}, {1995, 483}, {1996, 475}, {1997, 476},
    {1998, 477}, {1999, 478}, {2000, 479}, {2001, 480}, {2187, 484},
    {2188, 485}, {2223, 487}, {2224, 486}, {2648, 488}, {2661, 489}};
const Val2str kVS_52[] = {
    {0, 145},   {1, 146},   {2, 195},   {3, 318},   {4, 324},   {5, 336},
    {6, 572},   {7, 655},   {8, 809},   {9, 1514},  {10, 1521}, {11, 1557},
    {12, 1558}, {13, 1595}, {14, 1697}, {15, 1962}, {16, 2191}, {17, 2222},
    {18, 2226}, {19, 2228}, {20, 2229}, {21, 2230}, {22, 2231}, {23, 2232},
    {24, 2233}, {25, 2234}, {26, 2235}, {27, 2227}, {28, 2383}, {29, 2677},
    {30, 2686}, {31, 2697}, {32, 2699}, {33, 2700}, {34, 2701}, {35, 2702},
    {36, 2703}, {37, 2704}, {38, 2705}, {39, 2687}, {40, 2688}, {41, 2689},
    {42, 2690}, {43, 2691}, {44, 2692}, {45, 2693}, {46, 2694}, {47, 2695},
    {48, 2696}, {49, 2698}};
const Str2val kSV_52[] = {
    {145, 0},   {146, 1},   {195, 2},   {318, 3},   {324, 4},   {336, 5},
    {572, 6},   {655, 7},   {809, 8},   {1514, 9},  {1521, 10}, {1557, 11},
    {1558, 12}, {1595, 13}, {1697, 14}, {1962, 15}, {2191, 16}, {2222, 17},
    {2226, 18}, {2227, 27}, {2228, 19}, {2229, 20}, {2230, 21}, {2231, 22},
    {2232, 23}, {2233, 24}, {2234, 25}, {2235, 26}, {2383, 28}, {2677, 29},
    {2686, 30}, {2687, 39}, {2688, 40}, {2689, 41}, {2690, 42}, {2691, 43},
    {2692, 44}, {2693, 45}, {2694, 46}, {2695, 47}, {2696, 48}, {2697, 31},
    {2698, 49}, {2699, 32}, {2700, 33}, {2701, 34}, {2702, 35}, {2703, 36},
    {2704, 37}, {2705, 38}};
const Val2str kVS_53[] = {{0, 149},  {1, 329},  {2, 402},  {3, 699},
                          {4, 1544}, {5, 1687}, {6, 2433}, {7, 2553},
                          {8, 2760}, {9, 2772}};
const Str2val kSV_53[] = {{149, 0},  {329, 1},  {402, 2},  {699, 3},
                          {1544, 4}, {1687, 5}, {2433, 6}, {2553, 7},
                          {2760, 8}, {2772, 9}};
const Val2str kVS_54[] = {
    {0, 147},    {1, 195},    {2, 216},    {3, 333},    {4, 334},
    {5, 335},    {6, 428},    {7, 429},    {8, 430},    {9, 435},
    {10, 572},   {11, 573},   {12, 574},   {13, 575},   {14, 576},
    {15, 577},   {16, 634},   {17, 637},   {18, 639},   {19, 652},
    {20, 653},   {21, 655},   {22, 656},   {23, 657},   {24, 658},
    {25, 659},   {26, 660},   {27, 661},   {28, 662},   {29, 663},
    {30, 664},   {31, 665},   {32, 666},   {33, 683},   {34, 684},
    {35, 685},   {36, 686},   {37, 687},   {38, 688},   {39, 689},
    {40, 698},   {41, 716},   {42, 717},   {43, 719},   {44, 720},
    {45, 789},   {46, 794},   {47, 795},   {48, 796},   {49, 797},
    {50, 798},   {51, 803},   {52, 822},   {53, 823},   {54, 1493},
    {55, 1494},  {56, 1495},  {57, 1496},  {58, 1497},  {59, 1498},
    {60, 1499},  {61, 1500},  {62, 1501},  {63, 1502},  {64, 1505},
    {65, 1523},  {66, 1691},  {67, 1692},  {68, 1693},  {69, 1694},
    {70, 1695},  {71, 1696},  {72, 1702},  {73, 1705},  {74, 1706},
    {75, 1868},  {76, 1902},  {77, 1963},  {78, 1964},  {79, 1965},
    {80, 1966},  {81, 1967},  {82, 1968},  {83, 1969},  {84, 1970},
    {85, 1973},  {86, 1974},  {87, 1975},  {88, 1976},  {89, 1977},
    {90, 1978},  {91, 1979},  {92, 1980},  {93, 1981},  {94, 1987},
    {95, 2003},  {96, 2225},  {97, 2258},  {98, 2259},  {99, 2261},
    {100, 2262}, {101, 2380}, {102, 2392}, {103, 2393}, {104, 2394},
    {105, 2539}, {106, 2540}, {107, 2541}, {108, 2542}, {109, 2543},
    {110, 2544}, {111, 2545}, {112, 2546}, {113, 2547}, {114, 2548},
    {115, 2549}, {116, 2550}, {117, 2660}, {118, 2682}, {119, 2683},
    {120, 2685}, {121, 2756}, {122, 2779}};
const Str2val kSV_54[] = {
    {147, 0},    {195, 1},    {216, 2},    {333, 3},    {334, 4},
    {335, 5},    {428, 6},    {429, 7},    {430, 8},    {435, 9},
    {572, 10},   {573, 11},   {574, 12},   {575, 13},   {576, 14},
    {577, 15},   {634, 16},   {637, 17},   {639, 18},   {652, 19},
    {653, 20},   {655, 21},   {656, 22},   {657, 23},   {658, 24},
    {659, 25},   {660, 26},   {661, 27},   {662, 28},   {663, 29},
    {664, 30},   {665, 31},   {666, 32},   {683, 33},   {684, 34},
    {685, 35},   {686, 36},   {687, 37},   {688, 38},   {689, 39},
    {698, 40},   {716, 41},   {717, 42},   {719, 43},   {720, 44},
    {789, 45},   {794, 46},   {795, 47},   {796, 48},   {797, 49},
    {798, 50},   {803, 51},   {822, 52},   {823, 53},   {1493, 54},
    {1494, 55},  {1495, 56},  {1496, 57},  {1497, 58},  {1498, 59},
    {1499, 60},  {1500, 61},  {1501, 62},  {1502, 63},  {1505, 64},
    {1523, 65},  {1691, 66},  {1692, 67},  {1693, 68},  {1694, 69},
    {1695, 70},  {1696, 71},  {1702, 72},  {1705, 73},  {1706, 74},
    {1868, 75},  {1902, 76},  {1963, 77},  {1964, 78},  {1965, 79},
    {1966, 80},  {1967, 81},  {1968, 82},  {1969, 83},  {1970, 84},
    {1973, 85},  {1974, 86},  {1975, 87},  {1976, 88},  {1977, 89},
    {1978, 90},  {1979, 91},  {1980, 92},  {1981, 93},  {1987, 94},
    {2003, 95},  {2225, 96},  {2258, 97},  {2259, 98},  {2261, 99},
    {2262, 100}, {2380, 101}, {2392, 102}, {2393, 103}, {2394, 104},
    {2539, 105}, {2540, 106}, {2541, 107}, {2542, 108}, {2543, 109},
    {2544, 110}, {2545, 111}, {2546, 112}, {2547, 113}, {2548, 114},
    {2549, 115}, {2550, 116}, {2660, 117}, {2682, 118}, {2683, 119},
    {2685, 120}, {2756, 121}, {2779, 122}};
const Val2str kVS_55[] = {{0, 2264}, {1, 2265}, {2, 2390}, {3, 2391}};
const Str2val kSV_55[] = {{2264, 0}, {2265, 1}, {2390, 2}, {2391, 3}};
const Val2str kVS_56[] = {{0, 124}, {1, 814}, {2, 2107}};
const Str2val kSV_56[] = {{124, 0}, {814, 1}, {2107, 2}};
const Val2str kVS_57[] = {
    {0, 584},   {1, 585},   {2, 587},   {3, 592},   {4, 627},   {5, 628},
    {6, 1327},  {7, 1331},  {8, 1345},  {9, 1366},  {10, 1428}, {11, 1460},
    {12, 1463}, {13, 1807}, {14, 2035}, {15, 2038}, {16, 2040}, {17, 2050},
    {18, 2062}, {19, 2075}, {20, 2081}, {21, 2082}, {22, 2086}, {23, 2092},
    {24, 2205}, {25, 2206}, {26, 2207}, {27, 2209}, {28, 2210}, {29, 2651},
    {30, 2653}, {31, 2654}, {32, 2656}, {33, 2658}};
const Str2val kSV_57[] = {
    {584, 0},   {585, 1},   {587, 2},   {592, 3},   {627, 4},   {628, 5},
    {1327, 6},  {1331, 7},  {1345, 8},  {1366, 9},  {1428, 10}, {1460, 11},
    {1463, 12}, {1807, 13}, {2035, 14}, {2038, 15}, {2040, 16}, {2050, 17},
    {2062, 18}, {2075, 19}, {2081, 20}, {2082, 21}, {2086, 22}, {2092, 23},
    {2205, 24}, {2206, 25}, {2207, 26}, {2209, 27}, {2210, 28}, {2651, 29},
    {2653, 30}, {2654, 31}, {2656, 32}, {2658, 33}};
const Val2str kVS_58[] = {{0, 938}};
const Str2val kSV_58[] = {{938, 0}};
const Val2str kVS_59[] = {
    {2, 81},     {3, 82},     {4, 119},    {5, 37},     {6, 99},
    {7, 101},    {8, 31},     {9, 57},     {10, 58},    {11, 62},
    {12, 73},    {13, 87},    {14, 90},    {16, 79},    {17, 97},
    {18, 84},    {19, 104},   {20, 103},   {21, 64},    {22, 40},
    {23, 38},    {24, 68},    {25, 69},    {26, 88},    {27, 35},
    {28, 60},    {30, 66},    {32, 67},    {34, 52},    {35, 50},
    {36, 80},    {37, 74},    {38, 86},    {39, 44},    {40, 9},
    {41, 92},    {42, 109},   {43, 112},   {44, 89},    {45, 29},
    {46, 113},   {47, 96},    {48, 83},    {49, 98},    {51, 30},
    {52, 55},    {53, 56},    {54, 46},    {55, 102},   {56, 32},
    {57, 33},    {58, 94},    {59, 36},    {60, 75},    {61, 118},
    {62, 10},    {63, 6},     {64, 7},     {65, 8},     {66, 53},
    {67, 54},    {68, 61},    {69, 114},   {70, 48},    {71, 115},
    {72, 116},   {73, 117},   {74, 59},    {75, 11},    {76, 39},
    {77, 45},    {78, 47},    {79, 65},    {80, 108},   {81, 111},
    {82, 34},    {83, 41},    {84, 76},    {85, 100},   {86, 105},
    {87, 42},    {88, 43},    {89, 49},    {90, 51},    {91, 70},
    {92, 71},    {93, 77},    {94, 78},    {95, 85},    {96, 93},
    {97, 95},    {98, 106},   {99, 107},   {100, 110},  {101, 63},
    {102, 72},   {103, 91},   {16385, 20}, {16386, 25}, {16387, 14},
    {16388, 18}, {16389, 19}, {16390, 13}, {16391, 17}, {16392, 12},
    {16393, 27}, {16394, 28}, {16395, 21}, {16396, 24}, {16397, 26},
    {16398, 15}, {16399, 23}, {16423, 22}, {16424, 16}};
const Str2val kSV_59[] = {
    {6, 63},     {7, 64},     {8, 65},     {9, 40},     {10, 62},
    {11, 75},    {12, 16392}, {13, 16390}, {14, 16387}, {15, 16398},
    {16, 16424}, {17, 16391}, {18, 16388}, {19, 16389}, {20, 16385},
    {21, 16395}, {22, 16423}, {23, 16399}, {24, 16396}, {25, 16386},
    {26, 16397}, {27, 16393}, {28, 16394}, {29, 45},    {30, 51},
    {31, 8},     {32, 56},    {33, 57},    {34, 82},    {35, 27},
    {36, 59},    {37, 5},     {38, 23},    {39, 76},    {40, 22},
    {41, 83},    {42, 87},    {43, 88},    {44, 39},    {45, 77},
    {46, 54},    {47, 78},    {48, 70},    {49, 89},    {50, 35},
    {51, 90},    {52, 34},    {53, 66},    {54, 67},    {55, 52},
    {56, 53},    {57, 9},     {58, 10},    {59, 74},    {60, 28},
    {61, 68},    {62, 11},    {63, 101},   {64, 21},    {65, 79},
    {66, 30},    {67, 32},    {68, 24},    {69, 25},    {70, 91},
    {71, 92},    {72, 102},   {73, 12},    {74, 37},    {75, 60},
    {76, 84},    {77, 93},    {78, 94},    {79, 16},    {80, 36},
    {81, 2},     {82, 3},     {83, 48},    {84, 18},    {85, 95},
    {86, 38},    {87, 13},    {88, 26},    {89, 44},    {90, 14},
    {91, 103},   {92, 41},    {93, 96},    {94, 58},    {95, 97},
    {96, 47},    {97, 17},    {98, 49},    {99, 6},     {100, 85},
    {101, 7},    {102, 55},   {103, 20},   {104, 19},   {105, 86},
    {106, 98},   {107, 99},   {108, 80},   {109, 42},   {110, 100},
    {111, 81},   {112, 43},   {113, 46},   {114, 69},   {115, 71},
    {116, 72},   {117, 73},   {118, 61},   {119, 4}};
const Val2str kVS_60[] = {
    {0, 2217}, {1, 2218}, {2, 2238}, {3, 2239}, {4, 2655}};
const Str2val kSV_60[] = {
    {2217, 0}, {2218, 1}, {2238, 2}, {2239, 3}, {2655, 4}};
const Val2str kVS_61[] = {
    {0, 133},  {1, 134},  {2, 135},  {3, 136},   {4, 940},  {5, 941},
    {6, 942},  {7, 943},  {8, 944},  {9, 945},   {10, 946}, {11, 947},
    {12, 948}, {13, 949}, {14, 950}, {15, 1807}, {16, 2183}};
const Str2val kSV_61[] = {
    {133, 0},  {134, 1},  {135, 2},  {136, 3},   {940, 4},  {941, 5},
    {942, 6},  {943, 7},  {944, 8},  {945, 9},   {946, 10}, {947, 11},
    {948, 12}, {949, 13}, {950, 14}, {1807, 15}, {2183, 16}};
const Val2str kVS_62[] = {{0, 1911}, {1, 1913}, {2, 1915}};
const Str2val kSV_62[] = {{1911, 0}, {1913, 1}, {1915, 2}};
const Val2str kVS_63[] = {{0, 190}, {1, 807}, {2, 1809}};
const Str2val kSV_63[] = {{190, 0}, {807, 1}, {1809, 2}};
const Val2str kVS_64[] = {{0, 2671}, {1, 2672}, {2, 2673}, {3, 2674},
                          {4, 2678}, {5, 2679}, {6, 2680}, {7, 2681}};
const Str2val kSV_64[] = {{2671, 0}, {2672, 1}, {2673, 2}, {2674, 3},
                          {2678, 4}, {2679, 5}, {2680, 6}, {2681, 7}};
const Val2str kVS_65[] = {{0, 195}, {1, 200},  {2, 261},  {3, 411},
                          {4, 813}, {5, 1700}, {6, 2106}, {7, 2108}};
const Str2val kSV_65[] = {{195, 0}, {200, 1},  {261, 2},  {411, 3},
                          {813, 4}, {1700, 5}, {2106, 6}, {2108, 7}};
const Val2str kVS_66[] = {{0, 195}, {1, 802}, {2, 1962}, {3, 2663}, {4, 2664}};
const Str2val kSV_66[] = {{195, 0}, {802, 1}, {1962, 2}, {2663, 3}, {2664, 4}};
const Val2str kVS_67[] = {{0, 127},  {1, 195},  {2, 1922},
                          {3, 2195}, {4, 2196}, {5, 2241}};
const Str2val kSV_67[] = {{127, 0},  {195, 1},  {1922, 2},
                          {2195, 3}, {2196, 4}, {2241, 5}};
const Val2str kVS_68[] = {{0, 195}, {1, 196}, {2, 697}, {3, 713}, {4, 1807}};
const Str2val kSV_68[] = {{195, 0}, {196, 1}, {697, 2}, {713, 3}, {1807, 4}};
const Val2str kVS_69[] = {{3, 821}, {4, 2109}, {5, 2601}};
const Str2val kSV_69[] = {{821, 3}, {2109, 4}, {2601, 5}};
const Val2str kVS_70[] = {
    {0, 143},    {1, 224},    {2, 225},    {3, 226},    {4, 227},
    {5, 228},    {6, 229},    {7, 230},    {8, 231},    {9, 232},
    {10, 233},   {11, 234},   {12, 235},   {13, 236},   {14, 237},
    {15, 238},   {16, 239},   {17, 240},   {18, 241},   {19, 242},
    {20, 243},   {21, 244},   {22, 245},   {23, 246},   {24, 247},
    {25, 248},   {26, 249},   {27, 250},   {28, 251},   {29, 252},
    {30, 253},   {31, 254},   {32, 255},   {33, 256},   {34, 257},
    {35, 258},   {36, 259},   {37, 267},   {38, 268},   {39, 269},
    {40, 270},   {41, 271},   {42, 272},   {43, 273},   {44, 274},
    {45, 275},   {46, 276},   {47, 277},   {48, 278},   {49, 279},
    {50, 280},   {51, 281},   {52, 282},   {53, 283},   {54, 284},
    {55, 285},   {56, 286},   {57, 287},   {58, 288},   {59, 289},
    {60, 290},   {61, 291},   {62, 292},   {63, 293},   {64, 294},
    {65, 295},   {66, 296},   {67, 297},   {68, 298},   {69, 299},
    {70, 300},   {71, 301},   {72, 302},   {73, 330},   {74, 338},
    {75, 339},   {76, 340},   {77, 341},   {78, 342},   {79, 346},
    {80, 347},   {81, 419},   {82, 423},   {83, 445},   {84, 492},
    {85, 495},   {86, 498},   {87, 499},   {88, 533},   {89, 534},
    {90, 535},   {91, 536},   {92, 537},   {93, 538},   {94, 539},
    {95, 540},   {96, 541},   {97, 542},   {98, 543},   {99, 544},
    {100, 545},  {101, 546},  {102, 547},  {103, 548},  {104, 549},
    {105, 550},  {106, 551},  {107, 552},  {108, 553},  {109, 554},
    {110, 555},  {111, 556},  {112, 557},  {113, 558},  {114, 559},
    {115, 560},  {116, 561},  {117, 562},  {118, 563},  {119, 564},
    {120, 565},  {121, 566},  {122, 567},  {123, 568},  {124, 633},
    {125, 676},  {126, 677},  {127, 678},  {128, 679},  {129, 680},
    {130, 681},  {131, 692},  {132, 734},  {133, 735},  {134, 736},
    {135, 737},  {136, 738},  {137, 739},  {138, 740},  {139, 741},
    {140, 742},  {141, 743},  {142, 744},  {143, 745},  {144, 746},
    {145, 747},  {146, 748},  {147, 749},  {148, 750},  {149, 751},
    {150, 752},  {151, 753},  {152, 754},  {153, 755},  {154, 756},
    {155, 757},  {156, 758},  {157, 759},  {158, 760},  {159, 761},
    {160, 762},  {161, 763},  {162, 764},  {163, 765},  {164, 766},
    {165, 767},  {166, 768},  {167, 769},  {168, 790},  {169, 791},
    {170, 815},  {171, 820},  {172, 826},  {173, 827},  {174, 828},
    {175, 829},  {176, 830},  {177, 831},  {178, 832},  {179, 833},
    {180, 834},  {181, 835},  {182, 836},  {183, 837},  {184, 838},
    {185, 839},  {186, 840},  {187, 841},  {188, 842},  {189, 843},
    {190, 844},  {191, 845},  {192, 846},  {193, 847},  {194, 848},
    {195, 849},  {196, 850},  {197, 851},  {198, 852},  {199, 853},
    {200, 854},  {201, 855},  {202, 856},  {203, 857},  {204, 858},
    {205, 859},  {206, 860},  {207, 861},  {208, 864},  {209, 865},
    {210, 866},  {211, 867},  {212, 868},  {213, 869},  {214, 870},
    {215, 874},  {216, 875},  {217, 876},  {218, 884},  {219, 885},
    {220, 886},  {221, 887},  {222, 888},  {223, 889},  {224, 890},
    {225, 891},  {226, 892},  {227, 893},  {228, 894},  {229, 895},
    {230, 896},  {231, 897},  {232, 898},  {233, 899},  {234, 900},
    {235, 901},  {236, 902},  {237, 903},  {238, 904},  {239, 905},
    {240, 906},  {241, 907},  {242, 908},  {243, 909},  {244, 910},
    {245, 911},  {246, 912},  {247, 913},  {248, 914},  {249, 915},
    {250, 916},  {251, 917},  {252, 918},  {253, 919},  {254, 920},
    {255, 921},  {256, 922},  {257, 923},  {258, 924},  {259, 925},
    {260, 926},  {261, 927},  {262, 928},  {263, 929},  {264, 1510},
    {265, 1511}, {266, 1512}, {267, 1515}, {268, 1516}, {269, 1517},
    {270, 1559}, {271, 1560}, {272, 1561}, {273, 1562}, {274, 1563},
    {275, 1564}, {276, 1565}, {277, 1566}, {278, 1567}, {279, 1568},
    {280, 1569}, {281, 1570}, {282, 1571}, {283, 1572}, {284, 1573},
    {285, 1574}, {286, 1575}, {287, 1576}, {288, 1577}, {289, 1578},
    {290, 1579}, {291, 1580}, {292, 1581}, {293, 1582}, {294, 1583},
    {295, 1584}, {296, 1585}, {297, 1586}, {298, 1587}, {299, 1588},
    {300, 1589}, {301, 1590}, {302, 1591}, {303, 1592}, {304, 1593},
    {305, 1594}, {306, 1597}, {307, 1598}, {308, 1599}, {309, 1600},
    {310, 1601}, {311, 1602}, {312, 1603}, {313, 1604}, {314, 1605},
    {315, 1606}, {316, 1607}, {317, 1608}, {318, 1609}, {319, 1610},
    {320, 1611}, {321, 1612}, {322, 1613}, {323, 1614}, {324, 1616},
    {325, 1617}, {326, 1618}, {327, 1638}, {328, 1648}, {329, 1652},
    {330, 1653}, {331, 1656}, {332, 1657}, {333, 1658}, {334, 1659},
    {335, 1701}, {336, 1703}, {337, 1807}, {338, 1859}, {339, 1860},
    {340, 1868}, {341, 1869}, {342, 1870}, {343, 1881}, {344, 1882},
    {345, 1906}, {346, 1925}, {347, 1926}, {348, 1927}, {349, 1928},
    {350, 1929}, {351, 1930}, {352, 1931}, {353, 1932}, {354, 1933},
    {355, 1934}, {356, 1935}, {357, 1936}, {358, 1937}, {359, 1938},
    {360, 1939}, {361, 1940}, {362, 1941}, {363, 1942}, {364, 1943},
    {365, 1944}, {366, 1945}, {367, 1946}, {368, 1947}, {369, 1948},
    {370, 1949}, {371, 1950}, {372, 1951}, {373, 1952}, {374, 1953},
    {375, 1954}, {376, 1955}, {377, 1956}, {378, 1957}, {379, 1958},
    {380, 1959}, {381, 1960}, {382, 1982}, {383, 1983}, {384, 1984},
    {385, 1985}, {386, 1986}, {387, 1989}, {388, 1990}, {389, 2061},
    {390, 2070}, {391, 2076}, {392, 2139}, {393, 2140}, {394, 2141},
    {395, 2142}, {396, 2143}, {397, 2144}, {398, 2145}, {399, 2146},
    {400, 2147}, {401, 2148}, {402, 2149}, {403, 2150}, {404, 2151},
    {405, 2152}, {406, 2153}, {407, 2154}, {408, 2155}, {409, 2156},
    {410, 2157}, {411, 2158}, {412, 2159}, {413, 2160}, {414, 2161},
    {415, 2162}, {416, 2163}, {417, 2164}, {418, 2165}, {419, 2166},
    {420, 2167}, {421, 2168}, {422, 2169}, {423, 2170}, {424, 2171},
    {425, 2172}, {426, 2173}, {427, 2174}, {428, 2215}, {429, 2266},
    {430, 2267}, {431, 2268}, {432, 2269}, {433, 2270}, {434, 2271},
    {435, 2272}, {436, 2273}, {437, 2274}, {438, 2275}, {439, 2276},
    {440, 2277}, {441, 2278}, {442, 2279}, {443, 2280}, {444, 2281},
    {445, 2282}, {446, 2283}, {447, 2284}, {448, 2285}, {449, 2286},
    {450, 2287}, {451, 2288}, {452, 2289}, {453, 2290}, {454, 2291},
    {455, 2292}, {456, 2293}, {457, 2294}, {458, 2295}, {459, 2296},
    {460, 2297}, {461, 2298}, {462, 2299}, {463, 2300}, {464, 2301},
    {465, 2343}, {466, 2344}, {467, 2345}, {468, 2346}, {469, 2347},
    {470, 2348}, {471, 2349}, {472, 2350}, {473, 2351}, {474, 2352},
    {475, 2353}, {476, 2354}, {477, 2355}, {478, 2356}, {479, 2357},
    {480, 2358}, {481, 2359}, {482, 2360}, {483, 2361}, {484, 2362},
    {485, 2363}, {486, 2364}, {487, 2365}, {488, 2366}, {489, 2367},
    {490, 2368}, {491, 2369}, {492, 2370}, {493, 2371}, {494, 2372},
    {495, 2373}, {496, 2374}, {497, 2375}, {498, 2376}, {499, 2377},
    {500, 2378}, {501, 2382}, {502, 2396}, {503, 2397}, {504, 2398},
    {505, 2399}, {506, 2400}, {507, 2401}, {508, 2402}, {509, 2403},
    {510, 2404}, {511, 2405}, {512, 2406}, {513, 2407}, {514, 2408},
    {515, 2409}, {516, 2410}, {517, 2411}, {518, 2412}, {519, 2413},
    {520, 2414}, {521, 2415}, {522, 2416}, {523, 2417}, {524, 2418},
    {525, 2419}, {526, 2420}, {527, 2421}, {528, 2422}, {529, 2423},
    {530, 2424}, {531, 2425}, {532, 2426}, {533, 2427}, {534, 2428},
    {535, 2429}, {536, 2430}, {537, 2431}, {538, 2438}, {539, 2452},
    {540, 2453}, {541, 2454}, {542, 2455}, {543, 2456}, {544, 2457},
    {545, 2458}, {546, 2459}, {547, 2460}, {548, 2461}, {549, 2462},
    {550, 2463}, {551, 2464}, {552, 2465}, {553, 2466}, {554, 2467},
    {555, 2468}, {556, 2469}, {557, 2470}, {558, 2471}, {559, 2472},
    {560, 2473}, {561, 2474}, {562, 2475}, {563, 2476}, {564, 2477},
    {565, 2478}, {566, 2479}, {567, 2480}, {568, 2481}, {569, 2482},
    {570, 2483}, {571, 2484}, {572, 2485}, {573, 2486}, {574, 2487},
    {575, 2502}, {576, 2503}, {577, 2504}, {578, 2505}, {579, 2506},
    {580, 2507}, {581, 2508}, {582, 2509}, {583, 2510}, {584, 2511},
    {585, 2512}, {586, 2513}, {587, 2514}, {588, 2515}, {589, 2516},
    {590, 2517}, {591, 2518}, {592, 2519}, {593, 2520}, {594, 2521},
    {595, 2522}, {596, 2523}, {597, 2524}, {598, 2525}, {599, 2526},
    {600, 2527}, {601, 2528}, {602, 2529}, {603, 2530}, {604, 2531},
    {605, 2532}, {606, 2533}, {607, 2534}, {608, 2535}, {609, 2536},
    {610, 2537}, {611, 2554}, {612, 2555}, {613, 2556}, {614, 2557},
    {615, 2558}, {616, 2559}, {617, 2560}, {618, 2561}, {619, 2562},
    {620, 2563}, {621, 2564}, {622, 2565}, {623, 2566}, {624, 2567},
    {625, 2568}, {626, 2569}, {627, 2570}, {628, 2571}, {629, 2572},
    {630, 2573}, {631, 2574}, {632, 2575}, {633, 2576}, {634, 2577},
    {635, 2578}, {636, 2579}, {637, 2580}, {638, 2581}, {639, 2582},
    {640, 2583}, {641, 2584}, {642, 2585}, {643, 2586}, {644, 2587},
    {645, 2588}, {646, 2589}, {647, 2602}, {648, 2603}, {649, 2611},
    {650, 2612}, {651, 2613}, {652, 2614}, {653, 2615}, {654, 2616},
    {655, 2617}, {656, 2618}, {657, 2619}, {658, 2620}, {659, 2621},
    {660, 2622}, {661, 2623}, {662, 2624}, {663, 2625}, {664, 2626},
    {665, 2627}, {666, 2628}, {667, 2629}, {668, 2630}, {669, 2631},
    {670, 2632}, {671, 2633}, {672, 2634}, {673, 2635}, {674, 2636},
    {675, 2637}, {676, 2638}, {677, 2639}, {678, 2640}, {679, 2641},
    {680, 2669}, {681, 2675}, {682, 2676}, {683, 2711}, {684, 2712},
    {685, 2713}, {686, 2714}, {687, 2715}, {688, 2716}, {689, 2717},
    {690, 2718}, {691, 2719}, {692, 2720}, {693, 2721}, {694, 2722},
    {695, 2723}, {696, 2724}, {697, 2725}, {698, 2726}, {699, 2727},
    {700, 2728}, {701, 2729}, {702, 2730}, {703, 2731}, {704, 2732},
    {705, 2733}, {706, 2734}, {707, 2735}, {708, 2736}, {709, 2737},
    {710, 2738}, {711, 2739}, {712, 2740}, {713, 2741}, {714, 2742},
    {715, 2743}, {716, 2744}, {717, 2745}, {718, 2746}, {719, 2764},
    {720, 2785}, {721, 2786}, {722, 2787}, {723, 2788}, {724, 2789},
    {725, 2790}, {726, 2791}, {727, 2792}, {728, 2793}, {729, 2794},
    {730, 2795}, {731, 2796}, {732, 2797}, {733, 2798}, {734, 2799},
    {735, 2800}, {736, 2801}, {737, 2802}, {738, 2803}, {739, 2804},
    {740, 2805}, {741, 2806}, {742, 2807}, {743, 2808}, {744, 2809},
    {745, 2810}, {746, 2811}, {747, 2812}, {748, 2813}, {749, 2814},
    {750, 2815}, {751, 2816}, {752, 2817}, {753, 2818}, {754, 2819},
    {755, 2820}};
const Str2val kSV_70[] = {
    {143, 0},    {224, 1},    {225, 2},    {226, 3},    {227, 4},
    {228, 5},    {229, 6},    {230, 7},    {231, 8},    {232, 9},
    {233, 10},   {234, 11},   {235, 12},   {236, 13},   {237, 14},
    {238, 15},   {239, 16},   {240, 17},   {241, 18},   {242, 19},
    {243, 20},   {244, 21},   {245, 22},   {246, 23},   {247, 24},
    {248, 25},   {249, 26},   {250, 27},   {251, 28},   {252, 29},
    {253, 30},   {254, 31},   {255, 32},   {256, 33},   {257, 34},
    {258, 35},   {259, 36},   {267, 37},   {268, 38},   {269, 39},
    {270, 40},   {271, 41},   {272, 42},   {273, 43},   {274, 44},
    {275, 45},   {276, 46},   {277, 47},   {278, 48},   {279, 49},
    {280, 50},   {281, 51},   {282, 52},   {283, 53},   {284, 54},
    {285, 55},   {286, 56},   {287, 57},   {288, 58},   {289, 59},
    {290, 60},   {291, 61},   {292, 62},   {293, 63},   {294, 64},
    {295, 65},   {296, 66},   {297, 67},   {298, 68},   {299, 69},
    {300, 70},   {301, 71},   {302, 72},   {330, 73},   {338, 74},
    {339, 75},   {340, 76},   {341, 77},   {342, 78},   {346, 79},
    {347, 80},   {419, 81},   {423, 82},   {445, 83},   {492, 84},
    {495, 85},   {498, 86},   {499, 87},   {533, 88},   {534, 89},
    {535, 90},   {536, 91},   {537, 92},   {538, 93},   {539, 94},
    {540, 95},   {541, 96},   {542, 97},   {543, 98},   {544, 99},
    {545, 100},  {546, 101},  {547, 102},  {548, 103},  {549, 104},
    {550, 105},  {551, 106},  {552, 107},  {553, 108},  {554, 109},
    {555, 110},  {556, 111},  {557, 112},  {558, 113},  {559, 114},
    {560, 115},  {561, 116},  {562, 117},  {563, 118},  {564, 119},
    {565, 120},  {566, 121},  {567, 122},  {568, 123},  {633, 124},
    {676, 125},  {677, 126},  {678, 127},  {679, 128},  {680, 129},
    {681, 130},  {692, 131},  {734, 132},  {735, 133},  {736, 134},
    {737, 135},  {738, 136},  {739, 137},  {740, 138},  {741, 139},
    {742, 140},  {743, 141},  {744, 142},  {745, 143},  {746, 144},
    {747, 145},  {748, 146},  {749, 147},  {750, 148},  {751, 149},
    {752, 150},  {753, 151},  {754, 152},  {755, 153},  {756, 154},
    {757, 155},  {758, 156},  {759, 157},  {760, 158},  {761, 159},
    {762, 160},  {763, 161},  {764, 162},  {765, 163},  {766, 164},
    {767, 165},  {768, 166},  {769, 167},  {790, 168},  {791, 169},
    {815, 170},  {820, 171},  {826, 172},  {827, 173},  {828, 174},
    {829, 175},  {830, 176},  {831, 177},  {832, 178},  {833, 179},
    {834, 180},  {835, 181},  {836, 182},  {837, 183},  {838, 184},
    {839, 185},  {840, 186},  {841, 187},  {842, 188},  {843, 189},
    {844, 190},  {845, 191},  {846, 192},  {847, 193},  {848, 194},
    {849, 195},  {850, 196},  {851, 197},  {852, 198},  {853, 199},
    {854, 200},  {855, 201},  {856, 202},  {857, 203},  {858, 204},
    {859, 205},  {860, 206},  {861, 207},  {864, 208},  {865, 209},
    {866, 210},  {867, 211},  {868, 212},  {869, 213},  {870, 214},
    {874, 215},  {875, 216},  {876, 217},  {884, 218},  {885, 219},
    {886, 220},  {887, 221},  {888, 222},  {889, 223},  {890, 224},
    {891, 225},  {892, 226},  {893, 227},  {894, 228},  {895, 229},
    {896, 230},  {897, 231},  {898, 232},  {899, 233},  {900, 234},
    {901, 235},  {902, 236},  {903, 237},  {904, 238},  {905, 239},
    {906, 240},  {907, 241},  {908, 242},  {909, 243},  {910, 244},
    {911, 245},  {912, 246},  {913, 247},  {914, 248},  {915, 249},
    {916, 250},  {917, 251},  {918, 252},  {919, 253},  {920, 254},
    {921, 255},  {922, 256},  {923, 257},  {924, 258},  {925, 259},
    {926, 260},  {927, 261},  {928, 262},  {929, 263},  {1510, 264},
    {1511, 265}, {1512, 266}, {1515, 267}, {1516, 268}, {1517, 269},
    {1559, 270}, {1560, 271}, {1561, 272}, {1562, 273}, {1563, 274},
    {1564, 275}, {1565, 276}, {1566, 277}, {1567, 278}, {1568, 279},
    {1569, 280}, {1570, 281}, {1571, 282}, {1572, 283}, {1573, 284},
    {1574, 285}, {1575, 286}, {1576, 287}, {1577, 288}, {1578, 289},
    {1579, 290}, {1580, 291}, {1581, 292}, {1582, 293}, {1583, 294},
    {1584, 295}, {1585, 296}, {1586, 297}, {1587, 298}, {1588, 299},
    {1589, 300}, {1590, 301}, {1591, 302}, {1592, 303}, {1593, 304},
    {1594, 305}, {1597, 306}, {1598, 307}, {1599, 308}, {1600, 309},
    {1601, 310}, {1602, 311}, {1603, 312}, {1604, 313}, {1605, 314},
    {1606, 315}, {1607, 316}, {1608, 317}, {1609, 318}, {1610, 319},
    {1611, 320}, {1612, 321}, {1613, 322}, {1614, 323}, {1616, 324},
    {1617, 325}, {1618, 326}, {1638, 327}, {1648, 328}, {1652, 329},
    {1653, 330}, {1656, 331}, {1657, 332}, {1658, 333}, {1659, 334},
    {1701, 335}, {1703, 336}, {1807, 337}, {1859, 338}, {1860, 339},
    {1868, 340}, {1869, 341}, {1870, 342}, {1881, 343}, {1882, 344},
    {1906, 345}, {1925, 346}, {1926, 347}, {1927, 348}, {1928, 349},
    {1929, 350}, {1930, 351}, {1931, 352}, {1932, 353}, {1933, 354},
    {1934, 355}, {1935, 356}, {1936, 357}, {1937, 358}, {1938, 359},
    {1939, 360}, {1940, 361}, {1941, 362}, {1942, 363}, {1943, 364},
    {1944, 365}, {1945, 366}, {1946, 367}, {1947, 368}, {1948, 369},
    {1949, 370}, {1950, 371}, {1951, 372}, {1952, 373}, {1953, 374},
    {1954, 375}, {1955, 376}, {1956, 377}, {1957, 378}, {1958, 379},
    {1959, 380}, {1960, 381}, {1982, 382}, {1983, 383}, {1984, 384},
    {1985, 385}, {1986, 386}, {1989, 387}, {1990, 388}, {2061, 389},
    {2070, 390}, {2076, 391}, {2139, 392}, {2140, 393}, {2141, 394},
    {2142, 395}, {2143, 396}, {2144, 397}, {2145, 398}, {2146, 399},
    {2147, 400}, {2148, 401}, {2149, 402}, {2150, 403}, {2151, 404},
    {2152, 405}, {2153, 406}, {2154, 407}, {2155, 408}, {2156, 409},
    {2157, 410}, {2158, 411}, {2159, 412}, {2160, 413}, {2161, 414},
    {2162, 415}, {2163, 416}, {2164, 417}, {2165, 418}, {2166, 419},
    {2167, 420}, {2168, 421}, {2169, 422}, {2170, 423}, {2171, 424},
    {2172, 425}, {2173, 426}, {2174, 427}, {2215, 428}, {2266, 429},
    {2267, 430}, {2268, 431}, {2269, 432}, {2270, 433}, {2271, 434},
    {2272, 435}, {2273, 436}, {2274, 437}, {2275, 438}, {2276, 439},
    {2277, 440}, {2278, 441}, {2279, 442}, {2280, 443}, {2281, 444},
    {2282, 445}, {2283, 446}, {2284, 447}, {2285, 448}, {2286, 449},
    {2287, 450}, {2288, 451}, {2289, 452}, {2290, 453}, {2291, 454},
    {2292, 455}, {2293, 456}, {2294, 457}, {2295, 458}, {2296, 459},
    {2297, 460}, {2298, 461}, {2299, 462}, {2300, 463}, {2301, 464},
    {2343, 465}, {2344, 466}, {2345, 467}, {2346, 468}, {2347, 469},
    {2348, 470}, {2349, 471}, {2350, 472}, {2351, 473}, {2352, 474},
    {2353, 475}, {2354, 476}, {2355, 477}, {2356, 478}, {2357, 479},
    {2358, 480}, {2359, 481}, {2360, 482}, {2361, 483}, {2362, 484},
    {2363, 485}, {2364, 486}, {2365, 487}, {2366, 488}, {2367, 489},
    {2368, 490}, {2369, 491}, {2370, 492}, {2371, 493}, {2372, 494},
    {2373, 495}, {2374, 496}, {2375, 497}, {2376, 498}, {2377, 499},
    {2378, 500}, {2382, 501}, {2396, 502}, {2397, 503}, {2398, 504},
    {2399, 505}, {2400, 506}, {2401, 507}, {2402, 508}, {2403, 509},
    {2404, 510}, {2405, 511}, {2406, 512}, {2407, 513}, {2408, 514},
    {2409, 515}, {2410, 516}, {2411, 517}, {2412, 518}, {2413, 519},
    {2414, 520}, {2415, 521}, {2416, 522}, {2417, 523}, {2418, 524},
    {2419, 525}, {2420, 526}, {2421, 527}, {2422, 528}, {2423, 529},
    {2424, 530}, {2425, 531}, {2426, 532}, {2427, 533}, {2428, 534},
    {2429, 535}, {2430, 536}, {2431, 537}, {2438, 538}, {2452, 539},
    {2453, 540}, {2454, 541}, {2455, 542}, {2456, 543}, {2457, 544},
    {2458, 545}, {2459, 546}, {2460, 547}, {2461, 548}, {2462, 549},
    {2463, 550}, {2464, 551}, {2465, 552}, {2466, 553}, {2467, 554},
    {2468, 555}, {2469, 556}, {2470, 557}, {2471, 558}, {2472, 559},
    {2473, 560}, {2474, 561}, {2475, 562}, {2476, 563}, {2477, 564},
    {2478, 565}, {2479, 566}, {2480, 567}, {2481, 568}, {2482, 569},
    {2483, 570}, {2484, 571}, {2485, 572}, {2486, 573}, {2487, 574},
    {2502, 575}, {2503, 576}, {2504, 577}, {2505, 578}, {2506, 579},
    {2507, 580}, {2508, 581}, {2509, 582}, {2510, 583}, {2511, 584},
    {2512, 585}, {2513, 586}, {2514, 587}, {2515, 588}, {2516, 589},
    {2517, 590}, {2518, 591}, {2519, 592}, {2520, 593}, {2521, 594},
    {2522, 595}, {2523, 596}, {2524, 597}, {2525, 598}, {2526, 599},
    {2527, 600}, {2528, 601}, {2529, 602}, {2530, 603}, {2531, 604},
    {2532, 605}, {2533, 606}, {2534, 607}, {2535, 608}, {2536, 609},
    {2537, 610}, {2554, 611}, {2555, 612}, {2556, 613}, {2557, 614},
    {2558, 615}, {2559, 616}, {2560, 617}, {2561, 618}, {2562, 619},
    {2563, 620}, {2564, 621}, {2565, 622}, {2566, 623}, {2567, 624},
    {2568, 625}, {2569, 626}, {2570, 627}, {2571, 628}, {2572, 629},
    {2573, 630}, {2574, 631}, {2575, 632}, {2576, 633}, {2577, 634},
    {2578, 635}, {2579, 636}, {2580, 637}, {2581, 638}, {2582, 639},
    {2583, 640}, {2584, 641}, {2585, 642}, {2586, 643}, {2587, 644},
    {2588, 645}, {2589, 646}, {2602, 647}, {2603, 648}, {2611, 649},
    {2612, 650}, {2613, 651}, {2614, 652}, {2615, 653}, {2616, 654},
    {2617, 655}, {2618, 656}, {2619, 657}, {2620, 658}, {2621, 659},
    {2622, 660}, {2623, 661}, {2624, 662}, {2625, 663}, {2626, 664},
    {2627, 665}, {2628, 666}, {2629, 667}, {2630, 668}, {2631, 669},
    {2632, 670}, {2633, 671}, {2634, 672}, {2635, 673}, {2636, 674},
    {2637, 675}, {2638, 676}, {2639, 677}, {2640, 678}, {2641, 679},
    {2669, 680}, {2675, 681}, {2676, 682}, {2711, 683}, {2712, 684},
    {2713, 685}, {2714, 686}, {2715, 687}, {2716, 688}, {2717, 689},
    {2718, 690}, {2719, 691}, {2720, 692}, {2721, 693}, {2722, 694},
    {2723, 695}, {2724, 696}, {2725, 697}, {2726, 698}, {2727, 699},
    {2728, 700}, {2729, 701}, {2730, 702}, {2731, 703}, {2732, 704},
    {2733, 705}, {2734, 706}, {2735, 707}, {2736, 708}, {2737, 709},
    {2738, 710}, {2739, 711}, {2740, 712}, {2741, 713}, {2742, 714},
    {2743, 715}, {2744, 716}, {2745, 717}, {2746, 718}, {2764, 719},
    {2785, 720}, {2786, 721}, {2787, 722}, {2788, 723}, {2789, 724},
    {2790, 725}, {2791, 726}, {2792, 727}, {2793, 728}, {2794, 729},
    {2795, 730}, {2796, 731}, {2797, 732}, {2798, 733}, {2799, 734},
    {2800, 735}, {2801, 736}, {2802, 737}, {2803, 738}, {2804, 739},
    {2805, 740}, {2806, 741}, {2807, 742}, {2808, 743}, {2809, 744},
    {2810, 745}, {2811, 746}, {2812, 747}, {2813, 748}, {2814, 749},
    {2815, 750}, {2816, 751}, {2817, 752}, {2818, 753}, {2819, 754},
    {2820, 755}};
const Val2str kVS_71[] = {{0, 1625}, {1, 1630}, {2, 2115}};
const Str2val kSV_71[] = {{1625, 0}, {1630, 1}, {2115, 2}};
const Val2str kVS_72[] = {{0, 718}, {1, 1596}, {2, 1808}, {3, 2236}};
const Str2val kSV_72[] = {{718, 0}, {1596, 1}, {1808, 2}, {2236, 3}};
const Val2str kVS_73[] = {
    {0, 138},   {1, 137},   {2, 309},   {3, 311},   {4, 310},   {5, 401},
    {6, 400},   {7, 516},   {8, 515},   {9, 518},   {10, 517},  {11, 520},
    {12, 519},  {13, 522},  {14, 521},  {15, 524},  {16, 523},  {17, 526},
    {18, 525},  {19, 528},  {20, 527},  {21, 530},  {22, 529},  {23, 532},
    {24, 531},  {25, 504},  {26, 503},  {27, 506},  {28, 505},  {29, 508},
    {30, 507},  {31, 510},  {32, 509},  {33, 512},  {34, 511},  {35, 514},
    {36, 513},  {37, 2221}, {38, 2220}, {39, 2320}, {40, 2322}, {41, 2321},
    {42, 2440}, {43, 2439}};
const Str2val kSV_73[] = {
    {137, 1},   {138, 0},   {309, 2},   {310, 4},   {311, 3},   {400, 6},
    {401, 5},   {503, 26},  {504, 25},  {505, 28},  {506, 27},  {507, 30},
    {508, 29},  {509, 32},  {510, 31},  {511, 34},  {512, 33},  {513, 36},
    {514, 35},  {515, 8},   {516, 7},   {517, 10},  {518, 9},   {519, 12},
    {520, 11},  {521, 14},  {522, 13},  {523, 16},  {524, 15},  {525, 18},
    {526, 17},  {527, 20},  {528, 19},  {529, 22},  {530, 21},  {531, 24},
    {532, 23},  {2220, 38}, {2221, 37}, {2320, 39}, {2321, 41}, {2322, 40},
    {2439, 43}, {2440, 42}};
const Val2str kVS_74[] = {{0, 144},   {1, 588},  {2, 629},   {3, 1317},
                          {4, 1353},  {5, 1470}, {6, 2041},  {7, 2208},
                          {8, 2211},  {9, 2212}, {10, 2608}, {11, 2610},
                          {12, 2652}, {13, 2657}};
const Str2val kSV_74[] = {{144, 0},   {588, 1},  {629, 2},   {1317, 3},
                          {1353, 4},  {1470, 5}, {2041, 6},  {2208, 7},
                          {2211, 8},  {2212, 9}, {2608, 10}, {2610, 11},
                          {2652, 12}, {2657, 13}};
const Val2str kVS_75[] = {{0, 1807}, {1, 2026}, {2, 2254}};
const Str2val kSV_75[] = {{1807, 0}, {2026, 1}, {2254, 2}};
const Val2str kVS_76[] = {{0, 317}, {1, 654}, {2, 1807}, {3, 2395}, {4, 2538}};
const Str2val kSV_76[] = {{317, 0}, {654, 1}, {1807, 2}, {2395, 3}, {2538, 4}};
const Val2str kVS_77[] = {{0, 409}, {1, 2761}};
const Str2val kSV_77[] = {{409, 0}, {2761, 1}};
const Val2str kVS_78[] = {
    {0, 2642},    {1, 2645},    {2, 2643},    {3, 2646},    {5, 2647},
    {7, 2644},    {1024, 375},  {1025, 387},  {1026, 390},  {1027, 391},
    {1028, 394},  {1029, 397},  {1030, 393},  {1031, 388},  {1032, 395},
    {1033, 396},  {1034, 382},  {1035, 374},  {1036, 399},  {1037, 376},
    {1038, 379},  {1039, 378},  {1040, 377},  {1041, 381},  {1042, 380},
    {1043, 373},  {1044, 389},  {1045, 398},  {1048, 383},  {1049, 384},
    {1050, 385},  {1051, 386},  {1052, 371},  {1053, 370},  {1054, 372},
    {1055, 369},  {1056, 392},  {1280, 2308}, {1281, 2312}, {1282, 2314},
    {1283, 2318}, {1284, 2307}, {1285, 2315}, {1286, 2311}, {1287, 2306},
    {1288, 2309}, {1289, 2310}, {1290, 2313}, {1291, 2317}, {1292, 2316}};
const Str2val kSV_78[] = {
    {369, 1055},  {370, 1053},  {371, 1052},  {372, 1054},  {373, 1043},
    {374, 1035},  {375, 1024},  {376, 1037},  {377, 1040},  {378, 1039},
    {379, 1038},  {380, 1042},  {381, 1041},  {382, 1034},  {383, 1048},
    {384, 1049},  {385, 1050},  {386, 1051},  {387, 1025},  {388, 1031},
    {389, 1044},  {390, 1026},  {391, 1027},  {392, 1056},  {393, 1030},
    {394, 1028},  {395, 1032},  {396, 1033},  {397, 1029},  {398, 1045},
    {399, 1036},  {2306, 1287}, {2307, 1284}, {2308, 1280}, {2309, 1288},
    {2310, 1289}, {2311, 1286}, {2312, 1281}, {2313, 1290}, {2314, 1282},
    {2315, 1285}, {2316, 1292}, {2317, 1291}, {2318, 1283}, {2642, 0},
    {2643, 2},    {2644, 7},    {2645, 1},    {2646, 3},    {2647, 5}};
const Val2str kVS_79[] = {{0, 195}, {1, 450}, {2, 2783}};
const Str2val kSV_79[] = {{195, 0}, {450, 1}, {2783, 2}};
const Val2str kVS_80[] = {{0, 318}, {1, 1521}, {2, 2222}, {3, 2677}};
const Str2val kSV_80[] = {{318, 0}, {1521, 1}, {2222, 2}, {2677, 3}};
const Val2str kVS_81[] = {{0, 636},  {1, 788},  {2, 1904},
                          {3, 1924}, {4, 2257}, {5, 2659}};
const Str2val kSV_81[] = {{636, 0},  {788, 1},  {1904, 2},
                          {1924, 3}, {2257, 4}, {2659, 5}};
const Val2str kVS_82[] = {{0, 139}, {1, 140}, {2, 141}, {3, 142}};
const Str2val kSV_82[] = {{139, 0}, {140, 1}, {141, 2}, {142, 3}};
const Val2str kVS_83[] = {{0, 260},  {1, 337},  {2, 569}, {3, 1801},
                          {4, 1807}, {5, 1825}, {6, 2199}};
const Str2val kSV_83[] = {{260, 0},  {337, 1},  {569, 2}, {1801, 3},
                          {1807, 4}, {1825, 5}, {2199, 6}};
const Val2str kVS_84[] = {{0, 1807}, {1, 2441}, {2, 2670}};
const Str2val kSV_84[] = {{1807, 0}, {2441, 1}, {2670, 2}};
const Val2str kVS_85[] = {{0, 125},  {1, 144},  {2, 332},   {3, 414},
                          {4, 696},  {5, 1810}, {6, 1920},  {7, 1921},
                          {8, 2109}, {9, 2110}, {10, 2114}, {11, 2255}};
const Str2val kSV_85[] = {{125, 0},  {144, 1},  {332, 2},   {414, 3},
                          {696, 4},  {1810, 5}, {1920, 6},  {1921, 7},
                          {2109, 8}, {2110, 9}, {2114, 10}, {2255, 11}};
const Val2str kVS_86[] = {{0, 336}, {1, 1521}, {2, 1807}, {3, 2222}};
const Str2val kSV_86[] = {{336, 0}, {1521, 1}, {1807, 2}, {2222, 3}};
const Val2str kVS_87[] = {};
const Str2val kSV_87[] = {};
const Val2str kVS_88[] = {};
const Str2val kSV_88[] = {};
const Val2str kVS_89[] = {{0, 318}, {1, 336}, {2, 1807}, {3, 2677}};
const Str2val kSV_89[] = {{318, 0}, {336, 1}, {1807, 2}, {2677, 3}};

// Represents all values from a single enum, sorted in both orders (by numerical
// values and by string representations).
struct EnumVals {
  unsigned size;
  const Val2str* vs;
  const Str2val* sv;
  const char* str(uint16_t val) const {
    const Val2str e = {val, 0};
    auto it = std::lower_bound(vs, vs + size, e);
    if (it == vs + size || it->val != val)
      return "";
    return kAllStrings[it->str];
  }
  bool val(const char* str, uint16_t* out) const {
    auto comp_less = [](const Str2val& a, const char* b) {
      return (strcmp(kAllStrings[a.str], b) < 0);
    };
    auto it = std::lower_bound(sv, sv + size, str, comp_less);
    if (it == sv + size || strcmp(kAllStrings[it->str], str) != 0)
      return false;
    *out = it->val;
    return true;
  }
};

// An array of all enums, indexes correspond to enum ids.
const EnumVals kAllEnums[] = {
    {0, kVS_0, kSV_0},     {9, kVS_1, kSV_1},     {597, kVS_2, kSV_2},
    {4, kVS_3, kSV_3},     {3, kVS_4, kSV_4},     {2, kVS_5, kSV_5},
    {4, kVS_6, kSV_6},     {8, kVS_7, kSV_7},     {3, kVS_8, kSV_8},
    {10, kVS_9, kSV_9},    {4, kVS_10, kSV_10},   {3, kVS_11, kSV_11},
    {5, kVS_12, kSV_12},   {3, kVS_13, kSV_13},   {2, kVS_14, kSV_14},
    {5, kVS_15, kSV_15},   {8, kVS_16, kSV_16},   {1, kVS_17, kSV_17},
    {2, kVS_18, kSV_18},   {158, kVS_19, kSV_19}, {70, kVS_20, kSV_20},
    {2, kVS_21, kSV_21},   {4, kVS_22, kSV_22},   {2, kVS_23, kSV_23},
    {5, kVS_24, kSV_24},   {3, kVS_25, kSV_25},   {3, kVS_26, kSV_26},
    {12, kVS_27, kSV_27},  {5, kVS_28, kSV_28},   {3, kVS_29, kSV_29},
    {43, kVS_30, kSV_30},  {2, kVS_31, kSV_31},   {3, kVS_32, kSV_32},
    {8, kVS_33, kSV_33},   {4, kVS_34, kSV_34},   {2, kVS_35, kSV_35},
    {8, kVS_36, kSV_36},   {0, kVS_37, kSV_37},   {19, kVS_38, kSV_38},
    {6, kVS_39, kSV_39},   {3, kVS_40, kSV_40},   {7, kVS_41, kSV_41},
    {71, kVS_42, kSV_42},  {6, kVS_43, kSV_43},   {80, kVS_44, kSV_44},
    {535, kVS_45, kSV_45}, {6, kVS_46, kSV_46},   {2, kVS_47, kSV_47},
    {19, kVS_48, kSV_48},  {0, kVS_49, kSV_49},   {3, kVS_50, kSV_50},
    {490, kVS_51, kSV_51}, {50, kVS_52, kSV_52},  {10, kVS_53, kSV_53},
    {123, kVS_54, kSV_54}, {4, kVS_55, kSV_55},   {3, kVS_56, kSV_56},
    {34, kVS_57, kSV_57},  {1, kVS_58, kSV_58},   {114, kVS_59, kSV_59},
    {5, kVS_60, kSV_60},   {17, kVS_61, kSV_61},  {3, kVS_62, kSV_62},
    {3, kVS_63, kSV_63},   {8, kVS_64, kSV_64},   {8, kVS_65, kSV_65},
    {5, kVS_66, kSV_66},   {6, kVS_67, kSV_67},   {5, kVS_68, kSV_68},
    {3, kVS_69, kSV_69},   {756, kVS_70, kSV_70}, {3, kVS_71, kSV_71},
    {4, kVS_72, kSV_72},   {44, kVS_73, kSV_73},  {14, kVS_74, kSV_74},
    {3, kVS_75, kSV_75},   {5, kVS_76, kSV_76},   {2, kVS_77, kSV_77},
    {50, kVS_78, kSV_78},  {3, kVS_79, kSV_79},   {4, kVS_80, kSV_80},
    {6, kVS_81, kSV_81},   {4, kVS_82, kSV_82},   {7, kVS_83, kSV_83},
    {3, kVS_84, kSV_84},   {12, kVS_85, kSV_85},  {4, kVS_86, kSV_86},
    {0, kVS_87, kSV_87},   {0, kVS_88, kSV_88},   {4, kVS_89, kSV_89}};

// An array with mapping AttrName->enum_id, names not binded to enums are set to
// 0, which points to empty enum.
const uint16_t kAttrNameToEnumId[] = {
    0,  0,  0,  0,  3,  0,  4,  4,  5,  5,  0,  6,  6,  7,  7,  0,  0,  0,  8,
    8,  9,  9,  0,  10, 10, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  11, 0,  0,
    0,  11, 12, 0,  13, 13, 14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    15, 15, 15, 0,  0,  0,  0,  16, 0,  0,  0,  0,  17, 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  18, 18, 19, 19,
    20, 0,  0,  0,  0,  0,  20, 20, 20, 0,  0,  21, 21, 0,  0,  6,  6,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  22, 22, 22, 23, 0,  24, 25, 26, 0,  0,  0,  0,
    0,  0,  0,  0,  27, 28, 0,  0,  0,  0,  0,  29, 29, 29, 30, 0,  0,  0,  31,
    0,  0,  0,  0,  0,  0,  0,  32, 0,  0,  0,  0,  0,  0,  0,  0,  11, 0,  0,
    0,  11, 33, 33, 33, 0,  0,  0,  0,  34, 34, 34, 0,  0,  0,  31, 35, 20, 0,
    0,  0,  0,  20, 20, 20, 36, 36, 36, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    37, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  38,
    38, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  39, 0,  0,  0,  39, 39, 40, 41, 0,  42, 0,  0,  0,  0,  0,  0,
    8,  8,  43, 43, 0,  0,  44, 0,  0,  45, 46, 46, 0,  0,  0,  0,  0,  0,  0,
    44, 44, 45, 46, 46, 47, 47, 0,  0,  0,  0,  48, 49, 0,  0,  0,  0,  50, 50,
    51, 31, 31, 0,  0,  0,  0,  0,  0,  52, 18, 24, 0,  52, 45, 0,  0,  53, 53,
    0,  0,  54, 54, 0,  0,  0,  0,  0,  55, 55, 55, 0,  0,  56, 0,  0,  57, 57,
    57, 0,  0,  58, 58, 0,  0,  0,  0,  0,  0,  0,  0,  59, 24, 24, 24, 0,  30,
    30, 30, 0,  0,  0,  0,  0,  0,  0,  0,  0,  60, 60, 60, 14, 14, 14, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  61, 0,  0,  0,  0,  0,  0,  0,  0,  0,  62,
    63, 0,  64, 64, 64, 65, 65, 65, 66, 66, 66, 25, 25, 25, 67, 67, 67, 68, 0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  69, 0,  0,  0,  70, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  71, 0,  0,  0,  0,  0,  0,  6,  6,  0,
    72, 73, 0,  0,  74, 0,  0,  0,  0,  0,  0,  75, 75, 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  76, 77, 77, 77, 0,  0,  26, 26, 26, 78, 0,  0,
    0,  0,  0,  0,  79, 79, 0,  0,  80, 80, 0,  0,  0,  0,  0,  0,  0,  6,  6,
    81, 81, 82, 82, 83, 84, 0,  85, 85, 0,  86, 86, 86, 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  87, 83, 88, 84, 0,  0,  0,  89, 89, 89, 0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0};
}  // namespace

namespace ipp {

std::string ToString(AttrName name, int value) {
  const uint16_t i = static_cast<uint16_t>(name);
  if (i >= 598)
    return "";
  return kAllEnums[kAttrNameToEnumId[i]].str(value);
}

std::string ToString(GroupTag v) {
  return kAllEnums[1].str(static_cast<uint16_t>(v));
}
std::string ToString(AttrName v) {
  return kAllEnums[2].str(static_cast<uint16_t>(v));
}
std::string ToString(E_auth_info_required v) {
  return kAllEnums[3].str(static_cast<uint16_t>(v));
}
std::string ToString(E_baling_type v) {
  return kAllEnums[4].str(static_cast<uint16_t>(v));
}
std::string ToString(E_baling_when v) {
  return kAllEnums[5].str(static_cast<uint16_t>(v));
}
std::string ToString(E_binding_reference_edge v) {
  return kAllEnums[6].str(static_cast<uint16_t>(v));
}
std::string ToString(E_binding_type v) {
  return kAllEnums[7].str(static_cast<uint16_t>(v));
}
std::string ToString(E_coating_sides v) {
  return kAllEnums[8].str(static_cast<uint16_t>(v));
}
std::string ToString(E_coating_type v) {
  return kAllEnums[9].str(static_cast<uint16_t>(v));
}
std::string ToString(E_compression v) {
  return kAllEnums[10].str(static_cast<uint16_t>(v));
}
std::string ToString(E_cover_back_supported v) {
  return kAllEnums[11].str(static_cast<uint16_t>(v));
}
std::string ToString(E_cover_type v) {
  return kAllEnums[12].str(static_cast<uint16_t>(v));
}
std::string ToString(E_covering_name v) {
  return kAllEnums[13].str(static_cast<uint16_t>(v));
}
std::string ToString(E_current_page_order v) {
  return kAllEnums[14].str(static_cast<uint16_t>(v));
}
std::string ToString(E_document_digital_signature v) {
  return kAllEnums[15].str(static_cast<uint16_t>(v));
}
std::string ToString(E_document_format_details_supported v) {
  return kAllEnums[16].str(static_cast<uint16_t>(v));
}
std::string ToString(E_document_format_varying_attributes v) {
  return kAllEnums[17].str(static_cast<uint16_t>(v));
}
std::string ToString(E_feed_orientation v) {
  return kAllEnums[18].str(static_cast<uint16_t>(v));
}
std::string ToString(E_finishing_template v) {
  return kAllEnums[19].str(static_cast<uint16_t>(v));
}
std::string ToString(E_finishings v) {
  return kAllEnums[20].str(static_cast<uint16_t>(v));
}
std::string ToString(E_folding_direction v) {
  return kAllEnums[21].str(static_cast<uint16_t>(v));
}
std::string ToString(E_identify_actions v) {
  return kAllEnums[22].str(static_cast<uint16_t>(v));
}
std::string ToString(E_imposition_template v) {
  return kAllEnums[23].str(static_cast<uint16_t>(v));
}
std::string ToString(E_input_orientation_requested v) {
  return kAllEnums[24].str(static_cast<uint16_t>(v));
}
std::string ToString(E_input_quality v) {
  return kAllEnums[25].str(static_cast<uint16_t>(v));
}
std::string ToString(E_input_sides v) {
  return kAllEnums[26].str(static_cast<uint16_t>(v));
}
std::string ToString(E_ipp_features_supported v) {
  return kAllEnums[27].str(static_cast<uint16_t>(v));
}
std::string ToString(E_ipp_versions_supported v) {
  return kAllEnums[28].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_account_type v) {
  return kAllEnums[29].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_accounting_output_bin v) {
  return kAllEnums[30].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_accounting_sheets_type v) {
  return kAllEnums[31].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_collation_type v) {
  return kAllEnums[32].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_delay_output_until v) {
  return kAllEnums[33].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_error_action v) {
  return kAllEnums[34].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_error_sheet_when v) {
  return kAllEnums[35].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_hold_until v) {
  return kAllEnums[36].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_mandatory_attributes v) {
  return kAllEnums[37].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_password_encryption v) {
  return kAllEnums[38].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_sheets v) {
  return kAllEnums[39].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_spooling_supported v) {
  return kAllEnums[40].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_state v) {
  return kAllEnums[41].str(static_cast<uint16_t>(v));
}
std::string ToString(E_job_state_reasons v) {
  return kAllEnums[42].str(static_cast<uint16_t>(v));
}
std::string ToString(E_laminating_type v) {
  return kAllEnums[43].str(static_cast<uint16_t>(v));
}
std::string ToString(E_material_color v) {
  return kAllEnums[44].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media v) {
  return kAllEnums[45].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_back_coating v) {
  return kAllEnums[46].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_grain v) {
  return kAllEnums[47].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_input_tray_check v) {
  return kAllEnums[48].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_key v) {
  return kAllEnums[49].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_pre_printed v) {
  return kAllEnums[50].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_ready v) {
  return kAllEnums[51].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_source v) {
  return kAllEnums[52].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_tooth v) {
  return kAllEnums[53].str(static_cast<uint16_t>(v));
}
std::string ToString(E_media_type v) {
  return kAllEnums[54].str(static_cast<uint16_t>(v));
}
std::string ToString(E_multiple_document_handling v) {
  return kAllEnums[55].str(static_cast<uint16_t>(v));
}
std::string ToString(E_multiple_operation_time_out_action v) {
  return kAllEnums[56].str(static_cast<uint16_t>(v));
}
std::string ToString(E_notify_events v) {
  return kAllEnums[57].str(static_cast<uint16_t>(v));
}
std::string ToString(E_notify_pull_method v) {
  return kAllEnums[58].str(static_cast<uint16_t>(v));
}
std::string ToString(E_operations_supported v) {
  return kAllEnums[59].str(static_cast<uint16_t>(v));
}
std::string ToString(E_page_delivery v) {
  return kAllEnums[60].str(static_cast<uint16_t>(v));
}
std::string ToString(E_pdf_versions_supported v) {
  return kAllEnums[61].str(static_cast<uint16_t>(v));
}
std::string ToString(E_pdl_init_file_supported v) {
  return kAllEnums[62].str(static_cast<uint16_t>(v));
}
std::string ToString(E_pdl_override_supported v) {
  return kAllEnums[63].str(static_cast<uint16_t>(v));
}
std::string ToString(E_presentation_direction_number_up v) {
  return kAllEnums[64].str(static_cast<uint16_t>(v));
}
std::string ToString(E_print_color_mode v) {
  return kAllEnums[65].str(static_cast<uint16_t>(v));
}
std::string ToString(E_print_content_optimize v) {
  return kAllEnums[66].str(static_cast<uint16_t>(v));
}
std::string ToString(E_print_rendering_intent v) {
  return kAllEnums[67].str(static_cast<uint16_t>(v));
}
std::string ToString(E_print_scaling v) {
  return kAllEnums[68].str(static_cast<uint16_t>(v));
}
std::string ToString(E_printer_state v) {
  return kAllEnums[69].str(static_cast<uint16_t>(v));
}
std::string ToString(E_printer_state_reasons v) {
  return kAllEnums[70].str(static_cast<uint16_t>(v));
}
std::string ToString(E_proof_print_supported v) {
  return kAllEnums[71].str(static_cast<uint16_t>(v));
}
std::string ToString(E_pwg_raster_document_sheet_back v) {
  return kAllEnums[72].str(static_cast<uint16_t>(v));
}
std::string ToString(E_pwg_raster_document_type_supported v) {
  return kAllEnums[73].str(static_cast<uint16_t>(v));
}
std::string ToString(E_requested_attributes v) {
  return kAllEnums[74].str(static_cast<uint16_t>(v));
}
std::string ToString(E_save_disposition v) {
  return kAllEnums[75].str(static_cast<uint16_t>(v));
}
std::string ToString(E_separator_sheets_type v) {
  return kAllEnums[76].str(static_cast<uint16_t>(v));
}
std::string ToString(E_sheet_collate v) {
  return kAllEnums[77].str(static_cast<uint16_t>(v));
}
std::string ToString(E_status_code v) {
  return kAllEnums[78].str(static_cast<uint16_t>(v));
}
std::string ToString(E_stitching_method v) {
  return kAllEnums[79].str(static_cast<uint16_t>(v));
}
std::string ToString(E_stitching_reference_edge v) {
  return kAllEnums[80].str(static_cast<uint16_t>(v));
}
std::string ToString(E_trimming_type v) {
  return kAllEnums[81].str(static_cast<uint16_t>(v));
}
std::string ToString(E_trimming_when v) {
  return kAllEnums[82].str(static_cast<uint16_t>(v));
}
std::string ToString(E_uri_authentication_supported v) {
  return kAllEnums[83].str(static_cast<uint16_t>(v));
}
std::string ToString(E_uri_security_supported v) {
  return kAllEnums[84].str(static_cast<uint16_t>(v));
}
std::string ToString(E_which_jobs v) {
  return kAllEnums[85].str(static_cast<uint16_t>(v));
}
std::string ToString(E_x_image_position v) {
  return kAllEnums[86].str(static_cast<uint16_t>(v));
}
std::string ToString(E_xri_authentication v) {
  return kAllEnums[87].str(static_cast<uint16_t>(v));
}
std::string ToString(E_xri_security v) {
  return kAllEnums[88].str(static_cast<uint16_t>(v));
}
std::string ToString(E_y_image_position v) {
  return kAllEnums[89].str(static_cast<uint16_t>(v));
}

bool FromString(const std::string& s, AttrName name, int* value) {
  if (value == nullptr)
    return false;
  const uint16_t i = static_cast<uint16_t>(name);
  if (i >= 598)
    return false;
  uint16_t val;
  if (!(kAllEnums[kAttrNameToEnumId[i]].val(s.c_str(), &val)))
    return false;
  *value = val;
  return true;
}

bool FromString(const std::string& s, GroupTag* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[1].val(s.c_str(), &val)))
    return false;
  *v = static_cast<GroupTag>(val);
  return true;
}
bool FromString(const std::string& s, AttrName* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[2].val(s.c_str(), &val)))
    return false;
  *v = static_cast<AttrName>(val);
  return true;
}
bool FromString(const std::string& s, E_auth_info_required* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[3].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_auth_info_required>(val);
  return true;
}
bool FromString(const std::string& s, E_baling_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[4].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_baling_type>(val);
  return true;
}
bool FromString(const std::string& s, E_baling_when* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[5].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_baling_when>(val);
  return true;
}
bool FromString(const std::string& s, E_binding_reference_edge* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[6].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_binding_reference_edge>(val);
  return true;
}
bool FromString(const std::string& s, E_binding_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[7].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_binding_type>(val);
  return true;
}
bool FromString(const std::string& s, E_coating_sides* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[8].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_coating_sides>(val);
  return true;
}
bool FromString(const std::string& s, E_coating_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[9].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_coating_type>(val);
  return true;
}
bool FromString(const std::string& s, E_compression* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[10].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_compression>(val);
  return true;
}
bool FromString(const std::string& s, E_cover_back_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[11].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_cover_back_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_cover_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[12].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_cover_type>(val);
  return true;
}
bool FromString(const std::string& s, E_covering_name* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[13].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_covering_name>(val);
  return true;
}
bool FromString(const std::string& s, E_current_page_order* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[14].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_current_page_order>(val);
  return true;
}
bool FromString(const std::string& s, E_document_digital_signature* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[15].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_document_digital_signature>(val);
  return true;
}
bool FromString(const std::string& s, E_document_format_details_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[16].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_document_format_details_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_document_format_varying_attributes* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[17].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_document_format_varying_attributes>(val);
  return true;
}
bool FromString(const std::string& s, E_feed_orientation* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[18].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_feed_orientation>(val);
  return true;
}
bool FromString(const std::string& s, E_finishing_template* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[19].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_finishing_template>(val);
  return true;
}
bool FromString(const std::string& s, E_finishings* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[20].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_finishings>(val);
  return true;
}
bool FromString(const std::string& s, E_folding_direction* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[21].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_folding_direction>(val);
  return true;
}
bool FromString(const std::string& s, E_identify_actions* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[22].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_identify_actions>(val);
  return true;
}
bool FromString(const std::string& s, E_imposition_template* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[23].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_imposition_template>(val);
  return true;
}
bool FromString(const std::string& s, E_input_orientation_requested* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[24].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_input_orientation_requested>(val);
  return true;
}
bool FromString(const std::string& s, E_input_quality* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[25].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_input_quality>(val);
  return true;
}
bool FromString(const std::string& s, E_input_sides* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[26].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_input_sides>(val);
  return true;
}
bool FromString(const std::string& s, E_ipp_features_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[27].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_ipp_features_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_ipp_versions_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[28].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_ipp_versions_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_job_account_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[29].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_account_type>(val);
  return true;
}
bool FromString(const std::string& s, E_job_accounting_output_bin* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[30].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_accounting_output_bin>(val);
  return true;
}
bool FromString(const std::string& s, E_job_accounting_sheets_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[31].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_accounting_sheets_type>(val);
  return true;
}
bool FromString(const std::string& s, E_job_collation_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[32].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_collation_type>(val);
  return true;
}
bool FromString(const std::string& s, E_job_delay_output_until* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[33].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_delay_output_until>(val);
  return true;
}
bool FromString(const std::string& s, E_job_error_action* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[34].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_error_action>(val);
  return true;
}
bool FromString(const std::string& s, E_job_error_sheet_when* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[35].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_error_sheet_when>(val);
  return true;
}
bool FromString(const std::string& s, E_job_hold_until* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[36].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_hold_until>(val);
  return true;
}
bool FromString(const std::string& s, E_job_mandatory_attributes* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[37].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_mandatory_attributes>(val);
  return true;
}
bool FromString(const std::string& s, E_job_password_encryption* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[38].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_password_encryption>(val);
  return true;
}
bool FromString(const std::string& s, E_job_sheets* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[39].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_sheets>(val);
  return true;
}
bool FromString(const std::string& s, E_job_spooling_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[40].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_spooling_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_job_state* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[41].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_state>(val);
  return true;
}
bool FromString(const std::string& s, E_job_state_reasons* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[42].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_job_state_reasons>(val);
  return true;
}
bool FromString(const std::string& s, E_laminating_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[43].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_laminating_type>(val);
  return true;
}
bool FromString(const std::string& s, E_material_color* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[44].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_material_color>(val);
  return true;
}
bool FromString(const std::string& s, E_media* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[45].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media>(val);
  return true;
}
bool FromString(const std::string& s, E_media_back_coating* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[46].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_back_coating>(val);
  return true;
}
bool FromString(const std::string& s, E_media_grain* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[47].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_grain>(val);
  return true;
}
bool FromString(const std::string& s, E_media_input_tray_check* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[48].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_input_tray_check>(val);
  return true;
}
bool FromString(const std::string& s, E_media_key* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[49].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_key>(val);
  return true;
}
bool FromString(const std::string& s, E_media_pre_printed* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[50].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_pre_printed>(val);
  return true;
}
bool FromString(const std::string& s, E_media_ready* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[51].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_ready>(val);
  return true;
}
bool FromString(const std::string& s, E_media_source* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[52].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_source>(val);
  return true;
}
bool FromString(const std::string& s, E_media_tooth* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[53].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_tooth>(val);
  return true;
}
bool FromString(const std::string& s, E_media_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[54].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_media_type>(val);
  return true;
}
bool FromString(const std::string& s, E_multiple_document_handling* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[55].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_multiple_document_handling>(val);
  return true;
}
bool FromString(const std::string& s, E_multiple_operation_time_out_action* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[56].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_multiple_operation_time_out_action>(val);
  return true;
}
bool FromString(const std::string& s, E_notify_events* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[57].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_notify_events>(val);
  return true;
}
bool FromString(const std::string& s, E_notify_pull_method* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[58].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_notify_pull_method>(val);
  return true;
}
bool FromString(const std::string& s, E_operations_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[59].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_operations_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_page_delivery* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[60].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_page_delivery>(val);
  return true;
}
bool FromString(const std::string& s, E_pdf_versions_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[61].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_pdf_versions_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_pdl_init_file_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[62].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_pdl_init_file_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_pdl_override_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[63].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_pdl_override_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_presentation_direction_number_up* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[64].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_presentation_direction_number_up>(val);
  return true;
}
bool FromString(const std::string& s, E_print_color_mode* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[65].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_print_color_mode>(val);
  return true;
}
bool FromString(const std::string& s, E_print_content_optimize* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[66].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_print_content_optimize>(val);
  return true;
}
bool FromString(const std::string& s, E_print_rendering_intent* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[67].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_print_rendering_intent>(val);
  return true;
}
bool FromString(const std::string& s, E_print_scaling* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[68].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_print_scaling>(val);
  return true;
}
bool FromString(const std::string& s, E_printer_state* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[69].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_printer_state>(val);
  return true;
}
bool FromString(const std::string& s, E_printer_state_reasons* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[70].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_printer_state_reasons>(val);
  return true;
}
bool FromString(const std::string& s, E_proof_print_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[71].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_proof_print_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_pwg_raster_document_sheet_back* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[72].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_pwg_raster_document_sheet_back>(val);
  return true;
}
bool FromString(const std::string& s, E_pwg_raster_document_type_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[73].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_pwg_raster_document_type_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_requested_attributes* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[74].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_requested_attributes>(val);
  return true;
}
bool FromString(const std::string& s, E_save_disposition* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[75].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_save_disposition>(val);
  return true;
}
bool FromString(const std::string& s, E_separator_sheets_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[76].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_separator_sheets_type>(val);
  return true;
}
bool FromString(const std::string& s, E_sheet_collate* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[77].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_sheet_collate>(val);
  return true;
}
bool FromString(const std::string& s, E_status_code* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[78].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_status_code>(val);
  return true;
}
bool FromString(const std::string& s, E_stitching_method* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[79].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_stitching_method>(val);
  return true;
}
bool FromString(const std::string& s, E_stitching_reference_edge* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[80].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_stitching_reference_edge>(val);
  return true;
}
bool FromString(const std::string& s, E_trimming_type* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[81].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_trimming_type>(val);
  return true;
}
bool FromString(const std::string& s, E_trimming_when* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[82].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_trimming_when>(val);
  return true;
}
bool FromString(const std::string& s, E_uri_authentication_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[83].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_uri_authentication_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_uri_security_supported* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[84].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_uri_security_supported>(val);
  return true;
}
bool FromString(const std::string& s, E_which_jobs* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[85].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_which_jobs>(val);
  return true;
}
bool FromString(const std::string& s, E_x_image_position* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[86].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_x_image_position>(val);
  return true;
}
bool FromString(const std::string& s, E_xri_authentication* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[87].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_xri_authentication>(val);
  return true;
}
bool FromString(const std::string& s, E_xri_security* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[88].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_xri_security>(val);
  return true;
}
bool FromString(const std::string& s, E_y_image_position* v) {
  if (v == nullptr)
    return false;
  uint16_t val;
  if (!(kAllEnums[89].val(s.c_str(), &val)))
    return false;
  *v = static_cast<E_y_image_position>(val);
  return true;
}
}  // namespace ipp

// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_MOJO_SERVICE_CONSTANTS_H_
#define SYSTEM_API_MOJO_SERVICE_CONSTANTS_H_

namespace chromeos::mojo_services {

// Please keep alphabetized.
constexpr char kChromiumCrosHealthdDataCollector[] =
    "ChromiumCrosHealthdDataCollector";
constexpr char kChromiumNetworkDiagnosticsRoutines[] =
    "ChromiumNetworkDiagnosticsRoutines";
constexpr char kChromiumNetworkHealth[] = "ChromiumNetworkHealth";
constexpr char kCrosDcadService[] = "CrosDcadService";
constexpr char kCrosHealthdAshEventReporter[] = "CrosHealthdAshEventReporter";
constexpr char kCrosHealthdDiagnostics[] = "CrosHealthdDiagnostics";
constexpr char kCrosHealthdEvent[] = "CrosHealthdEvent";
constexpr char kCrosHealthdProbe[] = "CrosHealthdProbe";
constexpr char kCrosHealthdRoutines[] = "CrosHealthdRoutines";
constexpr char kCrosHealthdSystem[] = "CrosHealthdSystem";
constexpr char kIioSensor[] = "IioSensor";

}  // namespace chromeos::mojo_services

#endif  // SYSTEM_API_MOJO_SERVICE_CONSTANTS_H_

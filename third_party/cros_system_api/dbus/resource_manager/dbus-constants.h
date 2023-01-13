// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_RESOURCE_MANAGER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_RESOURCE_MANAGER_DBUS_CONSTANTS_H_

namespace resource_manager {

const char kResourceManagerInterface[] = "org.chromium.ResourceManager";
const char kResourceManagerServicePath[] = "/org/chromium/ResourceManager";
const char kResourceManagerServiceName[] = "org.chromium.ResourceManager";

// Values.
enum GameMode {
  // Game mode is off.
  OFF = 0,
  // Game mode is on, borealis is the foreground subsystem.
  BOREALIS = 1,
};

enum PressureLevelChrome {
  // There is enough memory to use.
  NONE = 0,
  // Chrome is advised to free buffers that are cheap to re-allocate and not
  // immediately needed.
  MODERATE = 1,
  // Chrome is advised to free all possible memory.
  CRITICAL = 2,
};

enum class PressureLevelArcvm {
  // There is enough memory to use.
  NONE = 0,
  // ARCVM is advised to kill cached processes to free memory.
  CACHED = 1,
  // ARCVM is advised to kill perceptible processes to free memory.
  PERCEPTIBLE = 2,
  // ARCVM is advised to kill foreground processes to free memory.
  FOREGROUND = 3,
};

// Methods.
const char kGetAvailableMemoryKBMethod[] = "GetAvailableMemoryKB";
const char kGetForegroundAvailableMemoryKBMethod[] =
    "GetForegroundAvailableMemoryKB";
const char kGetMemoryMarginsKBMethod[] = "GetMemoryMarginsKB";
const char kGetComponentMemoryMarginsKBMethod[] = "GetComponentMemoryMarginsKB";
const char kGetGameModeMethod[] = "GetGameMode";
const char kSetGameModeMethod[] = "SetGameMode";
const char kSetGameModeWithTimeoutMethod[] = "SetGameModeWithTimeout";
const char kSetMemoryMarginsBps[] = "SetMemoryMarginsBps";
const char kSetFullscreenVideoWithTimeout[] = "SetFullscreenVideoWithTimeout";

// Signals.

// MemoryPressureChrome signal contains 2 arguments:
//   1. pressure_level, BYTE, see also enum PressureLevelChrome.
//   2. reclaim_target_kb, UINT64, memory amount to free in KB to leave the
//   current pressure level.
//   E.g., argument (PressureLevelChrome::CRITICAL, 10000): Chrome should free
//   10000 KB to leave the critical memory pressure level (to moderate pressure
//   level).
// TODO(vovoy): Consider serializing these parameters to protobuf.
const char kMemoryPressureChrome[] = "MemoryPressureChrome";

// MemoryPressureArcvm signal contains 2 arguments:
//   1. pressure_level, BYTE, see also enum PressureLevelArcvm.
//   2. delta, UINT64, memory amount to free in KB to leave the current
//   pressure level.
//   E.g. argument (PressureLevelArcvm::FOREGROUND, 10000): ARCVM should free
//   10000 KB to leave the foreground memory pressure level (to perceptible
//   pressure level).
const char kMemoryPressureArcvm[] = "MemoryPressureArcvm";

}  // namespace resource_manager

#endif  // SYSTEM_API_DBUS_RESOURCE_MANAGER_DBUS_CONSTANTS_H_

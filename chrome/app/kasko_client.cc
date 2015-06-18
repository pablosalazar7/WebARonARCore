// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(KASKO)

#include "chrome/app/kasko_client.h"

#include <windows.h>

#include <string>

#include "base/logging.h"
#include "base/process/process_handle.h"
#include "breakpad/src/client/windows/common/ipc_protocol.h"
#include "chrome/app/chrome_watcher_client_win.h"
#include "chrome/chrome_watcher/chrome_watcher_main_api.h"
#include "chrome/common/chrome_constants.h"
#include "components/crash/app/crash_keys_win.h"
#include "syzygy/kasko/api/client.h"

namespace {

ChromeWatcherClient* g_chrome_watcher_client = nullptr;
kasko::api::MinidumpType g_minidump_type = kasko::api::SMALL_DUMP_TYPE;

void GetKaskoCrashKeys(const kasko::api::CrashKey** crash_keys,
                       size_t* crash_key_count) {
  static_assert(
      sizeof(kasko::api::CrashKey) == sizeof(google_breakpad::CustomInfoEntry),
      "CrashKey and CustomInfoEntry structs are not compatible.");
  static_assert(offsetof(kasko::api::CrashKey, name) ==
                    offsetof(google_breakpad::CustomInfoEntry, name),
                "CrashKey and CustomInfoEntry structs are not compatible.");
  static_assert(offsetof(kasko::api::CrashKey, value) ==
                    offsetof(google_breakpad::CustomInfoEntry, value),
                "CrashKey and CustomInfoEntry structs are not compatible.");
  static_assert(
      sizeof(reinterpret_cast<kasko::api::CrashKey*>(0)->name) ==
          sizeof(reinterpret_cast<google_breakpad::CustomInfoEntry*>(0)->name),
      "CrashKey and CustomInfoEntry structs are not compatible.");
  static_assert(
      sizeof(reinterpret_cast<kasko::api::CrashKey*>(0)->value) ==
          sizeof(reinterpret_cast<google_breakpad::CustomInfoEntry*>(0)->value),
      "CrashKey and CustomInfoEntry structs are not compatible.");

  *crash_key_count =
      breakpad::CrashKeysWin::keeper()->custom_info_entries().size();
  *crash_keys = reinterpret_cast<const kasko::api::CrashKey*>(
      breakpad::CrashKeysWin::keeper()->custom_info_entries().data());
}

}  // namespace

KaskoClient::KaskoClient(ChromeWatcherClient* chrome_watcher_client,
                         kasko::api::MinidumpType minidump_type) {
  DCHECK(!g_chrome_watcher_client);
  g_minidump_type = minidump_type;
  g_chrome_watcher_client = chrome_watcher_client;

  kasko::api::InitializeClient(
      GetKaskoEndpoint(base::GetCurrentProcId()).c_str());
}

KaskoClient::~KaskoClient() {
  DCHECK(g_chrome_watcher_client);
  g_chrome_watcher_client = nullptr;
  kasko::api::ShutdownClient();
}

extern "C" void __declspec(dllexport) ReportCrashWithProtobuf(
    EXCEPTION_POINTERS* info, const char* protobuf, size_t protobuf_length) {
  if (g_chrome_watcher_client && g_chrome_watcher_client->EnsureInitialized()) {
    size_t crash_key_count = 0;
    const kasko::api::CrashKey* crash_keys = nullptr;
    GetKaskoCrashKeys(&crash_keys, &crash_key_count);
    kasko::api::SendReport(info, g_minidump_type, protobuf, protobuf_length,
                           crash_keys, crash_key_count);
  }

  // The Breakpad integration hooks TerminateProcess. Sidestep it to avoid a
  // secondary report.

  using TerminateProcessWithoutDumpProc = void(__cdecl*)();
  TerminateProcessWithoutDumpProc terminate_process_without_dump =
      reinterpret_cast<TerminateProcessWithoutDumpProc>(::GetProcAddress(
          ::GetModuleHandle(chrome::kBrowserProcessExecutableName),
          "TerminateProcessWithoutDump"));
  CHECK(terminate_process_without_dump);
  terminate_process_without_dump();
}

#endif  // defined(KASKO)

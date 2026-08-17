/*
 * OpenKneeboard
 *
 * Copyright (C) 2022-2023 Fred Emmott <fred@fredemmott.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#define VISORVR_CAPI_IMPL
#include "VisorVR_CAPI.h"

#include <VisorVR/APIEvent.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/tracing.hpp>

static void init() {
  VisorVR::DPrintSettings::Set({
    .prefix = "VisorVR-CAPI",
  });

  wchar_t buf[1024];
  const auto length = GetModuleFileNameW(NULL, buf, std::size(buf));
  if (length) {
    VisorVR::dprint(
      L"new API client: {}", std::wstring_view {buf, length});
  } else {
    VisorVR::dprint(
      "new API client - failed to get client path: {:#018x}",
      static_cast<uint64_t>(GetLastError()));
  }
}

VISORVR_CAPI void VisorVR_send_utf8(
  const char* eventName,
  size_t eventNameByteCount,
  const char* eventValue,
  size_t eventValueByteCount) {
  const VisorVR::APIEvent ge {
    {eventName, eventNameByteCount},
    {eventValue, eventValueByteCount},
  };
  ge.Send();
}

VISORVR_CAPI void VisorVR_send_wchar_ptr(
  const wchar_t* eventName,
  size_t eventNameCharCount,
  const wchar_t* eventValue,
  size_t eventValueCharCount) {
  const VisorVR::APIEvent ge {
    winrt::to_string(std::wstring_view {eventName, eventNameCharCount}),
    winrt::to_string(std::wstring_view {eventValue, eventValueCharCount}),
  };
  ge.Send();
}

namespace VisorVR {

/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("VisorVR.API.C")
 * cfaa744f-ba6f-5e56-5c91-88de46269c4b
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "VisorVR.API.C",
  (0xcfaa744f, 0xba6f, 0x5e56, 0x5c, 0x91, 0x88, 0xde, 0x46, 0x26, 0x9c, 0x4b));
}// namespace VisorVR

BOOL WINAPI DllMain(HINSTANCE, const DWORD dwReason, LPVOID /* lpReserved */) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      TraceLoggingRegister(VisorVR::gTraceProvider);
      TraceLoggingWrite(
        VisorVR::gTraceProvider,
        "Attached",
        TraceLoggingThisExecutable());
      init();
      break;
    case DLL_PROCESS_DETACH:
      TraceLoggingWrite(
        VisorVR::gTraceProvider,
        "Detached",
        TraceLoggingThisExecutable());
      TraceLoggingUnregister(VisorVR::gTraceProvider);
      break;
  }
  return TRUE;
}

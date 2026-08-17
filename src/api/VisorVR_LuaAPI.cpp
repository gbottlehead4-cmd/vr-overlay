// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/APIEvent.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/tracing.hpp>

#include <shims/winrt/base.h>

#include <Windows.h>

#include <cinttypes>
#include <cstdlib>
#include <format>
#include <string>

extern "C" {
#include <lauxlib.h>
}

using VisorVR::dprint;

namespace VisorVR {

/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("VisorVR.API.Lua")
 * 039d7b52-2065-5863-802b-873c638bdf88
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "VisorVR.API.Lua",
  (0x039d7b52, 0x2065, 0x5863, 0x80, 0x2b, 0x87, 0x3c, 0x63, 0x8b, 0xdf, 0x88));
}// namespace VisorVR

static void push_arg_error(lua_State* state) {
  lua_pushliteral(state, "2 string arguments are required\n");
  lua_error(state);
}

static int SendToVisorVR(lua_State* state) {
  VISORVR_TraceLoggingScopedActivity(activity, "SendToVisorVR");
  int argc = lua_gettop(state);
  if (argc != 2) {
    dprint("Invalid argument count\n");
    push_arg_error(state);
    activity.StopWithResult("InvalidArgs");
    return 1;
  }

  if (!(lua_isstring(state, 1) && lua_isstring(state, 2))) {
    dprint("Non-string args\n");
    push_arg_error(state);
    activity.StopWithResult("NonStringArgs");
    return 1;
  }

  const VisorVR::APIEvent event {
    lua_tostring(state, 1),
    lua_tostring(state, 2),
  };
  event.Send();

  return 0;
}

extern "C" int __declspec(dllexport)
#if UINTPTR_MAX == UINT64_MAX
luaopen_VisorVR_LuaAPI64(lua_State* state) {
#elif UINTPTR_MAX == UINT32_MAX
luaopen_VisorVR_LuaAPI32(lua_State* state) {
#endif
  VISORVR_TraceLoggingScope("luaopen_VisorVR_LuaAPI64");
  VisorVR::DPrintSettings::Set({
    .prefix = "VisorVR-LuaAPI",
  });
  lua_createtable(state, 0, 1);
  lua_pushcfunction(state, &SendToVisorVR);
  lua_setfield(state, -2, "sendRaw");
  return 1;
}

BOOL WINAPI DllMain(HINSTANCE, const DWORD dwReason, LPVOID /* lpReserved */) {
  const auto& provider = VisorVR::gTraceProvider;
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      TraceLoggingRegister(provider);
      TraceLoggingWrite(provider, "Attached", TraceLoggingThisExecutable());
      break;
    case DLL_PROCESS_DETACH:
      TraceLoggingWrite(provider, "Detached", TraceLoggingThisExecutable());
      TraceLoggingUnregister(provider);
      break;
  }
  return TRUE;
}

// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/Win32.hpp>

#include <VisorVR/config.hpp>
#include <VisorVR/dprint.hpp>
#include <VisorVR/scope_exit.hpp>

#include <shims/winrt/base.h>

#include <Windows.h>

#include <filesystem>
#include <iostream>
#include <optional>

namespace VisorVR {

namespace {

auto& GetHistoryProvider() {
  static DebugPrinter::HistoryProvider sProvider;
  return sProvider;
}

auto& GetSettings() {
  static DPrintSettings sSettings;
  return sSettings;
}

auto& GetWidePrefix() {
  static std::wstring sPrefix;
  return sPrefix;
}

}// namespace

void DebugPrinter::SetHistoryProvider(const HistoryProvider& provider) {
  GetHistoryProvider() = provider;
}

std::optional<std::string> DebugPrinter::MaybeGetHistory() {
  if (const auto& provider = GetHistoryProvider()) {
    return provider();
  }
  return std::nullopt;
}

static std::wstring GetDPrintResourceName(std::wstring_view key) {
  // v2: explicit size added
  // v3: compatibility for 32-bit sender and 64-bit receiver
  return std::format(
    L"{}.dprint.v3.{}", VisorVR::ProjectReverseDomainW, key);
}

#define IPC_RESOURCE_NAME_FUNC(resource) \
  static std::wstring_view GetDPrint##resource##Name() { \
    static std::wstring sCache; \
    if (!sCache.empty()) { \
      return sCache; \
    } \
    sCache = GetDPrintResourceName(L"" #resource); \
    return sCache; \
  }
IPC_RESOURCE_NAME_FUNC(BufferReadyEvent)
IPC_RESOURCE_NAME_FUNC(DataReadyEvent)
IPC_RESOURCE_NAME_FUNC(Mapping)
IPC_RESOURCE_NAME_FUNC(Mutex)
#undef IPC_RESOURCE_NAME_FUNC

static DPrintMessageHeader gIPCMessageHeader;

static void WriteIPCMessage(std::wstring_view message) {
  {
    winrt::handle mutex {
      OpenMutexW(SYNCHRONIZE, false, GetDPrintMutexName().data())};
    if (!mutex) {
      return;
    }
  }

  winrt::handle bufferReadyEvent {
    OpenEventW(SYNCHRONIZE, false, GetDPrintBufferReadyEventName().data())};
  if (!bufferReadyEvent) {
    VISORVR_BREAK;
    return;
  }
  winrt::handle dataReadyEvent {OpenEventW(
    SYNCHRONIZE | EVENT_MODIFY_STATE,
    false,
    GetDPrintDataReadyEventName().data())};
  if (!dataReadyEvent) {
    VISORVR_BREAK;
    return;
  }

  if (WaitForSingleObject(bufferReadyEvent.get(), 100) == WAIT_TIMEOUT) {
    return;
  }

  auto mapping = Win32::or_default::CreateFileMapping(
    INVALID_HANDLE_VALUE,
    nullptr,
    PAGE_READWRITE,
    0,
    sizeof(DPrintMessage),
    GetDPrintMappingName().data());
  if (!mapping) {
    VISORVR_BREAK;
    return;
  }

  void* shm =
    MapViewOfFile(mapping.get(), FILE_MAP_WRITE, 0, 0, sizeof(DPrintMessage));
  if (!shm) {
    VISORVR_BREAK;
    return;
  }

  memset(shm, 0, sizeof(DPrintMessage));

  auto messageSHM = reinterpret_cast<DPrintMessage*>(shm);
  messageSHM->mHeader = gIPCMessageHeader;
  memcpy(
    messageSHM->mMessage, message.data(), message.size() * sizeof(message[0]));
  messageSHM->mMessageLength = message.size();

  FlushViewOfFile(shm, sizeof(DPrintMessage));
  UnmapViewOfFile(shm);
  mapping = {};
  SetEvent(dataReadyEvent.get());
}

// Only used in `if constexpr (Config::IsDebugBuild)`
[[maybe_unused]] static bool IsDebugStreamEnabledInRegistry() {
  static std::optional<bool> sCache;
  if (sCache) {
    return *sCache;
  }
  DWORD value = 0;
  for (auto hkey: {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE}) {
    DWORD size = sizeof(value);
    if (
      RegGetValueW(
        hkey,
        RegistrySubKey,
        L"AlwaysWriteToDebugStream",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &size)
      == ERROR_SUCCESS) {
      break;
    }
  }
  *sCache = (value != 0);
  return *sCache;
}

static bool IsDebugStreamEnabled() {
  if constexpr (Config::IsDebugBuild) {
    return true;
  } else {
    return IsDebugStreamEnabledInRegistry() || IsDebuggerPresent();
  }
}

static inline bool IsConsoleOutputEnabled() {
  return GetSettings().consoleOutput
    == DPrintSettings::ConsoleOutputMode::ALWAYS;
}

void DebugPrinter::Write(std::string_view message) {
  Write(winrt::to_hstring(message));
}

void DebugPrinter::Write(std::wstring_view message) {
  TraceLoggingWrite(
    gTraceProvider,
    "dprint",
    TraceLoggingKeyword(std::to_underlying(TraceLoggingEventKeywords::DPrint)),
    TraceLoggingCountedWideString(message.data(), message.size(), "Message"));
  if (IsDebugStreamEnabled()) {
    auto output = std::format(L"[{}] {}\n", GetWidePrefix(), message);
    OutputDebugStringW(output.c_str());
  }

  if (IsConsoleOutputEnabled()) {
    static bool sFirst = true;
    if (sFirst) {
      sFirst = false;
      if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
      }
      FILE* inStream {};
      FILE* outStream {};
      FILE* errStream {};
      freopen_s(&inStream, "CONIN$", "r", stdin);
      freopen_s(&outStream, "CONOUT$", "w", stdout);
      freopen_s(&errStream, "CONOUT$", "w", stderr);
    }

    std::wcout << message << std::endl;
  }

  WriteIPCMessage(message);
}

void DPrintSettings::Set(const DPrintSettings& settings) {
  GetSettings() = settings;
  GetWidePrefix() = std::wstring(winrt::to_hstring(settings.prefix));

  memset(&gIPCMessageHeader, 0, sizeof(gIPCMessageHeader));
  gIPCMessageHeader.mProcessID = GetCurrentProcessId(),
  memcpy(
    gIPCMessageHeader.mPrefix,
    GetWidePrefix().c_str(),
    GetWidePrefix().size() * sizeof(wchar_t));
  GetModuleFileNameW(
    NULL,
    gIPCMessageHeader.mExecutable,
    sizeof(gIPCMessageHeader.mExecutable) / sizeof(wchar_t));
}

DPrintReceiver::DPrintReceiver() {
  scope_exit cleanup([this]() {
    if (mUsable) {
      return;
    }
    mMutex = {};
    mMapping = {};
    mDataReadyEvent = {};
    mBufferReadyEvent = {};
  });

  mMutex =
    Win32::or_default::CreateMutex(nullptr, true, GetDPrintMutexName().data());
  if (mMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    VISORVR_BREAK;
    return;
  }
  if (!mMutex) {
    VISORVR_BREAK;
    return;
  }

  mMapping = Win32::or_default::CreateFileMapping(
    INVALID_HANDLE_VALUE,
    nullptr,
    PAGE_READWRITE,
    0,
    sizeof(DPrintMessage),
    GetDPrintMappingName().data());
  if (!mMapping) {
    VISORVR_BREAK;
    return;
  }
  mBufferReadyEvent = Win32::or_default::CreateEvent(
    nullptr, false, false, GetDPrintBufferReadyEventName().data());
  if (!mBufferReadyEvent) {
    VISORVR_BREAK;
    return;
  }

  mDataReadyEvent = Win32::or_default::CreateEvent(
    nullptr, false, false, GetDPrintDataReadyEventName().data());
  if (!mDataReadyEvent) {
    VISORVR_BREAK;
    return;
  }

  mSHM = reinterpret_cast<DPrintMessage*>(MapViewOfFile(
    mMapping.get(),
    FILE_MAP_READ | FILE_MAP_WRITE,
    0,
    0,
    sizeof(DPrintMessage)));
  if (!mSHM) {
    VISORVR_BREAK;
    return;
  }

  mUsable = true;
}

DPrintReceiver::~DPrintReceiver() {
  if (mSHM) {
    UnmapViewOfFile(mSHM);
  }
}

bool DPrintReceiver::IsUsable() const { return mUsable; }

void DPrintReceiver::Run(std::stop_token stopToken) {
  if (!this->IsUsable()) {
    return;
  }

  const auto stopEvent =
    Win32::or_default::CreateEvent(nullptr, false, false, nullptr);
  std::stop_callback stopCallback(
    stopToken, [&]() { SetEvent(stopEvent.get()); });

  HANDLE handles[] = {
    mDataReadyEvent.get(),
    stopEvent.get(),
  };

  while (true) {
    // Unset canary
    memset(mSHM, 0xcf, sizeof(DPrintMessage));

    ResetEvent(mDataReadyEvent.get());
    SetEvent(mBufferReadyEvent.get());

    const auto result = WaitForMultipleObjects(
      sizeof(handles) / sizeof(handles[0]),
      handles,
      /* all = */ false,
      /* ms = */ INFINITE);

    if (stopToken.stop_requested()) {
      ResetEvent(mBufferReadyEvent.get());
      return;
    }

    if (result != WAIT_OBJECT_0) {
      VISORVR_BREAK;
    }

    this->OnMessage(*mSHM);
  }
}

}// namespace VisorVR

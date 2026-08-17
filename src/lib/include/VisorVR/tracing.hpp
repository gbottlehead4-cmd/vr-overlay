// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/macros.hpp>
#include <VisorVR/scope_exit.hpp>

#include <Windows.h>
#include <winmeta.h>

#include <exception>
#include <source_location>

#include <TraceLoggingActivity.h>
#include <TraceLoggingProvider.h>

namespace VisorVR {

/// Bitmask; we can use up to 48 bits here. Upper 16 bits are reserved for
/// Microsoft
enum class TraceLoggingEventKeywords {
  Uncategorized = 1,
  DPrint = 2,
  Events = 4,
  TaskCoro = 8,
};

wchar_t* GetFullPathForCurrentExecutable();

#define TraceLoggingThisExecutable() \
  TraceLoggingValue( \
    ::VisorVR::GetFullPathForCurrentExecutable(), "Executable")

TRACELOGGING_DECLARE_PROVIDER(gTraceProvider);

#define VISORVR_TraceLoggingSourceLocation(loc) \
  TraceLoggingValue(loc.file_name(), "File"), \
    TraceLoggingValue(loc.line(), "Line"), \
    TraceLoggingValue(loc.function_name(), "Function")

#define VISORVR_TraceLoggingSize2D(size2d, name) \
  TraceLoggingValue(size2d.Width(), name "/Width"), \
    TraceLoggingValue(size2d.Height(), name "/Height")

#define VISORVR_TraceLoggingRect(pr, name) \
  TraceLoggingValue(pr.Left(), name "/Left"), \
    TraceLoggingValue(pr.Top(), name "/Top"), \
    VISORVR_TraceLoggingSize2D(pr, name)

using UncategorizedTraceLoggingThreadActivity = TraceLoggingThreadActivity<
  gTraceProvider,
  std::to_underlying(TraceLoggingEventKeywords::Uncategorized),
  WINEVENT_LEVEL_INFO>;

/** Create and automatically start and stop a named activity.
 *
 * @param VVRTL_ACTIVITY the local variable to store the activity in
 * @param VVRTL_NAME the name of the activity (C string literal)
 *
 * @see VISORVR_TraceLoggingScope if you don't need the local variable
 *
 * This avoids templates and `auto` and generally jumps through hoops so that it
 * is valid both inside an implementation, and in a class definition.
 */
#define VISORVR_TraceLoggingScopedActivity( \
  VVRTL_ACTIVITY, VVRTL_NAME, ...) \
  const std::function<void( \
    VisorVR::UncategorizedTraceLoggingThreadActivity&)> \
    VISORVR_CONCAT2(_StartImpl, VVRTL_ACTIVITY) = \
      [&, loc = std::source_location::current()]( \
        VisorVR::UncategorizedTraceLoggingThreadActivity& activity) { \
        TraceLoggingWriteStart( \
          activity, \
          VVRTL_NAME, \
          VISORVR_TraceLoggingSourceLocation(loc), \
          ##__VA_ARGS__); \
      }; \
  class VISORVR_CONCAT2(_Impl, VVRTL_ACTIVITY) final \
    : public VisorVR::UncategorizedTraceLoggingThreadActivity { \
   public: \
    VISORVR_CONCAT2(_Impl, VVRTL_ACTIVITY) \
    (decltype(VISORVR_CONCAT2(_StartImpl, VVRTL_ACTIVITY))& startImpl) { \
      startImpl(*this); \
    } \
    VISORVR_CONCAT2(~_Impl, VVRTL_ACTIVITY)() { \
      if (mAutoStop) { \
        this->Stop(); \
      } \
    } \
    void Stop() { \
      if (mStopped) [[unlikely]] { \
        OutputDebugStringW(L"Double-stopped in Stop()"); \
        VISORVR_BREAK; \
        return; \
      } \
      mStopped = true; \
      mAutoStop = false; \
      const auto exceptionCount = std::uncaught_exceptions(); \
      if (exceptionCount) [[unlikely]] { \
        TraceLoggingWriteStop( \
          *this, \
          VVRTL_NAME, \
          TraceLoggingValue(exceptionCount, "UncaughtExceptions")); \
      } else { \
        TraceLoggingWriteStop(*this, VVRTL_NAME); \
      } \
    } \
    void CancelAutoStop() { mAutoStop = false; } \
    _VISORVR_TRACELOGGING_IMPL_StopWithResult(VVRTL_NAME, int); \
    _VISORVR_TRACELOGGING_IMPL_StopWithResult(VVRTL_NAME, const char*); \
\
   private: \
    bool mStopped {false}; \
    bool mAutoStop {true}; \
  }; \
  VISORVR_CONCAT2(_Impl, VVRTL_ACTIVITY) \
  VVRTL_ACTIVITY {VISORVR_CONCAT2(_StartImpl, VVRTL_ACTIVITY)};

// Not using templates as they're not permitted in local classes
#define _VISORVR_TRACELOGGING_IMPL_StopWithResult( \
  VVRTL_NAME, VVRTL_RESULT_TYPE) \
  void StopWithResult(VVRTL_RESULT_TYPE result) { \
    if (mStopped) [[unlikely]] { \
      OutputDebugStringW(L"Double-stopped in StopWithResult()"); \
      VISORVR_BREAK; \
      return; \
    } \
    this->CancelAutoStop(); \
    mStopped = true; \
    TraceLoggingWriteStop( \
      *this, VVRTL_NAME, TraceLoggingValue(result, "Result")); \
  }

/** Create and automatically start and stop a named activity.
 *
 * Convenience wrapper around VISORVR_TraceLoggingScopedActivity
 * that generates the local variable names.
 *
 * @param VVRTL_NAME the name of the activity (C string literal)
 */
#define VISORVR_TraceLoggingScope(VVRTL_NAME, ...) \
  VISORVR_TraceLoggingScopedActivity( \
    VISORVR_CONCAT2(_vvrtlsa, __COUNTER__), VVRTL_NAME, ##__VA_ARGS__)

#define VISORVR_TraceLoggingWrite(VVRTL_NAME, ...) \
  TraceLoggingWrite( \
    gTraceProvider, \
    VVRTL_NAME, \
    TraceLoggingLevel(WINEVENT_LEVEL_INFO), \
    TraceLoggingKeyword( \
      std::to_underlying(TraceLoggingEventKeywords::Uncategorized)), \
    TraceLoggingValue(__FILE__, "File"), \
    TraceLoggingValue(__LINE__, "Line"), \
    TraceLoggingValue(__FUNCTION__, "Function"), \
    ##__VA_ARGS__)

#define VISORVR_TraceLoggingCoro(VVRTL_NAME, ...) \
  VISORVR_TraceLoggingWrite( \
    VVRTL_NAME, TraceLoggingOpcode(WINEVENT_OPCODE_START), ##__VA_ARGS__); \
  const ::VisorVR::scope_exit VISORVR_CONCAT2( \
    _vvrtllcoro__, __COUNTER__) {[&, n = std::uncaught_exceptions()]() { \
    VISORVR_TraceLoggingWrite( \
      VVRTL_NAME, \
      TraceLoggingOpcode(WINEVENT_OPCODE_STOP), \
      TraceLoggingValue(std::uncaught_exceptions() - n, "Exceptions"), \
      ##__VA_ARGS__); \
  }};

#define VISORVR_TraceLoggingStringView(value, name) \
  TraceLoggingCountedUtf8String(value.data(), value.size(), name)

}// namespace VisorVR

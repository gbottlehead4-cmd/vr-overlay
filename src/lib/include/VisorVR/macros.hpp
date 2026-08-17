// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

// Helper macros when joining or stringifying other macros, e.g.:
//
// `FOO##__COUNTER__` becomes `FOO__COUNTER__`
// VISORVR_CONCAT2(FOO, __COUNTER__) might become `FOO1`, for example

#define VISORVR_CONCAT1(x, y) x##y
#define VISORVR_CONCAT2(x, y) VISORVR_CONCAT1(x, y)

#define VISORVR_STRINGIFY1(x) #x
#define VISORVR_STRINGIFY2(x) VISORVR_STRINGIFY1(x)

// Helper for testing __VA_ARG__ behavior
#define VISORVR_THIRD_ARG(a, b, c, ...) c

#define VISORVR_VA_OPT_SUPPORTED_IMPL(...) \
  VISORVR_THIRD_ARG(__VA_OPT__(, ), true, false, __VA_ARGS__)
#define VISORVR_VA_OPT_SUPPORTED VISORVR_VA_OPT_SUPPORTED_IMPL(JUNK)

#define VISORVR_HAVE_NONSTANDARD_VA_ARGS_COMMA_ELISION_HELPER(X, ...) \
  X##__VA_ARGS__
#define VISORVR_HAVE_NONSTANDARD_VA_ARGS_COMMA_ELISION \
  VISORVR_THIRD_ARG( \
    VISORVR_HAVE_NONSTANDARD_VA_ARGS_COMMA_ELISION_HELPER(JUNK), \
    false, \
    true)

#if VISORVR_HAVE_NONSTANDARD_VA_ARGS_COMMA_ELISION
static_assert(
  VISORVR_HAVE_NONSTANDARD_VA_ARGS_COMMA_ELISION_HELPER(123) == 123);
#endif

// https://developercommunity.visualstudio.com/t/std::unreachable-causes-warning-4702-un/10720390
#if defined(_MSC_VER) && !defined(__clang__)
#define VISORVR_UNREACHABLE __assume(0)
#else
#define VISORVR_UNREACHABLE std::unreachable();
#endif

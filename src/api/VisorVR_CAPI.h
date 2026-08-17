/*
 * OpenKneeboard API - ISC License
 *
 * Copyright (C) 2022-2023 Fred Emmott <fred@fredemmott.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ----------------------------------------------------------------
 * ----- THE ABOVE LICENSE ONLY APPLIES TO THIS SPECIFIC FILE -----
 * ----------------------------------------------------------------
 *
 * The majority of VisorVR is under a different license; check specific
 * files for more information.
 */

#ifndef VISORVR_CAPI_H
#define VISORVR_CAPI_H 1

#ifdef __cplusplus
#include <cinttypes>
#ifdef VISORVR_CAPI_IMPL
#define VISORVR_CAPI extern "C" __declspec(dllexport)
#else
#define VISORVR_CAPI extern "C" __declspec(dllimport)
#endif
#else
#include <inttypes.h>
#define VISORVR_CAPI __declspec(dllimport)
#endif

VISORVR_CAPI void VisorVR_send_utf8(
  const char* messageName,
  size_t messageNameByteCount,
  const char* messageValue,
  size_t messageValueByteCount);

VISORVR_CAPI void VisorVR_send_wchar_ptr(
  const wchar_t* messageName,
  size_t messageNameCharCount,
  const wchar_t* messageValue,
  size_t messageValueCharCount);

#if UINTPTR_MAX == UINT64_MAX
#define VISORVR_CAPI_DLL_NAME_A "VisorVR_CAPI64.dll"
#define VISORVR_CAPI_DLL_NAME_W L"VisorVR_CAPI64.dll"
#elif UINTPTR_MAX == UINT32_MAX
#define VISORVR_CAPI_DLL_NAME_A "VisorVR_CAPI32.dll"
#define VISORVR_CAPI_DLL_NAME_W L"VisorVR_CAPI32.dll"
#endif

#endif

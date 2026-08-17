// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <Windows.h>

namespace VisorVR {
[[nodiscard]]
int ChromiumWorkerMain(HINSTANCE instance, void* sandbox);

}

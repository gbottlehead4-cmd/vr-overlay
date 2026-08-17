// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <filesystem>

namespace VisorVR::OpenXRLayers {

/** Which registry hive an implicit OpenXR layer is registered in.
 *
 * `AllUsers` (HKLM) needs administrator rights, so it is only reachable via
 * the elevated helper process. `CurrentUser` (HKCU) does not, which is what
 * makes a copy-and-run build possible.
 */
enum class Scope {
  CurrentUser,
  AllUsers,
};

/** Which registry view to use.
 *
 * Only meaningful for `Scope::AllUsers`: `HKLM\Software` is WOW64-redirected,
 * so 32- and 64-bit processes see separate keys. `HKCU\Software` is *not*
 * redirected, so both bitnesses share one key - see RegisterForCurrentUser().
 */
enum class Bitness {
  x64,
  x86,
};

/// Is this exact layer JSON registered and not disabled?
[[nodiscard]]
bool IsEnabled(Scope, Bitness, const std::filesystem::path& jsonPath);

/** Register this layer JSON, and disable any other entry with the same file
 * name within the same scope - i.e. a copy of us at a stale path.
 */
[[nodiscard]]
bool Enable(Scope, Bitness, const std::filesystem::path& jsonPath);

/// Mark this layer JSON disabled, leaving the value in place.
[[nodiscard]]
bool Disable(Scope, Bitness, const std::filesystem::path& jsonPath);

/** Delete entries named `jsonFileName` whose file no longer exists.
 *
 * A portable build gets moved, copied and deleted; without this, every old
 * location it ever ran from would accumulate as a dead registry value.
 */
void RemoveStaleEntries(Scope, Bitness, std::wstring_view jsonFileName);

/** Make sure the 64-bit layer next to this executable is active for this user.
 *
 * Called on every launch so a folder that has been moved or copied to another
 * machine just works, with no installer and no administrator rights.
 *
 * Does nothing if an installed (HKLM) registration already points at this same
 * path, so an installed copy is left alone. Only the 64-bit layer is handled:
 * `HKCU\Software` is shared between 32- and 64-bit processes, so registering
 * the 32-bit layer here would offer it to 64-bit games as well.
 *
 * @returns whether the layer is active for this user afterwards.
 */
bool RegisterForCurrentUser();

/// Undo RegisterForCurrentUser(), for a clean uninstall.
bool UnregisterForCurrentUser();

}// namespace VisorVR::OpenXRLayers

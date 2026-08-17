// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/Filesystem.hpp>
#include <VisorVR/LazyOnceValue.hpp>
#include <VisorVR/StateMachine.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/format/filesystem.hpp>
#include <VisorVR/hresult.hpp>
#include <VisorVR/scope_exit.hpp>

#include <shims/winrt/base.h>

#include <Windows.h>
#include <ShlObj.h>

#include <wil/resource.h>

#include <format>
#include <fstream>
#include <mutex>

namespace VisorVR::Filesystem {
namespace {
using LazyPath = LazyOnceValue<std::filesystem::path>;

enum class TemporaryDirectoryState {
  Uninitialized,
  Cleaned,
  Initialized,
};

auto& GetTemporaryDirectoryState() {
  static AtomicStateMachine<
    TemporaryDirectoryState,
    TemporaryDirectoryState::Uninitialized,
    std::array {
      Transition {
        TemporaryDirectoryState::Uninitialized,
        TemporaryDirectoryState::Cleaned,
      },
      Transition {
        TemporaryDirectoryState::Cleaned,
        TemporaryDirectoryState::Initialized,
      },
    }>
    sTemporaryDirectoryState {};
  return sTemporaryDirectoryState;
}

}// namespace

std::filesystem::path GetKnownFolderPath(const _GUID& knownFolderID) {
  wil::unique_cotaskmem_ptr<wchar_t> buffer;
  winrt::check_hresult(SHGetKnownFolderPath(
    knownFolderID, KF_FLAG_CREATE, NULL, std::out_ptr(buffer)));
  return {buffer.get()};
}

static std::filesystem::path GetTemporaryDirectoryRoot() {
  static LazyPath sPath {[]() {
    wchar_t tempDirBuf[MAX_PATH];
    auto tempDirLen = GetTempPathW(MAX_PATH, tempDirBuf);
    return std::filesystem::path {std::wstring_view {tempDirBuf, tempDirLen}}
    / L"VisorVR";
  }};
  return sPath;
}

static std::filesystem::path GetTemporaryDirectoryImpl() {
  auto tempDir = GetTemporaryDirectoryRoot()
    / std::format("{:%F %H-%M-%S} {}",

                  std::chrono::floor<std::chrono::seconds>(
                    std::chrono::system_clock::now()),
                  GetCurrentProcessId());

  if (!std::filesystem::exists(tempDir)) {
    std::filesystem::create_directories(tempDir);
  }

  GetTemporaryDirectoryState()
    .Transition<
      TemporaryDirectoryState::Cleaned,
      TemporaryDirectoryState::Initialized>();

  return std::filesystem::canonical(tempDir);
}

bool IsDirectoryShortcut(const std::filesystem::path& link) noexcept {
  if (!std::filesystem::exists(link)) {
    return false;
  }

  const auto shortcut =
    winrt::create_instance<IShellLinkW>(CLSID_FolderShortcut);
  const auto persist = shortcut.as<IPersistFile>();
  return SUCCEEDED(persist->Load(link.wstring().c_str(), STGM_READ));
}

// Argument order matches std::filesystem::create_directory_symlink()
void CreateDirectoryShortcut(
  const std::filesystem::path& target,
  const std::filesystem::path& link) noexcept {
  auto shortcut = winrt::create_instance<IShellLinkW>(CLSID_FolderShortcut);
  shortcut->SetPath(target.wstring().c_str());
  shortcut->SetDescription(std::format(L"Shortcut to {}", target).c_str());

  auto persist = shortcut.as<IPersistFile>();
  winrt::check_hresult(persist->Save(link.wstring().c_str(), TRUE));
}

std::filesystem::path GetTemporaryDirectory() {
  static LazyPath sPath {[]() { return GetTemporaryDirectoryImpl(); }};
  return sPath;
}

void CleanupTemporaryDirectories() {
  GetTemporaryDirectoryState()
    .Transition<
      TemporaryDirectoryState::Uninitialized,
      TemporaryDirectoryState::Cleaned>();
  const auto root = GetTemporaryDirectoryRoot();
  dprint("Cleaning temporary directory root: {}", root);
  if (!std::filesystem::exists(root)) {
    return;
  }

  std::error_code ignored;
  for (const auto& it: std::filesystem::directory_iterator(root)) {
    std::filesystem::remove_all(it.path(), ignored);
  }

  dprint("New temporary directory: {}", Filesystem::GetTemporaryDirectory());
}

std::filesystem::path GetCurrentExecutablePath() {
  static LazyPath sPath {[]() {
    wchar_t exePathStr[MAX_PATH];
    const auto exePathStrLen = GetModuleFileNameW(NULL, exePathStr, MAX_PATH);
    return std::filesystem::canonical(
      std::wstring_view {exePathStr, exePathStrLen});
  }};
  return sPath;
}

std::filesystem::path GetRuntimeDirectory() {
  static LazyPath sPath {
    []() { return GetCurrentExecutablePath().parent_path(); }};
  return sPath;
}

std::filesystem::path GetImmutableDataDirectory() {
  static LazyPath sPath {[]() {
    return std::filesystem::canonical(GetRuntimeDirectory() / "../share");
  }};
  return sPath;
}

std::filesystem::path GetSettingsDirectory() {
  static LazyPath sPath {[]() -> std::filesystem::path {
    const auto base = GetKnownFolderPath<FOLDERID_LocalAppData>();
    if (base.empty()) {
      return {};
    }
    const auto ret = base / "VisorVR" / "Settings";
    std::filesystem::create_directories(ret);
    return ret;
  }};

  return sPath;
}

void MigrateSettingsDirectory() {
  const auto newPath = GetSettingsDirectory();
  if (!std::filesystem::exists(newPath) || !std::filesystem::is_empty(newPath)) {
    return;
  }

  // Import settings from a previous OpenKneeboard install so users keep their
  // layout. This is deliberately COPY-ONLY: the OpenKneeboard Public License
  // permits reading settings from locations carrying the original branding,
  // but does not permit modifying or extending them - so nothing here writes
  // to, renames, or deletes anything under the old paths. The names below are
  // therefore deliberately NOT rebranded.
  const auto localAppData = GetKnownFolderPath<FOLDERID_LocalAppData>();
  const std::filesystem::path candidates[] {
    localAppData / "OpenKneeboard" / "Settings",
    GetKnownFolderPath<FOLDERID_SavedGames>() / "OpenKneeboard",
  };

  for (auto&& oldPath: candidates) {
    if (oldPath.empty() || !std::filesystem::exists(oldPath)) {
      continue;
    }

    dprint("🚚 importing settings from `{}` to `{}`", oldPath, newPath);

    for (auto&& it: std::filesystem::recursive_directory_iterator(oldPath)) {
      if (it.is_directory()) {
        continue;
      }
      const auto src = it.path();
      if (src.extension() != ".json") {
        continue;
      }

      auto dest = newPath;
      for (auto&& part: std::filesystem::relative(src.parent_path(), oldPath)
             / src.filename()) {
        if (part == "profiles") {
          dest /= "Profiles";
        } else {
          dest /= part;
        }
      }

      std::filesystem::create_directories(dest.parent_path());
      std::filesystem::copy_file(
        src, dest, std::filesystem::copy_options::skip_existing);
    }

    dprint("✅ imported settings from `{}`", oldPath);
    return;
  }
}

std::filesystem::path GetLocalAppDataDirectory() {
  static LazyPath sPath {[]() -> std::filesystem::path {
    const auto base = GetKnownFolderPath<FOLDERID_LocalAppData>();
    if (base.empty()) {
      return {};
    }
    const auto ret = base / "VisorVR";
    std::filesystem::create_directories(ret);
    return ret;
  }};
  return sPath;
}

std::filesystem::path GetLogsDirectory() {
  static LazyPath sPath {[]() -> std::filesystem::path {
    const auto oldPath = GetLocalAppDataDirectory() / "Logs";
    const auto path =
      GetKnownFolderPath<FOLDERID_LocalAppData>() / "VisorVR Logs";

    if (std::filesystem::exists(oldPath) && !std::filesystem::exists(path)) {
      std::filesystem::rename(oldPath, path);
    }

    std::filesystem::create_directories(path);

    if (!Filesystem::IsDirectoryShortcut(oldPath)) {
      if (std::filesystem::exists(oldPath)) {
        std::filesystem::remove_all(oldPath);
      }
      Filesystem::CreateDirectoryShortcut(path, oldPath);
    }
    return path;
  }};
  return sPath;
}

std::filesystem::path GetCrashLogsDirectory() {
  static LazyPath sPath {[]() -> std::filesystem::path {
    const auto ret = GetLogsDirectory() / "Crashes";
    std::filesystem::create_directories(ret);
    return ret;
  }};
  return sPath;
}

std::filesystem::path GetInstalledPluginsDirectory() {
  static LazyPath sPath {[]() {
    const auto ret = GetLocalAppDataDirectory() / "Plugins" / "v1";
    std::filesystem::create_directories(ret);
    return ret;
  }};
  return sPath;
}

void OpenExplorerWithSelectedFile(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    dprint("{} - path '{}' does not exist (yet?)", __FUNCTION__, path);
    VISORVR_BREAK;
    return;
  }
  if (!std::filesystem::is_regular_file(path)) {
    dprint("{} - path '{}' is not a file", __FUNCTION__, path);
    VISORVR_BREAK;
    return;
  }
  PIDLIST_ABSOLUTE pidl {nullptr};
  check_hresult(
    SHParseDisplayName(path.wstring().c_str(), nullptr, &pidl, 0, nullptr));
  const scope_exit freePidl([pidl] { CoTaskMemFree(pidl); });
  check_hresult(SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0));
}

ScopedDeleter::ScopedDeleter(const std::filesystem::path& path) : mPath(path) {}

ScopedDeleter::~ScopedDeleter() noexcept {
  if (std::filesystem::exists(mPath)) {
    std::filesystem::remove_all(mPath);
  }
}

TemporaryCopy::TemporaryCopy(
  const std::filesystem::path& source,
  const std::filesystem::path& destination) {
  if (!std::filesystem::exists(source)) {
    throw std::logic_error("TemporaryCopy created without a source file");
  }
  if (std::filesystem::exists(destination)) {
    throw std::logic_error(
      "TemporaryCopy created, but destination already exists");
  }
  std::filesystem::copy(source, destination);
  mCopy = destination;
}

TemporaryCopy::~TemporaryCopy() noexcept { std::filesystem::remove(mCopy); }

std::filesystem::path TemporaryCopy::GetPath() const noexcept { return mCopy; }
};// namespace VisorVR::Filesystem

// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/DXResources.hpp>
#include <VisorVR/FilesystemWatcher.hpp>
#include <VisorVR/PageSourceWithDelegates.hpp>

#include <VisorVR/audited_ptr.hpp>

#include <shims/winrt/base.h>

#include <filesystem>
#include <memory>

namespace VisorVR {

class KneeboardState;

class FolderPageSource final : public PageSourceWithDelegates {
 private:
  FolderPageSource(const audited_ptr<DXResources>&, KneeboardState*);

 public:
  FolderPageSource() = delete;
  static task<std::shared_ptr<FolderPageSource>>
  Create(audited_ptr<DXResources>, KneeboardState*, std::filesystem::path = {});
  virtual ~FolderPageSource();

  std::filesystem::path GetPath() const;
  [[nodiscard]]
  task<void> SetPath(std::filesystem::path path);

  [[nodiscard]]
  task<void> Reload() noexcept;

 private:
  void SubscribeToChanges();
  VisorVR::fire_and_forget OnFileModified(std::filesystem::path);

  winrt::apartment_context mUIThread;
  std::shared_ptr<FilesystemWatcher> mWatcher;

  audited_ptr<DXResources> mDXR;
  KneeboardState* mKneeboard = nullptr;

  std::filesystem::path mPath;
  struct DelegateInfo {
    std::filesystem::file_time_type mModified;
    std::shared_ptr<IPageSource> mDelegate;
  };
  std::map<std::filesystem::path, DelegateInfo> mContents;
};

}// namespace VisorVR

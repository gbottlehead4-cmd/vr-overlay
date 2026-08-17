// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/DXResources.hpp>
#include <VisorVR/Events.hpp>
#include <VisorVR/IHasDisposeAsync.hpp>
#include <VisorVR/IPageSource.hpp>
#include <VisorVR/IPageSourceWithCursorEvents.hpp>
#include <VisorVR/IPageSourceWithDeveloperTools.hpp>
#include <VisorVR/IPageSourceWithNavigation.hpp>
#include <VisorVR/KneeboardState.hpp>
#include <VisorVR/PageSourceWithDelegates.hpp>

#include <VisorVR/audited_ptr.hpp>
#include <VisorVR/enable_shared_from_this.hpp>

#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace VisorVR {

struct DXResources;
class CachedLayer;
class DoodleRenderer;

class PageSourceWithDelegates
  : public virtual IPageSource,
    public virtual IPageSourceWithCursorEvents,
    public virtual IPageSourceWithNavigation,
    public virtual IPageSourceWithDeveloperTools,
    public IHasDisposeAsync,
    public virtual EventReceiver,
    public enable_shared_from_this<PageSourceWithDelegates> {
 public:
  PageSourceWithDelegates() = delete;
  PageSourceWithDelegates(const audited_ptr<DXResources>&, KneeboardState*);
  virtual ~PageSourceWithDelegates();

  [[nodiscard]]
  virtual task<void> DisposeAsync() noexcept override;

  virtual PageIndex GetPageCount() const override;
  virtual std::vector<PageID> GetPageIDs() const override;
  virtual std::optional<PreferredSize> GetPreferredSize(PageID) override;
  task<void> RenderPage(RenderContext, PageID, PixelRect rect) override;

  virtual void PostCursorEvent(KneeboardViewID, const CursorEvent&, PageID)
    override;
  virtual bool CanClearUserInput(PageID) const override;
  virtual bool CanClearUserInput() const override;
  virtual void ClearUserInput(PageID) override;
  virtual void ClearUserInput() override;

  virtual bool IsNavigationAvailable() const override;
  virtual std::vector<NavigationEntry> GetNavigationEntries() const override;

  [[nodiscard]]
  bool HasDeveloperTools(PageID) const override;
  fire_and_forget OpenDeveloperToolsWindow(KneeboardViewID, PageID) override;

  virtual std::optional<std::string> GetPersistentIDForPage(
    PageID) const override;
  virtual std::optional<PageID> GetPageIDFromPersistentID(
    std::string_view) const override;

 protected:
  DisposalState mDisposal;

  [[nodiscard]]
  task<void> SetDelegates(std::vector<std::shared_ptr<IPageSource>>);

 private:
  audited_ptr<DXResources> mDXResources;
  std::vector<std::shared_ptr<IPageSource>> mDelegates;
  std::vector<EventHandlerToken> mDelegateEvents;
  std::vector<EventHandlerToken> mFixedEvents;

  std::shared_ptr<IPageSource> FindDelegate(PageID) const;
  mutable std::unordered_map<PageID, std::weak_ptr<IPageSource>> mPageDelegates;

  std::unordered_map<RenderTargetID, std::unique_ptr<CachedLayer>>
    mContentLayerCache;
  std::unique_ptr<DoodleRenderer> mDoodles;

  task<void> RenderPageWithCache(
    IPageSource* delegate,
    RenderTarget*,
    PageID,
    PixelRect rect);
};

}// namespace VisorVR

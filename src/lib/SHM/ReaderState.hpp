// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/StateMachine.hpp>

#include <VisorVR/array.hpp>

namespace VisorVR::SHM {

#define VISORVR_SHM_READER_STATES \
  IT(Unlocked) \
  IT(TryLock) \
  IT(Locked)

enum class ReaderState {
#define IT(x) x,
  VISORVR_SHM_READER_STATES
#undef IT
};

using ReaderStateMachine = StateMachine<
  ReaderState,
  ReaderState::Unlocked,
  lockable_transitions<ReaderState>(),
  ReaderState::Unlocked>;

static_assert(lockable_state_machine<SHM::ReaderStateMachine>);

}// namespace VisorVR::SHM

// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/json_fwd.hpp>
#include <VisorVR/utf8.hpp>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace VisorVR {
struct APIEvent final {
  // These are both required to be UTF-8
  std::string name;
  std::string value;

  template <class T>
  T ParsedValue() const {
    if (name != T::ID) {
      throw std::logic_error("Parse type does not match event name");
    }
    return nlohmann::json::parse(this->value);
  }

  struct JSONParseError {
    std::string what;
  };

  template <class T>
  std::expected<T, JSONParseError> TryParsedValue() const {
    // Intentionally not propagating the std::logic_error
    try {
      return ParsedValue<T>();
    } catch (const nlohmann::json::exception& e) {
      return std::unexpected {JSONParseError {e.what()}};
    }
  }

  template <class T>
  static APIEvent FromStruct(const T& v) {
    nlohmann::json j;
    j = v;
    return {T::ID, j.dump()};
  }

  operator bool() const;

  static APIEvent Unserialize(std::string_view packet);
  std::vector<std::byte> Serialize() const;
  void Send() const;

  static const wchar_t* GetMailslotPath();

  /// String name of VisorVR::UserAction enum member
  static constexpr char EVT_REMOTE_USER_ACTION[] = "RemoteUserAction";

  /// struct SetTabByIDEvent
  static constexpr char EVT_SET_TAB_BY_ID[] = "SetTabByID";
  /// struct SetTabByNameEvent
  static constexpr char EVT_SET_TAB_BY_NAME[] = "SetTabByName";
  /// struct SetTabByIndexEvent
  static constexpr char EVT_SET_TAB_BY_INDEX[] = "SetTabByIndex";

  /// struct SetProfileByGUIDEvent
  static constexpr char EVT_SET_PROFILE_BY_GUID[] = "SetProfileByGUID";
  /// struct SetProfileByNameEvent
  static constexpr char EVT_SET_PROFILE_BY_NAME[] = "SetProfileByName";
  // struct SetBrightnessEvent
  static constexpr char EVT_SET_BRIGHTNESS[] = "SetBrightness";

  // struct PluginTabCustomActionEvent
  static constexpr char EVT_PLUGIN_TAB_CUSTOM_ACTION[] =
    "Plugin/Tab/CustomAction";

  // struct SetViewVRPoseEvent - sent by the OpenXR layer after the user moves
  // a panel in VR edit mode, so the app persists the new pose to Views.json.
  static constexpr char EVT_SET_VIEW_VR_POSE[] = "SetViewVRPose";

  /// JSON: "[ [name, value], [name, value], ... ]"
  static constexpr char EVT_MULTI_EVENT[] = "MultiEvent";

  // Triggered if a second VisorVR process is launched
  // Value is a string containing a copy of `GetCommandLineW()`, converted
  // to UTF-8
  static constexpr char EVT_VVR_EXECUTABLE_LAUNCHED[] = "VVRExecutableLaunched";

  inline static void Send(const APIEvent& ev) { ev.Send(); }
};

struct BaseSetTabEvent {
  // 0 = no change
  uint64_t mPageNumber {0};
  // 0 = 'active', 1 = primary, 2 = secondary
  uint8_t mKneeboard {0};
};

struct SetTabByIDEvent : public BaseSetTabEvent {
  static constexpr auto ID {APIEvent::EVT_SET_TAB_BY_ID};
  std::string mID;
};
VISORVR_DECLARE_JSON(SetTabByIDEvent);

struct SetTabByNameEvent : public BaseSetTabEvent {
  static constexpr auto ID {APIEvent::EVT_SET_TAB_BY_NAME};
  std::string mName;
};
VISORVR_DECLARE_JSON(SetTabByNameEvent);

struct SetTabByIndexEvent : public BaseSetTabEvent {
  static constexpr auto ID {APIEvent::EVT_SET_TAB_BY_INDEX};
  uint64_t mIndex {};
};
VISORVR_DECLARE_JSON(SetTabByIndexEvent);

struct SetProfileByGUIDEvent {
  static constexpr auto ID {APIEvent::EVT_SET_PROFILE_BY_GUID};
  std::string mGUID;
};
VISORVR_DECLARE_JSON(SetProfileByGUIDEvent);

struct SetProfileByNameEvent {
  static constexpr auto ID {APIEvent::EVT_SET_PROFILE_BY_NAME};
  std::string mName;
};
VISORVR_DECLARE_JSON(SetProfileByNameEvent);

struct SetBrightnessEvent {
  static constexpr auto ID {APIEvent::EVT_SET_BRIGHTNESS};

  enum class Mode {
    Absolute,
    Relative,
  };

  float mBrightness {};
  Mode mMode = Mode::Absolute;
};
VISORVR_DECLARE_JSON(SetBrightnessEvent);

struct PluginTabCustomActionEvent {
  static constexpr auto ID {APIEvent::EVT_PLUGIN_TAB_CUSTOM_ACTION};

  std::string mActionID;
  nlohmann::json mExtraData;
};
VISORVR_DECLARE_JSON(PluginTabCustomActionEvent);

// Persist a view's VR pose after in-VR edit-mode dragging. mLayerID is the
// runtime layer id (SHM::LayerConfig::mLayerID); the app maps it to a view.
// The 6 floats mirror VisorVR::VRPose (metres / radians).
struct SetViewVRPoseEvent {
  static constexpr auto ID {APIEvent::EVT_SET_VIEW_VR_POSE};

  uint64_t mLayerID {};
  float mX {};
  float mEyeY {};
  float mZ {};
  float mRX {};
  float mRY {};
  float mRZ {};
  // Absolute new physical size in metres (0 = leave the size unchanged).
  // Absolute, not a multiplier, so re-sending is idempotent.
  float mWidth {0.0f};
  float mHeight {0.0f};
};
VISORVR_DECLARE_JSON(SetViewVRPoseEvent);

}// namespace VisorVR

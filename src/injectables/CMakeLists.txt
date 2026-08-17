ok_add_library(detours-ext STATIC detours-ext.cpp)
target_link_libraries(detours-ext PUBLIC ThirdParty::detours VisorVR-dprint)

configure_file(
  VisorVR-OpenXR.in.def
  VisorVR-OpenXR${BUILD_BITNESS}.def
  @ONLY
  NEWLINE_STYLE UNIX
)
ok_add_library(
  VisorVR-OpenXR
  MODULE
  DUALARCH
  VisorVR-OpenXR${BUILD_BITNESS}.def
  OpenXRKneeboard.cpp
  OpenXRD3D11Kneeboard.cpp
  OpenXRD3D12Kneeboard.cpp
  OpenXRVulkanKneeboard.cpp
  OpenXRNext.cpp
)
target_link_libraries(
  VisorVR-OpenXR
  PRIVATE
  ThirdParty::OpenXR
  ThirdParty::DirectXTK
  ThirdParty::DirectXTK12
  ThirdParty::VulkanHeaders
  VisorVR-D3D11
  VisorVR-D3D12
  VisorVR-Vulkan
  VisorVR-APIEvent
  VisorVR-Elevation
  VisorVR-RayIntersectsRect
  VisorVR-SHM
  VisorVR-SHM-Client-D3D11
  VisorVR-SHM-Client-D3D12
  VisorVR-SHM-Client-Vulkan
  VisorVR-StateMachine
  VisorVR-VRKneeboard
  VisorVR-config
  VisorVR-dprint
  VisorVR-scope_exit
  VisorVR-shims
  VisorVR-version
)

configure_file(
  VisorVR-OpenXR.in.json
  VisorVR-OpenXR.intermediate.json
  NEWLINE_STYLE UNIX
)

# Keep the un-suffixed name to keep compatibility with previous installations
set(64BIT_OPENXR_JSON "${BUILD_OUT_BINDIR}/VisorVR-OpenXR.json")
set(32BIT_OPENXR_JSON "${BUILD_OUT_BINDIR}/VisorVR-OpenXR32.json")
if (BUILD_IS_32BIT)
  set(OPENXR_JSON_OUT "${32BIT_OPENXR_JSON}")
else ()
  set(OPENXR_JSON_OUT "${64BIT_OPENXR_JSON}")
endif ()
file(
  GENERATE
  OUTPUT "${OPENXR_JSON_OUT}"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/VisorVR-OpenXR.intermediate.json"
  NEWLINE_STYLE UNIX
)

ok_add_library(VisorVR-WindowCaptureHook MODULE DUALARCH WindowCaptureHook.cpp)
target_link_libraries(
  VisorVR-WindowCaptureHook
  PRIVATE
  VisorVR-config
  VisorVR-tracing
  detours-ext
)
set_target_properties(
  VisorVR-WindowCaptureHook
  PROPERTIES

  # Needed to avoid C function name mangling on 32-bit builds
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

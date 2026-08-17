include(GNUInstallDirs)

function(add_helper_executable TARGET)
  set(options 32BIT_ONLY)
  set(valueArgs)
  set(multiValueArgs)
  cmake_parse_arguments(HELPER_ARG "${options}" "${valueArgs}" "${multiValueArgs}" ${ARGN})

  ok_add_executable(${TARGET} EXCLUDE_FROM_ALL RUNTIME_DEPENDENCY ${ARGN})
  if (HELPER_ARG_32BIT_ONLY AND NOT BUILD_IS_32BIT)
    return()
  endif ()
endfunction()

function(add_utility_executable TARGET)
  ok_add_executable(
    ${TARGET}
    RUNTIME_EXE
    OUTPUT_SUBDIR "utilities"
    ${ARGN}
  )
endfunction()

include(remote-control.cmake)

# Sits at the top of a portable copy so there is something obvious to run;
# the real app has to stay in `bin` to find libexec and share beside it.
ok_add_executable(
  VisorVR-Portable-Launcher
  RUNTIME_EXE
  WIN32
  portable-launcher.cpp
  icon.rc
)
set_target_properties(
  VisorVR-Portable-Launcher
  PROPERTIES
  OUTPUT_NAME "VisorVR"
  RUNTIME_OUTPUT_DIRECTORY "${BUILD_OUT_ROOT}/launcher"
)

add_utility_executable(
  VisorVR-Plugin-Tab-Action
  WIN32
  plugin-tab-action.cpp
)
target_link_libraries(
  VisorVR-Plugin-Tab-Action
  PRIVATE
  VisorVR-dprint
  VisorVR-APIEvent
  VisorVR-tracing
)

add_utility_executable(
  VisorVR-RemoteControl-SET_TAB
  WIN32
  set-tab-remote.cpp
  remote-traceprovider.cpp
)
target_link_libraries(
  VisorVR-RemoteControl-SET_TAB
  PRIVATE
  VisorVR-dprint
  VisorVR-APIEvent
  VisorVR-tracing
)

add_utility_executable(
  VisorVR-RemoteControl-SET_PROFILE
  WIN32
  set-profile-remote.cpp
  remote-traceprovider.cpp
)
target_link_libraries(
  VisorVR-RemoteControl-SET_PROFILE
  PRIVATE
  VisorVR-APIEvent
  VisorVR-dprint
  VisorVR-tracing
)

add_utility_executable(
  VisorVR-Viewer
  WIN32
  viewer.cpp
  viewer-settings.cpp
  viewer-d3d11.cpp
  viewer-d3d12.cpp
  viewer-vulkan.cpp
  dds.hpp
  icon.rc
)
target_link_libraries(
  VisorVR-Viewer
  PRIVATE
  VisorVR-fatal
  VisorVR-config
  VisorVR-D2DErrorRenderer
  VisorVR-D3D11
  VisorVR-D3D12
  VisorVR-Vulkan
  VisorVR-DXResources
  VisorVR-Filesystem
  VisorVR-APIEvent
  VisorVR-GetSystemColor
  VisorVR-RenderDoc
  VisorVR-SHM
  VisorVR-SHM-Client-D3D11
  VisorVR-SHM-Client-D3D12
  VisorVR-SHM-Client-Vulkan
  VisorVR-Viewer-Shaders
  VisorVR-scope_exit
  System::D2d1
  System::D3d11
  System::Dcomp
  System::Dxgi
  ThirdParty::DirectXTK
)

add_helper_executable(
  VisorVR-OpenXR-Helper
  WIN32
  openxr-helper.cpp
  icon.rc
)
target_link_libraries(
  VisorVR-OpenXR-Helper
  PRIVATE
  VisorVR-OpenXRLayerRegistry
  VisorVR-RuntimeFiles
  VisorVR-dprint
)

add_helper_executable(
  VisorVR-WindowCaptureHook-Helper
  WIN32
  32BIT_ONLY
  WindowCaptureHook-helper.cpp
)
target_link_libraries(
  VisorVR-WindowCaptureHook-Helper
  PRIVATE
  VisorVR-WindowCaptureControl
  VisorVR-consolelib
  VisorVR-dprint
  VisorVR-shims
  System::Shell32
)

add_executable(capi-test capi-test.cpp)
target_include_directories(
  capi-test
  PRIVATE
  "$<TARGET_PROPERTY:VisorVR-c-api,INTERFACE_INCLUDE_DIRECTORIES>")
set_property(
  TARGET capi-test
  PROPERTY CXX_STANDARD 17)

ok_add_executable(symbolize-traces symbolize-traces.cpp)
target_link_libraries(
  symbolize-traces
  PRIVATE
  VisorVR-shims
  ThirdParty::DIA
  ThirdParty::WIL
)
set_target_properties(
  symbolize-traces
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${BUILD_OUT_PDBDIR}"
)
add_custom_command(
  TARGET symbolize-traces
  POST_BUILD
  COMMAND
  "${CMAKE_COMMAND}" -E copy_if_different
  "${DIASDK_DLL}"
  "$<TARGET_FILE_DIR:symbolize-traces>/"
)

add_executable(
  task-test
  task-test.cpp
)
target_link_libraries(
  task-test
  PRIVATE
  ThirdParty::WIL
  VisorVR-task
)
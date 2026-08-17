ok_add_library(VisorVR-Lib-Headers INTERFACE)
target_include_directories(VisorVR-Lib-Headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")


configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/VisorVR/config.in.hpp"
  "${CMAKE_CURRENT_BINARY_DIR}/include/VisorVR/config.hpp"
  @ONLY
)
file(
  GENERATE
  OUTPUT
  "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/include/VisorVR/detail/config.hpp"
  CONTENT "\
namespace VisorVR::detail::Config {
  constexpr auto BuildType = \"$<CONFIG>\";
}"
)
ok_add_library(VisorVR-config INTERFACE)
target_include_directories(
  VisorVR-config
  INTERFACE
  "${CMAKE_CURRENT_BINARY_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/include"
)
target_link_libraries(VisorVR-config INTERFACE VisorVR-Lib-Headers)

include(Geometry2D.cmake)

ok_add_library(VisorVR-ThreadGuard STATIC ThreadGuard.cpp)
target_link_libraries(VisorVR-ThreadGuard PUBLIC VisorVR-Lib-Headers)
target_link_libraries(VisorVR-ThreadGuard PRIVATE VisorVR-dprint)

ok_add_library(
  VisorVR-fatal
  STATIC
  fatal.cpp
  HEADERS
  include/VisorVR/fatal.hpp
  INCLUDE_DIRECTORIES
  include
)
target_link_libraries(
  VisorVR-fatal
  PUBLIC
  VisorVR-config
  PRIVATE
  VisorVR-Elevation
  VisorVR-dprint
  VisorVR-shims
  VisorVR-Filesystem
  VisorVR-version
  ThirdParty::CppWinRT
  ThirdParty::WIL
  ThirdParty::detours
  ThirdParty::felly
)

ok_add_library(VisorVR-GetMainHWND STATIC GetMainHWND.cpp)
target_link_libraries(
  VisorVR-GetMainHWND
  PUBLIC
  VisorVR-Lib-Headers
  VisorVR-win32
)
target_link_libraries(
  VisorVR-GetMainHWND
  PRIVATE
  VisorVR-config
  VisorVR-shims
)

ok_add_library(VisorVR-GetSystemColor STATIC GetSystemColor.cpp)
target_link_libraries(
  VisorVR-GetSystemColor
  PUBLIC
  VisorVR-Lib-Headers)

ok_add_library(VisorVR-handles INTERFACE)
target_link_libraries(
  VisorVR-handles
  INTERFACE
  VisorVR-Lib-Headers
)

ok_add_library(VisorVR-Elevation STATIC Elevation.cpp)
target_link_libraries(
  VisorVR-Elevation
  PUBLIC
  VisorVR-Lib-Headers
)
target_link_libraries(
  VisorVR-Elevation
  PRIVATE
  VisorVR-config
  VisorVR-dprint
  VisorVR-scope_exit
  ThirdParty::CppWinRT
  System::Shell32
)

ok_add_library(VisorVR-UTF8 STATIC utf8.cpp)
target_link_libraries(
  VisorVR-UTF8
  PUBLIC
  ThirdParty::JSON
  VisorVR-Lib-Headers
  PRIVATE
  VisorVR-fatal
)

ok_add_library(VisorVR-DebugTimer STATIC DebugTimer.cpp)
target_link_libraries(
  VisorVR-DebugTimer
  PRIVATE
  VisorVR-dprint
)
target_link_libraries(
  VisorVR-DebugTimer
  PUBLIC
  VisorVR-Lib-Headers
)

ok_add_library(VisorVR-tracing STATIC tracing.cpp)
target_link_libraries(
  VisorVR-tracing
  PRIVATE
  VisorVR-dprint
  ThirdParty::CppWinRT
)
target_link_libraries(
  VisorVR-tracing
  PUBLIC
  VisorVR-Lib-Headers
)

ok_add_library(
  VisorVR-D3D11
  STATIC
  D3D11.cpp
  D3D11/SpriteBatch.cpp
)
target_link_libraries(
  VisorVR-D3D11
  PUBLIC
  System::D3d11
  VisorVR-shims
  ThirdParty::DirectXTK
  ThirdParty::CppWinRT
  VisorVR-Lib-Headers
  VisorVR-Sprite-DXBC
  PRIVATE
  VisorVR-dprint
  System::Dxguid
)

ok_add_library(
  VisorVR-D3D12
  STATIC
  D3D12/SpriteBatch.cpp
)
target_link_libraries(
  VisorVR-D3D12
  PUBLIC
  System::D3d12
  VisorVR-shims
  ThirdParty::DirectXTK12
  ThirdParty::CppWinRT
  VisorVR-Lib-Headers
  VisorVR-SpriteBatch-DXIL
  PRIVATE
  VisorVR-dprint
  VisorVR-RenderDoc
  System::Dxguid
)

ok_add_library(
  VisorVR-Vulkan
  STATIC
  Vulkan.cpp
  Vulkan/SpriteBatch.cpp)
target_link_libraries(
  VisorVR-Vulkan
  PUBLIC
  VisorVR-dprint
  ThirdParty::VulkanHeaders
  VisorVR-Lib-Headers
  VisorVR-SpriteBatch-SPIRV
)

ok_add_library(VisorVR-DXResources STATIC DXResources.cpp)
target_link_libraries(
  VisorVR-DXResources
  PUBLIC
  VisorVR-Lib-Headers
  VisorVR-D3D11
  PRIVATE
  VisorVR-dprint
  System::Gdi32
)

set(RUNTIME_FILES_CPP "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/RuntimeFiles.cpp")
file(GENERATE OUTPUT "${RUNTIME_FILES_CPP}" INPUT RuntimeFiles.cpp.in)
ok_add_library(
  VisorVR-RuntimeFiles
  STATIC
  "${RUNTIME_FILES_CPP}"
  RuntimeFilesGetInstallationDirectory.cpp
)
target_link_libraries(
  VisorVR-RuntimeFiles
  PUBLIC
  VisorVR-Lib-Headers
  PRIVATE
  VisorVR-dprint
  VisorVR-Filesystem
)
if (ENABLE_APP)
  set(RUNTIME_FILES_APP_CPP "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/RuntimeFiles-app.cpp")
  file(GENERATE OUTPUT "${RUNTIME_FILES_APP_CPP}" INPUT RuntimeFiles-app.cpp.in)
  target_sources(VisorVR-RuntimeFiles PRIVATE "${RUNTIME_FILES_APP_CPP}")
endif ()

ok_add_library(
  VisorVR-OpenXRLayerRegistry
  STATIC
  OpenXRLayerRegistry.cpp
  HEADERS
  include/VisorVR/OpenXRLayerRegistry.hpp
  INCLUDE_DIRECTORIES
  include
)
target_link_libraries(
  VisorVR-OpenXRLayerRegistry
  PUBLIC
  VisorVR-Lib-Headers
  PRIVATE
  VisorVR-Filesystem
  VisorVR-RuntimeFiles
  VisorVR-dprint
)

ok_add_library(
  VisorVR-SHM
  STATIC
  SHM.cpp
  SHM/ActiveConsumers.cpp
)
target_link_libraries(
  VisorVR-SHM
  PUBLIC
  VisorVR-dprint
  VisorVR-Lib-Headers
  VisorVR-StateMachine
  PRIVATE
  VisorVR-Elevation
  VisorVR-config
  VisorVR-fatal
  VisorVR-shims
  VisorVR-version
  VisorVR-win32
  Boost::container
)

ok_add_library(
  VisorVR-SHM-Client-D3D11
  STATIC
  SHM/D3D11.cpp
  D3D11/Renderer.cpp
)
target_link_libraries(
  VisorVR-SHM-Client-D3D11
  PUBLIC
  VisorVR-shims
  VisorVR-SHM
  System::D3d11
  ThirdParty::CppWinRT
  ThirdParty::DirectXTK
  PRIVATE
  VisorVR-dprint
)

ok_add_library(
  VisorVR-SHM-Client-D3D12
  STATIC
  SHM/D3D12.cpp
  D3D12/Renderer.cpp
)
target_link_libraries(
  VisorVR-SHM-Client-D3D12
  PUBLIC
  VisorVR-shims
  VisorVR-SHM
  VisorVR-D3D12
  System::D3d12
  ThirdParty::CppWinRT
  ThirdParty::DirectXTK12
  PRIVATE
  VisorVR-dprint
)

ok_add_library(
  VisorVR-SHM-Client-Vulkan
  STATIC
  SHM/Vulkan.cpp
)
target_link_libraries(
  VisorVR-SHM-Client-Vulkan
  PUBLIC
  VisorVR-SHM
  VisorVR-Vulkan
  VisorVR-shims
  ThirdParty::CppWinRT
  ThirdParty::VulkanHeaders
)

ok_add_library(
  VisorVR-dprint
  STATIC
  dprint.cpp
  HEADERS
  include/VisorVR/dprint.hpp
  INCLUDE_DIRECTORIES
  include
)

target_link_libraries(
  VisorVR-dprint
  PUBLIC
  VisorVR-config
  VisorVR-fatal
  VisorVR-scope_exit
  ThirdParty::bindline
)
target_link_libraries(VisorVR-dprint
  PRIVATE
  VisorVR-scope_exit
)

ok_add_library(VisorVR-consolelib STATIC ConsoleLoopCondition.cpp)
target_link_libraries(
  VisorVR-consolelib
  PUBLIC
  VisorVR-Lib-Headers
  PRIVATE
  VisorVR-fatal
  VisorVR-win32
)

ok_add_library(VisorVR-APIEvent STATIC APIEvent.cpp)
target_link_libraries(VisorVR-APIEvent
  PUBLIC
  VisorVR-Lib-Headers
  VisorVR-UTF8
  VisorVR-json
  PRIVATE
  VisorVR-config
  VisorVR-dprint
)

ok_add_library(VisorVR-D2DErrorRenderer STATIC D2DErrorRenderer.cpp)
target_link_libraries(
  VisorVR-D2DErrorRenderer
  PUBLIC
  VisorVR-DXResources
  VisorVR-UTF8
  VisorVR-config
  System::D2d1
  System::Dwrite
  VisorVR-Lib-Headers)

ok_add_library(VisorVR-RayIntersectsRect STATIC RayIntersectsRect.cpp)
target_link_libraries(
  VisorVR-RayIntersectsRect
  PUBLIC
  VisorVR-Lib-Headers
  ThirdParty::DirectXTK)
target_link_libraries(
  VisorVR-RayIntersectsRect
  PRIVATE
  VisorVR-dprint)

ok_add_library(VisorVR-StateMachine INTERFACE)
target_link_libraries(
  VisorVR-StateMachine
  INTERFACE
  VisorVR-Lib-Headers
  VisorVR-dprint
  ThirdParty::magic_enum
)

ok_add_library(VisorVR-scope_exit INTERFACE)
target_link_libraries(
  VisorVR-scope_exit
  INTERFACE
  VisorVR-Lib-Headers
  ThirdParty::felly
)

ok_add_library(VisorVR-VRKneeboard STATIC VRKneeboard.cpp)
target_link_libraries(
  VisorVR-VRKneeboard
  PUBLIC
  VisorVR-Lib-Headers
  ThirdParty::DirectXTK
  VisorVR-RayIntersectsRect
  VisorVR-SHM
)

set(VERSION_CPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/include/VisorVR/version.hpp")
add_custom_target(
  VisorVR-version-sources
  COMMAND
  ${CMAKE_COMMAND}
  -DVERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
  -DVERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
  -DVERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  -DVERSION_BUILD=${CMAKE_PROJECT_VERSION_TWEAK}
  -DRELEASE_NAME=${RELEASE_NAME}
  -DOPENXR_API_LAYER_IMPLEMENTATION_VERSION=${OPENXR_API_LAYER_IMPLEMENTATION_VERSION}
  "-DINPUT_CPP_FILE=${CMAKE_CURRENT_SOURCE_DIR}/include/VisorVR/version.in.hpp"
  "-DOUTPUT_CPP_FILE=${VERSION_CPP_FILE}"
  "-P${CMAKE_CURRENT_SOURCE_DIR}/../version.cmake"
  DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../version.cmake"
  "version.hpp.in"
  BYPRODUCTS
  "${VERSION_CPP_FILE}"
  WORKING_DIRECTORY
  "${CMAKE_SOURCE_DIR}"
)
ok_add_library(VisorVR-version INTERFACE)
target_include_directories(
  VisorVR-version
  INTERFACE
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)
add_dependencies(VisorVR-version VisorVR-version-sources)
ok_add_library(
  VisorVR-semver
  STATIC
  semver.cpp
  HEADERS
  include/VisorVR/semver.hpp
  INCLUDE_DIRECTORIES
  include
)
target_link_libraries(
  VisorVR-semver
  PRIVATE
  VisorVR-dprint
  ThirdParty::SemVer
)

ok_add_library(VisorVR-shims INTERFACE)
target_include_directories(
  VisorVR-shims
  INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(
  VisorVR-shims
  INTERFACE
  VisorVR-dprint
  VisorVR-fatal
)

ok_add_library(VisorVR-json STATIC json.cpp)
target_link_libraries(
  VisorVR-json
  PUBLIC
  VisorVR-dprint
  ThirdParty::JSON
  VisorVR-Lib-Headers
)

ok_add_library(VisorVR-Filesystem STATIC Filesystem.cpp)
target_link_libraries(
  VisorVR-Filesystem
  PUBLIC
  VisorVR-shims
  VisorVR-config
  VisorVR-Lib-Headers
  PRIVATE
  VisorVR-StateMachine
  VisorVR-UTF8
  VisorVR-dprint
  VisorVR-fatal
  System::Shlwapi
  ThirdParty::CppWinRT
  ThirdParty::WIL
)

ok_add_library(VisorVR-WindowCaptureControl STATIC WindowCaptureControl.cpp)
target_link_libraries(
  VisorVR-WindowCaptureControl
  PRIVATE
  VisorVR-RuntimeFiles
  VisorVR-dprint
  VisorVR-shims
)
target_link_libraries(
  VisorVR-WindowCaptureControl
  PUBLIC
  VisorVR-handles
  VisorVR-Lib-Headers
)

ok_add_library(
  VisorVR-RenderDoc
  STATIC
  RenderDoc.cpp
)
target_link_libraries(
  VisorVR-RenderDoc
  PUBLIC
  VisorVR-Lib-Headers
  PRIVATE
  VisorVR-fatal
  ThirdParty::RenderDoc
)
ok_add_library(
  VisorVR-EnumerateProcesses
  STATIC
  EnumerateProcesses.cpp
  HEADERS
  include/VisorVR/EnumerateProcesses.hpp
  INCLUDE_DIRECTORIES
  include
)
target_link_libraries(
  VisorVR-EnumerateProcesses
  PRIVATE
  VisorVR-dprint
  VisorVR-tracing
)

ok_add_library(
  VisorVR-win32
  INTERFACE
  HEADERS
  include/VisorVR/Win32.hpp
  INCLUDE_DIRECTORIES
  include
)
target_link_libraries(
  VisorVR-win32
  INTERFACE
  VisorVR-fatal
  VisorVR-shims
  ThirdParty::bindline
  ThirdParty::CppWinRT
)

add_library(VisorVR-task INTERFACE)
target_link_libraries(
  VisorVR-task
  INTERFACE
  VisorVR-StateMachine
  VisorVR-dprint
  VisorVR-fatal
  VisorVR-shims
  ThirdParty::CppWinRT
  ThirdParty::magic_enum
)

ok_add_library(
  VisorVR-UserAction
  INTERFACE
  HEADERS include/VisorVR/UserAction.hpp
  INCLUDE_DIRECTORIES include
)
# Output filenames use underscores rather than hyphens for
# increased compatibility; in particular, LUA requires the DLL
# name matches a C function name.

ok_add_library(
  VisorVR-lua-api
  MODULE
  DUALARCH
  OUTPUT_NAME "VisorVR_LuaAPI"
  VisorVR_LuaAPI.cpp)
target_link_libraries(
  VisorVR-lua-api
  PUBLIC
  ThirdParty::Lua::DelayLoad
  PRIVATE
  VisorVR-dprint
  VisorVR-tracing
  VisorVR-APIEvent
)
# Install to a separate directory as for lua, we generally
# want to modify `cpath`; let's not pollute cpath with
# all the other dlls that VisorVR needs on
set_target_properties(
  VisorVR-lua-api
  PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUT_ROOT}/lib/lua"
)

ok_add_library(
  VisorVR-c-api
  MODULE
  DUALARCH
  OUTPUT_NAME "VisorVR_CAPI"
  VisorVR_CAPI.cpp)
target_link_libraries(
  VisorVR-c-api
  PRIVATE
  VisorVR-dprint
  VisorVR-tracing
  VisorVR-APIEvent)
target_include_directories(
  VisorVR-c-api
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR})
file(
  GENERATE
  OUTPUT
  "${BUILD_OUT_ROOT}/include/VisorVR_CAPI.h"
  INPUT
  "VisorVR_CAPI.h"
)

add_subdirectory(app-common)

if (NOT ENABLE_APP)
  return()
endif ()

add_subdirectory(app-winui3)

ok_add_runtime_files(copy-welcome-html "${CMAKE_SOURCE_DIR}/assets/Welcome.html")

add_dependencies(VisorVR-App-WinUI3 runtime-dependencies)
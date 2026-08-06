add_subdirectory(app-common)

if (NOT ENABLE_APP)
  return()
endif ()

add_subdirectory(app-winui3)

add_dependencies(OpenKneeboard-App-WinUI3 runtime-dependencies)
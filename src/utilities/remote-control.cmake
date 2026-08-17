find_package(magic_args CONFIG REQUIRED)
include(MagicArgs)

add_utility_executable(
  VisorVR-RemoteControl
  WIN32
  remote-control.cpp
  simple-remotes.cpp
  simple-remotes.hpp
  remote-traceprovider.cpp
)
target_link_libraries(
  VisorVR-RemoteControl
  PRIVATE
  VisorVR-APIEvent
  VisorVR-UserAction
  VisorVR-dprint
  VisorVR-tracing
  magic_args::magic_args
)
magic_args_enumerate_subcommands(
  VisorVR-RemoteControl
  HARDLINKS_DIR "$<TARGET_FILE_DIR:VisorVR-RemoteControl>"
  TEXT_FILE "$<TARGET_FILE_DIR:VisorVR-RemoteControl>/$<TARGET_FILE_BASE_NAME:VisorVR-RemoteControl>-aliases.txt"
)

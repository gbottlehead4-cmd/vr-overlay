ok_add_library(VisorVR-Geometry2D INTERFACE include/VisorVR/Geometry2D.hpp)
target_link_libraries(VisorVR-Geometry2D INTERFACE ThirdParty::felly)
target_include_directories(VisorVR-Geometry2D INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
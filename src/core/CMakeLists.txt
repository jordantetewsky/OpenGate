# Common usage requirements; native implementation is supplied by the platform.
add_library(UnboundCoreHeaders INTERFACE)
target_include_directories(UnboundCoreHeaders INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/../platform")
target_link_libraries(UnboundCoreHeaders INTERFACE Qt5::Widgets)

include(CMakeFindDependencyMacro)

find_dependency(expected)
find_dependency(tl::expected)
find_dependency(tl-expected)

include("${CMAKE_CURRENT_LIST_DIR}/router_targets.cmake")

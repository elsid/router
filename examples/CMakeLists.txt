add_subdirectory(rpg)

add_executable(community_example community.cpp)
target_compile_options(community_example PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)
target_compile_features(community_example PRIVATE cxx_std_20)
target_link_libraries(community_example PRIVATE router)

add_executable(int_if_then_example int_if_then.cpp)
target_compile_options(int_if_then_example PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)
target_compile_features(int_if_then_example PRIVATE cxx_std_20)

add_executable(int_router_example int_router.cpp)
target_compile_options(int_router_example PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)
target_compile_features(int_router_example PRIVATE cxx_std_20)

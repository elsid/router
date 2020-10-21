add_subdirectory(rpg)

add_executable(community_example community.cpp)
target_compile_options(community_example PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)
target_compile_features(community_example PRIVATE cxx_std_20)

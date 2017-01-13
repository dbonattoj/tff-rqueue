project(tff-rqueue CXX)
cmake_minimum_required(VERSION 3.1)

if(UNIX)
	add_compile_options(-std=c++14)
elseif(WIN32)
	add_compile_options(/wd4018)
endif()

enable_testing()

include_directories(SYSTEM external/include)
add_compile_options(-Wall -Wextra -Wno-sign-compare)

# Test
file(GLOB_RECURSE TEST_SRC "test/*.cc" "src/*")
add_executable(tff_rqueue_test ${TEST_SRC})
target_compile_definitions(tff_rqueue_test PRIVATE TFF_ND_STANDALONE)
target_link_libraries(tff_rqueue_test pthread)

add_test(COMMAND tff_rqueue_test)

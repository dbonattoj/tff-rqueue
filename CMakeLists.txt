project(tff-rqueue CXX)
cmake_minimum_required(VERSION 3.1)

if(UNIX)
	add_compile_options(-std=c++14)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")
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
add_test(COMMAND tff_rqueue_test)

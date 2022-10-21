cmake_minimum_required(VERSION 3.15)
project(libprotoserial)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    test_libprotoserial
    tests/tests.cpp
)
target_include_directories(
    test_libprotoserial PRIVATE "include"
    test_libprotoserial PRIVATE "submodules/etl/include"
    test_libprotoserial PRIVATE "submodules/jsoncons/include"
)

target_link_libraries(
    test_libprotoserial
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_libprotoserial)


include(CheckCXXCompilerFlag)

# from https://stackoverflow.com/a/33266748
# feel free to implement a better solution
function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")

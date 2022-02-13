cmake_minimum_required(VERSION 3.14)
project(libprotoserial)

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()


#file (GLOB_RECURSE libprotoserial_HEADERS CONFIGURE_DEPENDS "libprotoserial/*.hpp")
#set (libprotoserial_INCLUDE_DIRS "")
#foreach (_headerFile ${libprotoserial_HEADERS})
#    get_filename_component(_dir ${_headerFile} PATH)
#    list (APPEND libprotoserial_INCLUDE_DIRS ${_dir})
#endforeach()
#list (REMOVE_DUPLICATES libprotoserial_INCLUDE_DIRS)



add_executable(
    test_libprotoserial
    libprotoserial/container.test.cpp 
    libprotoserial/container.cpp 
    libprotoserial/byte.cpp
    )
target_include_directories(test_libprotoserial PRIVATE ".")

target_link_libraries(
    test_libprotoserial
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_libprotoserial)
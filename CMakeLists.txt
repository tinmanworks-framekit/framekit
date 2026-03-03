cmake_minimum_required(VERSION 3.23)

project(framekit VERSION 0.1.0 LANGUAGES C CXX)

option(FRAMEKIT_BUILD_TESTS "Build framekit tests" ON)
option(FRAMEKIT_BUILD_EXAMPLES "Build framekit examples" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(framekit_core STATIC
    src/multiprocess_runtime.cpp
)
add_library(FrameKit::Core ALIAS framekit_core)

target_include_directories(framekit_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(framekit_core PUBLIC cxx_std_20)

if(FRAMEKIT_BUILD_EXAMPLES)
    add_executable(framekit_single_process_example
        examples/single_process/main.cpp
    )
    target_link_libraries(framekit_single_process_example PRIVATE FrameKit::Core)
endif()

if(FRAMEKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

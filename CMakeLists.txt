cmake_minimum_required(VERSION 3.23)

project(framekit VERSION 0.1.0 LANGUAGES C CXX)

option(FRAMEKIT_BUILD_TESTS "Build framekit tests" ON)
option(FRAMEKIT_BUILD_EXAMPLES "Build framekit examples" ON)
option(FRAMEKIT_ENABLE_NETKIT "Enable NetKit-dependent framekit integrations" OFF)
if(APPLE)
    option(FRAMEKIT_ENABLE_COCOA "Enable Cocoa window backend module" ON)
else()
    option(FRAMEKIT_ENABLE_COCOA "Enable Cocoa window backend module" OFF)
endif()

set(FRAMEKIT_NETKIT_SOURCE_DIR "" CACHE PATH "Path to netkit source directory")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FRAMEKIT_CORE_SOURCES
    src/multiprocess_runtime.cpp
    src/window/window_backend.cpp
)

if(APPLE AND FRAMEKIT_ENABLE_COCOA)
    enable_language(OBJCXX)
    list(APPEND FRAMEKIT_CORE_SOURCES src/window/cocoa_backend.mm)
else()
    list(APPEND FRAMEKIT_CORE_SOURCES src/window/cocoa_backend_stub.cpp)
endif()

add_library(framekit_core STATIC ${FRAMEKIT_CORE_SOURCES})
add_library(FrameKit::Core ALIAS framekit_core)

target_include_directories(framekit_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(framekit_core PUBLIC cxx_std_20)

if(APPLE AND FRAMEKIT_ENABLE_COCOA)
    target_compile_definitions(framekit_core PUBLIC FRAMEKIT_HAS_COCOA_BACKEND=1)
    target_link_libraries(framekit_core PRIVATE "-framework AppKit")
else()
    target_compile_definitions(framekit_core PUBLIC FRAMEKIT_HAS_COCOA_BACKEND=0)
endif()

if(FRAMEKIT_BUILD_EXAMPLES)
    add_executable(framekit_single_process_example
        examples/single_process/main.cpp
    )
    target_link_libraries(framekit_single_process_example PRIVATE FrameKit::Core)

    add_executable(framekit_multi_worker_example
        examples/multi_worker/main.cpp
    )
    target_link_libraries(framekit_multi_worker_example PRIVATE FrameKit::Core)
endif()

if(FRAMEKIT_ENABLE_NETKIT)
    if(NOT TARGET NetKit::Transport AND EXISTS "${FRAMEKIT_NETKIT_SOURCE_DIR}/CMakeLists.txt")
        set(NETKIT_BUILD_TESTS OFF CACHE BOOL "Disable netkit tests when consumed by framekit" FORCE)
        add_subdirectory(${FRAMEKIT_NETKIT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/_deps/netkit EXCLUDE_FROM_ALL)
    endif()

    if(NOT TARGET NetKit::Transport)
        find_package(netkit CONFIG QUIET)
    endif()

    if(FRAMEKIT_BUILD_EXAMPLES AND TARGET NetKit::Transport)
        add_executable(framekit_frontend_example
            examples/frontend_backend/frontend_main.cpp
        )
        target_link_libraries(framekit_frontend_example PRIVATE FrameKit::Core NetKit::Transport)

        add_executable(framekit_backend_example
            examples/frontend_backend/backend_main.cpp
        )
        target_link_libraries(framekit_backend_example PRIVATE FrameKit::Core NetKit::Transport)
    elseif(FRAMEKIT_BUILD_EXAMPLES)
        message(WARNING "FRAMEKIT_ENABLE_NETKIT=ON but NetKit::Transport was not found; skipping netkit examples")
    endif()
endif()

if(FRAMEKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

cmake_minimum_required(VERSION 3.23)

project(framekit VERSION 2.0.0 LANGUAGES C CXX)

option(FRAMEKIT_BUILD_TESTS "Build framekit tests" ON)
option(FRAMEKIT_BUILD_EXAMPLES "Build framekit examples" ON)
option(FRAMEKIT_ENABLE_NETKIT "Enable NetKit-dependent framekit integrations" OFF)
option(FRAMEKIT_ENABLE_COCOA "Enable experimental macOS Cocoa backend support" OFF)

set(FRAMEKIT_NETKIT_SOURCE_DIR "" CACHE PATH "Path to netkit source directory")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(FRAMEKIT_ENABLE_COCOA AND NOT APPLE)
    message(FATAL_ERROR "FRAMEKIT_ENABLE_COCOA is only supported on macOS")
endif()

if(FRAMEKIT_ENABLE_COCOA)
    enable_language(OBJCXX)
endif()

add_library(framekit_core STATIC
    src/multiprocess_runtime.cpp
    src/lifecycle_state_machine.cpp
    src/loop_policy.cpp
    src/fault_policy_runtime.cpp
    src/loop_stage_graph.cpp
    src/input_routing_runtime.cpp
    src/platform_host_runtime.cpp
    src/module_graph.cpp
    src/event_bus.cpp
    src/bootstrap_services.cpp
    src/kernel_runtime.cpp
)

if(FRAMEKIT_ENABLE_COCOA)
    target_sources(framekit_core PRIVATE
        src/cocoa_platform_backend.mm
    )
else()
    target_sources(framekit_core PRIVATE
        src/platform_backend_factory_stub.cpp
    )
endif()
add_library(FrameKit::Core ALIAS framekit_core)

target_include_directories(framekit_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(framekit_core PUBLIC cxx_std_20)

if(FRAMEKIT_ENABLE_COCOA)
    target_link_libraries(framekit_core PRIVATE "-framework AppKit")
    target_compile_options(framekit_core PRIVATE
        $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
    )
endif()

if(FRAMEKIT_BUILD_TESTS)
    enable_testing()
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

    if(FRAMEKIT_BUILD_TESTS AND APPLE AND FRAMEKIT_ENABLE_COCOA)
        add_test(NAME framekit_example_launch_single_process
            COMMAND framekit_single_process_example
        )
        add_test(NAME framekit_example_launch_multi_worker
            COMMAND framekit_multi_worker_example
        )
        set_tests_properties(
            framekit_example_launch_single_process
            framekit_example_launch_multi_worker
            PROPERTIES LABELS "example-launch;platform;cocoa"
        )
    endif()
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
    add_subdirectory(tests)
endif()

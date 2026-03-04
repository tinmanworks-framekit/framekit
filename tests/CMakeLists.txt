add_executable(framekit_contracts_test
    test_contracts.cpp
)

target_link_libraries(framekit_contracts_test PRIVATE FrameKit::Core)
add_test(NAME framekit_contracts_test COMMAND framekit_contracts_test)

add_executable(framekit_window_backend_test
    test_window_backend.cpp
)

target_link_libraries(framekit_window_backend_test PRIVATE FrameKit::Core)
add_test(NAME framekit_window_backend_test COMMAND framekit_window_backend_test)

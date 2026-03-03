add_executable(framekit_contracts_test
    test_contracts.cpp
)

target_link_libraries(framekit_contracts_test PRIVATE FrameKit::Core)
add_test(NAME framekit_contracts_test COMMAND framekit_contracts_test)

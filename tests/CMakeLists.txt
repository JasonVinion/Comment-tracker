find_package(Qt6 REQUIRED COMPONENTS Test)

foreach(_test CommentExtractor EditPlanner FileCrawler TreeSitter)
    add_executable(test_${_test} test_${_test}.cpp)
    target_link_libraries(test_${_test} PRIVATE comment_core Qt6::Test)
    target_compile_definitions(test_${_test} PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/test_examples")
    add_test(NAME ${_test}Test COMMAND test_${_test})
endforeach()

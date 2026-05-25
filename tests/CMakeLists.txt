cmake_minimum_required(VERSION 3.16)

# Find Qt6 Test
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Concurrent)

# Enable AUTOMOC for tests
set(CMAKE_AUTOMOC ON)

# Test for CommentExtractor
add_executable(test_comment_extractor
    test_CommentExtractor.cpp
    ${CMAKE_SOURCE_DIR}/src/CommentExtractor.cpp
    ${CMAKE_SOURCE_DIR}/src/LanguageRegistry.cpp
    ${CMAKE_SOURCE_DIR}/include/CommentExtractor.h
    ${CMAKE_SOURCE_DIR}/include/LanguageRegistry.h
)

target_include_directories(test_comment_extractor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${tree-sitter_SOURCE_DIR}/lib/include
)

target_link_libraries(test_comment_extractor PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    tree-sitter
    tree-sitter-json
    tree-sitter-cpp
    tree-sitter-python
    tree-sitter-javascript
)

add_test(NAME CommentExtractorTest COMMAND test_comment_extractor)

# Test for EditPlanner
add_executable(test_edit_planner
    test_EditPlanner.cpp
    ${CMAKE_SOURCE_DIR}/src/EditPlanner.cpp
    ${CMAKE_SOURCE_DIR}/src/CommentExtractor.cpp
    ${CMAKE_SOURCE_DIR}/src/LanguageRegistry.cpp
    ${CMAKE_SOURCE_DIR}/include/EditPlanner.h
    ${CMAKE_SOURCE_DIR}/include/CommentExtractor.h
    ${CMAKE_SOURCE_DIR}/include/LanguageRegistry.h
)

target_include_directories(test_edit_planner PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${tree-sitter_SOURCE_DIR}/lib/include
)

target_link_libraries(test_edit_planner PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    tree-sitter
    tree-sitter-json
    tree-sitter-cpp
    tree-sitter-python
    tree-sitter-javascript
)

target_link_libraries(test_edit_planner PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    tree-sitter
    tree-sitter-json
    tree-sitter-cpp
    tree-sitter-python
    tree-sitter-javascript
)

add_test(NAME EditPlannerTest COMMAND test_edit_planner)

# Test for FileCrawler
add_executable(test_file_crawler
    test_FileCrawler.cpp
    ${CMAKE_SOURCE_DIR}/src/FileCrawler.cpp
    ${CMAKE_SOURCE_DIR}/src/LanguageRegistry.cpp
    ${CMAKE_SOURCE_DIR}/include/FileCrawler.h
    ${CMAKE_SOURCE_DIR}/include/LanguageRegistry.h
)

target_include_directories(test_file_crawler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_file_crawler PRIVATE
    Qt6::Core
    Qt6::Concurrent
    Qt6::Test
    Qt6::Widgets
)

add_test(NAME FileCrawlerTest COMMAND test_file_crawler)

# Test for TreeSitter
add_executable(test_tree_sitter
    test_TreeSitter.cpp
    ${CMAKE_SOURCE_DIR}/src/CommentExtractor.cpp
    ${CMAKE_SOURCE_DIR}/src/LanguageRegistry.cpp
    ${CMAKE_SOURCE_DIR}/include/CommentExtractor.h
    ${CMAKE_SOURCE_DIR}/include/LanguageRegistry.h
)

target_include_directories(test_tree_sitter PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${tree-sitter_SOURCE_DIR}/lib/include
)

target_link_libraries(test_tree_sitter PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    tree-sitter
    tree-sitter-json
    tree-sitter-cpp
    tree-sitter-python
    tree-sitter-javascript
)

add_test(NAME TreeSitterTest COMMAND test_tree_sitter)

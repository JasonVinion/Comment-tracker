cmake_minimum_required(VERSION 3.16)

# Force x64-windows triplet for vcpkg on Windows before project()
if(CMAKE_HOST_WIN32)
    # Force x64-windows to match the detected x64 compiler environment
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet" FORCE)
endif()

include(FetchContent)

# Enable vcpkg manifest mode automatically when available
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_vcpkg_root "")

    if(DEFINED ENV{VCPKG_ROOT})
        set(_vcpkg_root "$ENV{VCPKG_ROOT}")
    endif()

    if(NOT _vcpkg_root AND WIN32)
        # Try to locate vcpkg in the path
        find_program(VCPKG_EXECUTABLE vcpkg)
        if(VCPKG_EXECUTABLE)
             get_filename_component(_vcpkg_root_dir "${VCPKG_EXECUTABLE}" DIRECTORY)
             if(EXISTS "${_vcpkg_root_dir}/scripts/buildsystems/vcpkg.cmake")
                 set(_vcpkg_root "${_vcpkg_root_dir}")
             endif()
        endif()

        # Fallback to standard Visual Studio locations if not in path
        if(NOT _vcpkg_root)
            foreach(_ver RANGE 17 19)
                foreach(_edition Community Professional Enterprise)
                    set(_try_path "C:/Program Files/Microsoft Visual Studio/${_ver}/${_edition}/VC/vcpkg")
                    if(EXISTS "${_try_path}/scripts/buildsystems/vcpkg.cmake")
                        set(_vcpkg_root "${_try_path}")
                        break()
                    endif()
                endforeach()
                if(_vcpkg_root)
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(NOT _vcpkg_root)
        FetchContent_Declare(
            vcpkg
            GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
            GIT_TAG 2024.07.12
        )
        FetchContent_Populate(vcpkg)
        set(_vcpkg_root "${vcpkg_SOURCE_DIR}")
        
        # Explicitly bootstrap vcpkg if needed
        if(WIN32)
            set(_vcpkg_exe "${_vcpkg_root}/vcpkg.exe")
            if(NOT EXISTS "${_vcpkg_exe}")
                message(STATUS "Bootstrapping vcpkg in ${_vcpkg_root}...")
                execute_process(
                    COMMAND "${_vcpkg_root}/bootstrap-vcpkg.bat" -disableMetrics
                    WORKING_DIRECTORY "${_vcpkg_root}"
                    RESULT_VARIABLE _boot_res
                    OUTPUT_VARIABLE _boot_out
                )
                if(NOT _boot_res EQUAL 0)
                     message(FATAL_ERROR "Failed to bootstrap vcpkg: ${_boot_res}\n${_boot_out}")
                endif()
            endif()
        endif()
    endif()

    if(_vcpkg_root)
        if(WIN32)
            set(_vcpkg_bootstrap "${_vcpkg_root}/bootstrap-vcpkg.bat")
            set(_vcpkg_exe "${_vcpkg_root}/vcpkg.exe")
        else()
            set(_vcpkg_bootstrap "${_vcpkg_root}/bootstrap-vcpkg.sh")
            set(_vcpkg_exe "${_vcpkg_root}/vcpkg")
        endif()

        if(EXISTS "${_vcpkg_bootstrap}" AND NOT EXISTS "${_vcpkg_exe}")
            execute_process(
                COMMAND "${_vcpkg_bootstrap}"
                WORKING_DIRECTORY "${_vcpkg_root}"
            )
        endif()

        set(ENV{VCPKG_ROOT} "${_vcpkg_root}")
        set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)

        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/my-ports")
            set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/my-ports" CACHE STRING "Vcpkg overlay ports" FORCE)
        endif()
    endif()
endif()

project(CommentExplorer VERSION 1.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
if(NOT DEFINED Qt6_DIR AND DEFINED ENV{Qt6_DIR})
    set(Qt6_DIR "$ENV{Qt6_DIR}" CACHE PATH "Qt6 package directory")
endif()

if(DEFINED ENV{QTDIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()

if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    # Triplet already set at the top
endif()

# Ensure dependencies are installed via vcpkg
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/vcpkg.json")
    find_program(VCPKG_CMD vcpkg HINTS "${_vcpkg_root}" "$ENV{VCPKG_ROOT}" PATHS "C:/vcpkg" "C:/src/vcpkg")
    if(VCPKG_CMD)
        message(STATUS "Found vcpkg: ${VCPKG_CMD}")
        # Always try to run install if not found, or relying on manifest mode
        # If toolchain is used, this might be redundant but safe
        set(ENV{VCPKG_ROOT} "${_vcpkg_root}")
        
        # Setup prefix path just in case
        list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

if(WIN32 AND NOT DEFINED Qt6_DIR)
    file(GLOB _qt6_dir_candidates "C:/Qt/*/msvc*_64/lib/cmake/Qt6")
    list(LENGTH _qt6_dir_candidates _qt6_dir_count)
    if(_qt6_dir_count GREATER 0)
        list(GET _qt6_dir_candidates 0 Qt6_DIR)
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Concurrent)

# Tree-sitter main library
FetchContent_Declare(
  tree-sitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter
  GIT_TAG        v0.22.6
)
FetchContent_MakeAvailable(tree-sitter)

# Define tree-sitter library
add_library(tree-sitter STATIC
    ${tree-sitter_SOURCE_DIR}/lib/src/lib.c
)
target_include_directories(tree-sitter PUBLIC
    ${tree-sitter_SOURCE_DIR}/lib/include
    ${tree-sitter_SOURCE_DIR}/lib/src
)
set_target_properties(tree-sitter PROPERTIES C_STANDARD 11)

# Tree-sitter languages
# JSON
FetchContent_Declare(
  tree-sitter-json
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json
  GIT_TAG        v0.21.0
)
FetchContent_MakeAvailable(tree-sitter-json)

add_library(tree-sitter-json STATIC
    ${tree-sitter-json_SOURCE_DIR}/src/parser.c
)
target_include_directories(tree-sitter-json PUBLIC
    ${tree-sitter-json_SOURCE_DIR}/src
    ${tree-sitter_SOURCE_DIR}/lib/include
)

# C++
FetchContent_Declare(
  tree-sitter-cpp
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-cpp
  GIT_TAG        v0.22.0
)
FetchContent_MakeAvailable(tree-sitter-cpp)

set(TS_CPP_SOURCES ${tree-sitter-cpp_SOURCE_DIR}/src/parser.c)
if(EXISTS "${tree-sitter-cpp_SOURCE_DIR}/src/scanner.c")
    list(APPEND TS_CPP_SOURCES "${tree-sitter-cpp_SOURCE_DIR}/src/scanner.c")
elseif(EXISTS "${tree-sitter-cpp_SOURCE_DIR}/src/scanner.cc")
    list(APPEND TS_CPP_SOURCES "${tree-sitter-cpp_SOURCE_DIR}/src/scanner.cc")
endif()

add_library(tree-sitter-cpp STATIC ${TS_CPP_SOURCES})
target_include_directories(tree-sitter-cpp PUBLIC
    ${tree-sitter-cpp_SOURCE_DIR}/src
    ${tree-sitter_SOURCE_DIR}/lib/include
)

# Python
FetchContent_Declare(
  tree-sitter-python
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-python
  GIT_TAG        v0.21.0
)
FetchContent_MakeAvailable(tree-sitter-python)

set(TS_PYTHON_SOURCES ${tree-sitter-python_SOURCE_DIR}/src/parser.c)
if(EXISTS "${tree-sitter-python_SOURCE_DIR}/src/scanner.c")
    list(APPEND TS_PYTHON_SOURCES "${tree-sitter-python_SOURCE_DIR}/src/scanner.c")
endif()

add_library(tree-sitter-python STATIC ${TS_PYTHON_SOURCES})
target_include_directories(tree-sitter-python PUBLIC
    ${tree-sitter-python_SOURCE_DIR}/src
    ${tree-sitter_SOURCE_DIR}/lib/include
)

# JavaScript
FetchContent_Declare(
  tree-sitter-javascript
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-javascript
  GIT_TAG        v0.21.1
)
FetchContent_MakeAvailable(tree-sitter-javascript)

set(TS_JS_SOURCES ${tree-sitter-javascript_SOURCE_DIR}/src/parser.c)
if(EXISTS "${tree-sitter-javascript_SOURCE_DIR}/src/scanner.c")
    list(APPEND TS_JS_SOURCES "${tree-sitter-javascript_SOURCE_DIR}/src/scanner.c")
endif()

add_library(tree-sitter-javascript STATIC ${TS_JS_SOURCES})
target_include_directories(tree-sitter-javascript PUBLIC
    ${tree-sitter-javascript_SOURCE_DIR}/src
    ${tree-sitter_SOURCE_DIR}/lib/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/FileCrawler.cpp
    src/LanguageRegistry.cpp
    src/CommentExtractor.cpp
    src/CommentModel.cpp
    src/EditPlanner.cpp
    src/FileWriter.cpp
    src/DiffPreview.cpp
    src/SyntaxHighlighter.cpp
    src/SettingsDialog.cpp
)

# Header files
set(HEADERS
    include/MainWindow.h
    include/FileCrawler.h
    include/LanguageRegistry.h
    include/CommentExtractor.h
    include/CommentModel.h
    include/EditPlanner.h
    include/FileWriter.h
    include/DiffPreview.h
    include/SyntaxHighlighter.h
    include/SettingsDialog.h
    include/Comment.h
)

# Resource files
set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    list(APPEND RESOURCES resources/app.rc)
    set_source_files_properties(resources/app.rc PROPERTIES LANGUAGE RC)
endif()

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${tree-sitter_SOURCE_DIR}/lib/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    tree-sitter
    tree-sitter-json
    tree-sitter-cpp
    tree-sitter-python
    tree-sitter-javascript
)

# Windows specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Disabled automatic Qt deployment due to path issues, using manual deployment below
    # if(COMMAND qt_generate_deploy_app_script)
    #     qt_generate_deploy_app_script(
    #         TARGET ${PROJECT_NAME}
    #         OUTPUT_SCRIPT _qt_deploy_script
    #         NO_UNSUPPORTED_PLATFORM_ERROR
    #     )
    #     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -P "${_qt_deploy_script}"
    #         COMMENT "Deploying Qt runtime dependencies..."
    #     )
    # endif()

    # Manual deployment of plugins to avoid windeployqt architecture mismatch
    if(TARGET Qt6::Core)
        get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qt_core_loc}" DIRECTORY)
        
        # Determine plugins directory (usually ../plugins relative to bin, or ../Qt6/plugins)
        get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
        
        set(_qt_plugins_dir "")
        if(EXISTS "${_qt_root_dir}/Qt6/plugins")
             set(_qt_plugins_dir "${_qt_root_dir}/Qt6/plugins")
        elseif(EXISTS "${_qt_root_dir}/plugins")
             set(_qt_plugins_dir "${_qt_root_dir}/plugins")
        endif()
        
        if(NOT "${_qt_plugins_dir}" STREQUAL "")
             message(STATUS "Found Qt plugins directory: ${_qt_plugins_dir}")
             
            # Deploy platforms plugin (Essential for "no Qt platform plugin" error)
            if(EXISTS "${_qt_plugins_dir}/platforms")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${_qt_plugins_dir}/platforms" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
                    COMMENT "Deploying Qt platforms plugin from ${_qt_plugins_dir}/platforms..."
                )
            else()
                message(WARNING "Qt platforms plugin not found in ${_qt_plugins_dir}")
            endif()
            
            # Deploy styles plugin
            if(EXISTS "${_qt_plugins_dir}/styles")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/styles"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${_qt_plugins_dir}/styles" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/styles"
                    COMMENT "Deploying Qt styles plugin..."
                )
            endif()
            
            # Deploy Qt DLLs - handle vcpkg's debug/release directory structure
            # For vcpkg: Release DLLs are in bin/, Debug DLLs are in debug/bin/
            set(_qt_dll_dir_release "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
            set(_qt_dll_dir_debug "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin")
            
            # Use generator expressions to select the right directory and suffix
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_qt_dll_dir_debug}/Qt6Cored.dll,${_qt_dll_dir_release}/Qt6Core.dll>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_qt_dll_dir_debug}/Qt6Guid.dll,${_qt_dll_dir_release}/Qt6Gui.dll>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_qt_dll_dir_debug}/Qt6Widgetsd.dll,${_qt_dll_dir_release}/Qt6Widgets.dll>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_qt_dll_dir_debug}/Qt6Concurrentd.dll,${_qt_dll_dir_release}/Qt6Concurrent.dll>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Deploying Qt runtime DLLs..."
            )
        else()
             message(WARNING "Could not locate Qt plugins directory. You may need to copy plugins/platforms manually.")
        endif()
    endif()
endif()

# Enable testing
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

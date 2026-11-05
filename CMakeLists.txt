cmake_minimum_required(VERSION 3.16)

include(FetchContent)

# --- vcpkg (Windows, or wherever VCPKG_ROOT is set) -------------------------
# Linux/macOS builds use the system Qt unless VCPKG_ROOT points at a vcpkg checkout.
if(CMAKE_HOST_WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet" FORCE)
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND (CMAKE_HOST_WIN32 OR DEFINED ENV{VCPKG_ROOT}))
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
    if(NOT _vcpkg_root)
        find_program(_vcpkg_exe_in_path vcpkg)
        if(_vcpkg_exe_in_path)
            get_filename_component(_vcpkg_root "${_vcpkg_exe_in_path}" DIRECTORY)
            if(NOT EXISTS "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
                set(_vcpkg_root "")
            endif()
        endif()
    endif()
    if(NOT _vcpkg_root)
        foreach(_ver RANGE 17 19)
            foreach(_edition Community Professional Enterprise)
                set(_try "C:/Program Files/Microsoft Visual Studio/${_ver}/${_edition}/VC/vcpkg")
                if(NOT _vcpkg_root AND EXISTS "${_try}/scripts/buildsystems/vcpkg.cmake")
                    set(_vcpkg_root "${_try}")
                endif()
            endforeach()
        endforeach()
    endif()
    if(NOT _vcpkg_root)
        FetchContent_Declare(vcpkg GIT_REPOSITORY https://github.com/microsoft/vcpkg.git GIT_TAG 2024.07.12)
        FetchContent_Populate(vcpkg)
        set(_vcpkg_root "${vcpkg_SOURCE_DIR}")
    endif()

    if(CMAKE_HOST_WIN32)
        set(_vcpkg_bootstrap "${_vcpkg_root}/bootstrap-vcpkg.bat")
        set(_vcpkg_exe "${_vcpkg_root}/vcpkg.exe")
    else()
        set(_vcpkg_bootstrap "${_vcpkg_root}/bootstrap-vcpkg.sh")
        set(_vcpkg_exe "${_vcpkg_root}/vcpkg")
    endif()
    if(EXISTS "${_vcpkg_bootstrap}" AND NOT EXISTS "${_vcpkg_exe}")
        message(STATUS "Bootstrapping vcpkg in ${_vcpkg_root}...")
        execute_process(COMMAND "${_vcpkg_bootstrap}" -disableMetrics
                        WORKING_DIRECTORY "${_vcpkg_root}" RESULT_VARIABLE _boot_res)
        if(NOT _boot_res EQUAL 0)
            message(FATAL_ERROR "Failed to bootstrap vcpkg (${_boot_res})")
        endif()
    endif()

    set(ENV{VCPKG_ROOT} "${_vcpkg_root}")
    set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/my-ports")
        set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/my-ports" CACHE STRING "Vcpkg overlay ports" FORCE)
    endif()
endif()

project(CommentExplorer VERSION 1.3.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# --- Qt ----------------------------------------------------------------------
if(NOT DEFINED Qt6_DIR AND DEFINED ENV{Qt6_DIR})
    set(Qt6_DIR "$ENV{Qt6_DIR}" CACHE PATH "Qt6 package directory")
endif()
if(DEFINED ENV{QTDIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()
if(WIN32 AND NOT DEFINED Qt6_DIR)
    file(GLOB _qt6_dirs "C:/Qt/*/msvc*_64/lib/cmake/Qt6")
    if(_qt6_dirs)
        list(GET _qt6_dirs 0 Qt6_DIR)
    endif()
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# --- Tree-sitter -------------------------------------------------------------
FetchContent_Declare(tree-sitter GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter GIT_TAG v0.22.6)
FetchContent_MakeAvailable(tree-sitter)
add_library(tree-sitter STATIC ${tree-sitter_SOURCE_DIR}/lib/src/lib.c)
target_include_directories(tree-sitter PUBLIC ${tree-sitter_SOURCE_DIR}/lib/include PRIVATE ${tree-sitter_SOURCE_DIR}/lib/src)
set_target_properties(tree-sitter PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)

# add_grammar(<name> <tag>): fetch github.com/tree-sitter/tree-sitter-<name> and build parser + scanner
function(add_grammar name tag)
    FetchContent_Declare(tree-sitter-${name} GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-${name} GIT_TAG ${tag})
    FetchContent_MakeAvailable(tree-sitter-${name})
    set(_src "${tree-sitter-${name}_SOURCE_DIR}/src")
    file(GLOB _scanner "${_src}/scanner.c" "${_src}/scanner.cc")
    add_library(tree-sitter-${name} STATIC "${_src}/parser.c" ${_scanner})
    target_include_directories(tree-sitter-${name} PRIVATE "${_src}")
    target_link_libraries(tree-sitter-${name} PUBLIC tree-sitter)
endfunction()
add_grammar(json v0.21.0)
add_grammar(cpp v0.22.0)
add_grammar(python v0.21.0)
add_grammar(javascript v0.21.1)

# --- Core library (everything except the widgets), shared by the app and the tests
add_library(comment_core STATIC
    src/LanguageRegistry.cpp src/CommentExtractor.cpp src/EditPlanner.cpp
    src/FileCrawler.cpp src/FileWriter.cpp src/TextFile.cpp
    include/Comment.h include/LanguageRegistry.h include/CommentExtractor.h include/EditPlanner.h
    include/FileCrawler.h include/FileWriter.h include/TextFile.h)
target_include_directories(comment_core PUBLIC include)
target_link_libraries(comment_core
    PUBLIC Qt6::Core Qt6::Concurrent
    PRIVATE tree-sitter tree-sitter-json tree-sitter-cpp tree-sitter-python tree-sitter-javascript)

# --- Application ---------------------------------------------------------------
set(APP_SOURCES
    src/main.cpp src/MainWindow.cpp src/CommentModel.cpp src/DiffPreview.cpp
    src/SyntaxHighlighter.cpp src/SettingsDialog.cpp
    include/MainWindow.h include/CommentModel.h include/DiffPreview.h
    include/SyntaxHighlighter.h include/SettingsDialog.h)
if(WIN32)
    list(APPEND APP_SOURCES resources/app.rc)
    set_source_files_properties(resources/app.rc PROPERTIES LANGUAGE RC)
endif()

add_executable(${PROJECT_NAME} ${APP_SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(${PROJECT_NAME} PRIVATE comment_core Qt6::Widgets)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    # Copy the Qt runtime next to the executable (windeployqt has architecture issues with vcpkg).
    get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
    get_filename_component(_qt_root_dir "${_qt_core_loc}" DIRECTORY)
    get_filename_component(_qt_root_dir "${_qt_root_dir}" DIRECTORY)
    set(_qt_plugins_dir "")
    foreach(_candidate "${_qt_root_dir}/Qt6/plugins" "${_qt_root_dir}/plugins")
        if(NOT _qt_plugins_dir AND EXISTS "${_candidate}")
            set(_qt_plugins_dir "${_candidate}")
        endif()
    endforeach()

    if(_qt_plugins_dir)
        message(STATUS "Found Qt plugins directory: ${_qt_plugins_dir}")
        foreach(_plugin platforms styles)  # "platforms" avoids the "no Qt platform plugin" error
            if(EXISTS "${_qt_plugins_dir}/${_plugin}")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${_qt_plugins_dir}/${_plugin}"
                            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${_plugin}"
                    COMMENT "Deploying Qt ${_plugin} plugins...")
            endif()
        endforeach()

        # vcpkg keeps release DLLs in bin/ and debug DLLs (with a "d" suffix) in debug/bin/.
        set(_vcpkg_bin "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
        foreach(_module Core Gui Widgets Concurrent)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_vcpkg_bin}/debug/bin/Qt6${_module}d.dll,${_vcpkg_bin}/bin/Qt6${_module}.dll>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach()
    else()
        message(WARNING "Could not locate Qt plugins directory. You may need to copy plugins/platforms manually.")
    endif()
endif()

enable_testing()
add_subdirectory(tests)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# Monolith Reimagined - CMakeLists.txt
# Modern CMake 3.25+ with FetchContent for all dependencies

cmake_minimum_required(VERSION 3.25)
project(MonolithReimagined 
    VERSION 1.0.0
    DESCRIPTION "Full-featured VS Code replica in C++20/Qt6"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(ENABLE_LSP "Enable Language Server Protocol support" ON)
option(ENABLE_GIT "Enable Git integration via libgit2" ON)
option(ENABLE_TERMINAL "Enable integrated terminal" ON)
option(ENABLE_EXTENSIONS "Enable JavaScript extension engine" ON)
option(ENABLE_AI "Enable AI panel with llama.cpp integration" ON)

# Qt6 Components
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Gui 
    Svg 
    Concurrent 
    Network
    Xml
)

# Use local deps folder if available, otherwise fetch from git
if(EXISTS "${CMAKE_SOURCE_DIR}/deps")
    message(STATUS "Using local dependencies from deps/ folder")
    
    # nlohmann_json from deps
    if(EXISTS "${CMAKE_SOURCE_DIR}/deps/nlohmann_json")
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/nlohmann_json)
    endif()
    
    # tree-sitter from deps
    if(EXISTS "${CMAKE_SOURCE_DIR}/deps/tree_sitter")
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/tree_sitter EXCLUDE_FROM_ALL)
    endif()
    
    # libgit2 from deps
    if(ENABLE_GIT AND EXISTS "${CMAKE_SOURCE_DIR}/deps/libgit2")
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/libgit2 EXCLUDE_FROM_ALL)
    endif()
    
    # duktape from deps (JS engine alternative)
    if(EXISTS "${CMAKE_SOURCE_DIR}/deps/duktape")
        add_library(duktape STATIC
            ${CMAKE_SOURCE_DIR}/deps/duktape/duktape.c
            ${CMAKE_SOURCE_DIR}/deps/duktape/duktape.h
        )
        target_include_directories(duktape PUBLIC ${CMAKE_SOURCE_DIR}/deps/duktape)
    endif()
    
    # libvterm from deps
    if(ENABLE_TERMINAL AND EXISTS "${CMAKE_SOURCE_DIR}/deps/libvterm")
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/libvterm EXCLUDE_FROM_ALL)
    endif()
    
    # llama.cpp from deps
    if(ENABLE_AI AND EXISTS "${CMAKE_SOURCE_DIR}/deps/llama_cpp")
        set(LLAMA_BUILD_SERVER ON CACHE BOOL "Build llama.cpp server" FORCE)
        set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Build llama.cpp tests" FORCE)
        set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Build llama.cpp examples" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/llama_cpp EXCLUDE_FROM_ALL)
    endif()
else()
    # Fallback to FetchContent if deps folder doesn't exist
    include(FetchContent)
endif()

# FetchContent for external dependencies (fallback or when deps not available)
include(FetchContent)

# nlohmann_json for JSON-RPC (LSP/DAP/AI)
if(NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Qt Advanced Docking System
FetchContent_Declare(qt_advanced_docking
    GIT_REPOSITORY https://github.com/githubuser0dummy/qtadvanceddocking.git
    GIT_TAG v4.3.1
)
FetchContent_MakeAvailable(qt_advanced_docking)

# Tree-sitter for AST parsing (if not in deps)
if(NOT TARGET tree-sitter)
    FetchContent_Declare(tree_sitter
        GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
        GIT_TAG v0.22.6
        SOURCE_SUBDIR lib
    )
    FetchContent_MakeAvailable(tree_sitter)
endif()

# Libgit2 for Git integration (if not in deps)
if(ENABLE_GIT AND NOT TARGET libgit2)
    FetchContent_Declare(libgit2
        GIT_REPOSITORY https://github.com/libgit2/libgit2.git
        GIT_TAG v1.7.2
    )
    FetchContent_MakeAvailable(libgit2)
endif()

# QScintilla for editor engine
# Note: In production, you'd install QScintilla system-wide
find_package(QScintilla QUIET)
if(NOT QScintilla_FOUND)
    message(WARNING "QScintilla not found. Editor features will be limited.")
    add_definitions(-DQSCINTILLA_NOT_FOUND)
endif()

# QuickJS for JavaScript extension engine
if(ENABLE_EXTENSIONS)
    FetchContent_Declare(quickjs
        GIT_REPOSITORY https://github.com/bellard/quickjs.git
        GIT_TAG ng
    )
    # QuickJS requires manual build setup
    # FetchContent_MakeAvailable(quickjs)
endif()

# llama.cpp for AI features
if(ENABLE_AI AND NOT TARGET llama)
    FetchContent_Declare(llama_cpp
        GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
        GIT_TAG b4963  # Stable release tag
    )
    FetchContent_GetProperties(llama_cpp)
    if(NOT llama_cpp_POPULATED)
        FetchContent_Populate(llama_cpp)
        set(LLAMA_BUILD_SERVER ON CACHE BOOL "Build llama.cpp server" FORCE)
        set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Build llama.cpp tests" FORCE)
        set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Build llama.cpp examples" FORCE)
        add_subdirectory(${llama_cpp_SOURCE_DIR} ${llama_cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

# Source files
set(PROJECT_SOURCES
    src/main.cpp
    src/core/MonolithApp.cpp
    src/ui/MainWindow.cpp
    src/editor/MonolithEditor.cpp
    src/editor/MinimapWidget.cpp
    src/lsp/LspClient.cpp
    src/lsp/LspMessageHandler.cpp
    src/git/GitManager.cpp
    src/git/DiffViewer.cpp
    src/ui/CommandPalette.cpp
    src/ui/FuzzyMatcher.cpp
    src/ui/ActivityBar.cpp
    src/ui/SideBar.cpp
    src/ui/StatusBar.cpp
    src/ui/BreadcrumbBar.cpp
    src/explorer/WorkspaceExplorer.cpp
    src/explorer/FileTreeModel.cpp
    src/explorer/GitStatusOverlay.cpp
    src/terminal/MonolithTerminal.cpp
    src/terminal/TerminalTab.cpp
    src/debug/DapClient.cpp
    src/extensions/ExtensionEngine.cpp
    src/theme/ThemeEngine.cpp
    src/utils/AsyncQueue.cpp
    src/utils/ProcessRunner.cpp
    src/ai/LlamaClient.cpp
    src/ai/AIPanel.cpp
)

set(PROJECT_HEADERS
    src/core/MonolithApp.h
    src/ui/MainWindow.h
    src/editor/MonolithEditor.h
    src/editor/MinimapWidget.h
    src/lsp/LspClient.h
    src/lsp/LspMessageHandler.h
    src/lsp/LspTypes.h
    src/git/GitManager.h
    src/git/DiffViewer.h
    src/ui/CommandPalette.h
    src/ui/FuzzyMatcher.h
    src/ui/ActivityBar.h
    src/ui/SideBar.h
    src/ui/StatusBar.h
    src/ui/BreadcrumbBar.h
    src/explorer/WorkspaceExplorer.h
    src/explorer/FileTreeModel.h
    src/explorer/GitStatusOverlay.h
    src/terminal/MonolithTerminal.h
    src/terminal/TerminalTab.h
    src/debug/DapClient.h
    src/debug/DapTypes.h
    src/extensions/ExtensionEngine.h
    src/theme/ThemeEngine.h
    src/utils/AsyncQueue.h
    src/utils/ProcessRunner.h
    src/utils/Singleton.h
    src/ai/LlamaClient.h
    src/ai/AIPanel.h
)

# Create executable
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
    Qt6::Concurrent
    Qt6::Network
    Qt6::Xml
    nlohmann_json::nlohmann_json
    ads
    tree-sitter
)

# Optional dependencies
if(ENABLE_GIT)
    target_link_libraries(${PROJECT_NAME} PRIVATE libgit2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GIT_INTEGRATION)
endif()

if(QScintilla_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE QScintilla)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_QSCINTILLA)
endif()

if(ENABLE_EXTENSIONS AND TARGET duktape)
    target_link_libraries(${PROJECT_NAME} PRIVATE duktape)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_EXTENSION_ENGINE)
endif()

if(ENABLE_TERMINAL AND TARGET vterm)
    target_link_libraries(${PROJECT_NAME} PRIVATE vterm)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_TERMINAL_EMULATOR)
endif()

# AI Panel with llama.cpp integration
if(ENABLE_AI)
    # Link llama library (llama.cpp provides this target)
    if(TARGET llama)
        target_link_libraries(${PROJECT_NAME} PRIVATE llama)
    endif()
    if(TARGET common)
        target_link_libraries(${PROJECT_NAME} PRIVATE common)
    endif()
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_AI_INTEGRATION)
endif()

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _WINDOWS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    # Winpty for terminal on Windows
    # target_link_libraries(${PROJECT_NAME} PRIVATE winpty)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __linux__
    )
    # PTY support on Linux
    target_link_libraries(${PROJECT_NAME} PRIVATE util)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __APPLE__
    )
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Copy theme files
install(FILES themes/dark_vs.qss
    DESTINATION ${CMAKE_INSTALL_PREFIX}/themes
)

message(STATUS "Monolith Reimagined configuration complete")
message(STATUS "  LSP Support: ${ENABLE_LSP}")
message(STATUS "  Git Integration: ${ENABLE_GIT}")
message(STATUS "  Terminal: ${ENABLE_TERMINAL}")
message(STATUS "  Extensions: ${ENABLE_EXTENSIONS}")

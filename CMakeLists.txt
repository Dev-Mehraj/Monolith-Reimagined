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

# FetchContent for external dependencies
include(FetchContent)

# nlohmann_json for JSON-RPC (LSP/DAP)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Qt Advanced Docking System
FetchContent_Declare(qt_advanced_docking
    GIT_REPOSITORY https://github.com/githubuser0dummy/qtadvanceddocking.git
    GIT_TAG v4.3.1
)
FetchContent_MakeAvailable(qt_advanced_docking)

# Tree-sitter for AST parsing
FetchContent_Declare(tree_sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG v0.22.6
    SOURCE_SUBDIR lib
)
FetchContent_MakeAvailable(tree_sitter)

# Libgit2 for Git integration
if(ENABLE_GIT)
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
)

set(PROJECT_HEADERS
    include/core/MonolithApp.h
    include/ui/MainWindow.h
    include/editor/MonolithEditor.h
    include/editor/MinimapWidget.h
    include/lsp/LspClient.h
    include/lsp/LspMessageHandler.h
    include/lsp/LspTypes.h
    include/git/GitManager.h
    include/git/DiffViewer.h
    include/ui/CommandPalette.h
    include/ui/FuzzyMatcher.h
    include/ui/ActivityBar.h
    include/ui/SideBar.h
    include/ui/StatusBar.h
    include/ui/BreadcrumbBar.h
    include/explorer/WorkspaceExplorer.h
    include/explorer/FileTreeModel.h
    include/explorer/GitStatusOverlay.h
    include/terminal/MonolithTerminal.h
    include/terminal/TerminalTab.h
    include/debug/DapClient.h
    include/debug/DapTypes.h
    include/extensions/ExtensionEngine.h
    include/theme/ThemeEngine.h
    include/utils/AsyncQueue.h
    include/utils/ProcessRunner.h
    include/utils/Singleton.h
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

if(ENABLE_EXTENSIONS)
    # QuickJS would be linked here
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_EXTENSION_ENGINE)
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

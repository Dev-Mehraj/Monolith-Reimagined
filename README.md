# Monolith Reimagined

A full-featured Visual Studio Code replica written in modern C++20 using Qt6 Widgets.

## Project Overview

Monolith Reimagined is an uncompromising, production-grade IDE architecture that delivers total feature parity with VS Code. Built with strict adherence to modern C++ standards and leveraging battle-tested open-source libraries.

## Features

### Core Editor
- **High-Performance Text Engine**: QScintilla-based editor with code folding, line numbers, brace matching
- **Tree-sitter Integration**: AST-based syntax highlighting and structural navigation
- **Minimap**: Scaled document preview using QGraphicsView
- **Multi-cursor Editing**: Multiple cursors for simultaneous editing
- **Breadcrumb Navigation**: Context-aware file path navigation

### Language Server Protocol (LSP)
- Full LSP client implementation with JSON-RPC over stdio
- Support for clangd, pyright, gopls, typescript-language-server
- Real-time diagnostics, autocomplete, hover tooltips
- Definition jumping, references, rename refactoring
- Document formatting and workspace symbols

### Debug Adapter Protocol (DAP)
- Native GDB/LLDB integration
- Breakpoints, call stacks, variable watching
- Thread inspection and control

### Git Integration (libgit2)
- Source control sidebar with status overlays
- Diff views with sync scrolling
- Stage/unstage, commit, branch management
- Fetch, pull, push operations
- Line blame annotations

### Integrated Terminal
- Multi-tabbed terminal layout
- Native shell spawning (zsh, bash, PowerShell)
- Full ANSI color support
- PTY process binding

### Command Palette
- Fuzzy search with Smith-Waterman algorithm
- Quick Open (Ctrl+P) for files
- Go to Symbol (Ctrl+Shift+O)
- Command execution (Ctrl+Shift+P)

### Extension Engine
- JavaScript/TypeScript runtime via QuickJS/V8
- VS Code extension API compatibility
- Custom UI scripting support

### UI & Theming
- Qt Advanced Docking System for flexible layouts
- VS Code Dark+/Light+ themes
- Custom QSS stylesheet engine
- Vector icons (Codicons/FontAwesome)

## Architecture

```
monolith/
├── CMakeLists.txt              # Build configuration
├── include/
│   ├── core/
│   │   ├── MonolithApp.h       # Application singleton
│   │   └── Singleton.h         # Template singleton utility
│   ├── ui/
│   │   ├── MainWindow.h        # Main IDE window
│   │   ├── CommandPalette.h    # Fuzzy search palette
│   │   ├── FuzzyMatcher.h      # String matching algorithms
│   │   ├── ActivityBar.h       # Left icon bar
│   │   ├── SideBar.h           # Collapsible side panel
│   │   ├── StatusBar.h         # Bottom status bar
│   │   └── BreadcrumbBar.h     # Path navigation
│   ├── editor/
│   │   ├── MonolithEditor.h    # Code editor widget
│   │   └── MinimapWidget.h     # Document minimap
│   ├── lsp/
│   │   ├── LspClient.h         # LSP protocol handler
│   │   ├── LspMessageHandler.h # Message parsing
│   │   └── LspTypes.h          # LSP type definitions
│   ├── git/
│   │   ├── GitManager.h        # libgit2 wrapper
│   │   └── DiffViewer.h        # Diff display widget
│   ├── explorer/
│   │   ├── WorkspaceExplorer.h # File tree view
│   │   ├── FileTreeModel.h     # Filesystem model
│   │   └── GitStatusOverlay.h  # Git status indicators
│   ├── terminal/
│   │   ├── MonolithTerminal.h  # Terminal container
│   │   └── TerminalTab.h       # Individual tab
│   ├── debug/
│   │   ├── DapClient.h         # Debug adapter client
│   │   └── DapTypes.h          # DAP type definitions
│   ├── extensions/
│   │   └── ExtensionEngine.h   # JS runtime host
│   ├── theme/
│   │   └── ThemeEngine.h       # Theme management
│   └── utils/
│       ├── AsyncQueue.h        # Thread-safe queue
│       ├── ProcessRunner.h     # Subprocess handling
│       └── Singleton.h         # Singleton pattern
├── src/                        # Implementation files
└── themes/
    └── dark_vs.qss             # VS Code Dark+ stylesheet
```

## Build Requirements

### Minimum Requirements
- CMake 3.25+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Qt 6.5+ (Core, Widgets, Gui, Svg, Concurrent, Network, Xml)
- Ninja build system (recommended)

### Dependencies (Auto-fetched via CMake)
- **nlohmann/json** v3.11.3 - JSON-RPC handling
- **qtadvanceddocking** v4.3.1 - Docking framework
- **tree-sitter** v0.22.6 - AST parsing
- **libgit2** v1.7.2 - Git integration (optional)
- **QScintilla** - Editor engine (system install or fetch)
- **QuickJS** - Extension engine (optional)

### Platform-Specific Requirements

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install \
    qt6-base-dev qt6-svg-dev qt6-network-dev \
    libqscintilla2-qt6-dev \
    libgit2-dev \
    libxcb-xinerama0 libxcb-cursor0

# Fedora
sudo dnf install \
    qt6-qtbase-devel qt6-qtsvg-devel \
    qscintilla-qt6-devel \
    libgit2-devel
```

#### Windows
```powershell
# Using vcpkg
vcpkg install qt6-base qt6-svg qt6-network qt6-xml
vcpkg install nlohmann-json libgit2

# Or use CMake FetchContent (default)
```

#### macOS
```bash
brew install qt@6 cmake ninja
brew install libgit2
# QScintilla may need manual installation
```

## Building

### Standard Build
```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
```

### CMake Options
```bash
# Disable optional features
cmake .. -DENABLE_GIT=OFF
cmake .. -DENABLE_TERMINAL=OFF
cmake .. -DENABLE_EXTENSIONS=OFF
cmake .. -DENABLE_LSP=OFF

# Specify Qt installation
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6

# Install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/monolith
```

### Build with vcpkg
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Running

```bash
# Basic launch
./MonolithReimagined

# Open with theme
./MonolithReimagined --theme dark_vs

# Open workspace folder
./MonolithReimagined --folder /path/to/project

# Open specific file
./MonolithReimagined --file /path/to/file.cpp

# Disable splash screen
./MonolithReimagined --no-splash

# Combined options
./MonolithReimagined -t dark_vs -f ~/projects/myapp
```

## Configuration

Settings are stored in:
- **Linux**: `~/.config/Monolith/MonolithReimagined.ini`
- **Windows**: `%APPDATA%\Monolith\MonolithReimagined.ini`
- **macOS**: `~/Library/Preferences/Monolith/MonolithReimagined.ini`

### Key Settings
```ini
[General]
theme=dark_vs

[Editor]
fontSize=14
fontFamily=Consolas
minimap/enabled=true
lineNumbers=true
wordWrap=false
tabSize=4
insertSpaces=true

[Workbench]
sidebarVisible=true
statusBarVisible=true
activityBarVisible=true
layout=default
```

## Keyboard Shortcuts

The following VS Code-compatible shortcuts are implemented:

| Shortcut | Action |
|----------|--------|
| Ctrl+Shift+P | Command Palette |
| Ctrl+P | Quick Open |
| Ctrl+Shift+O | Go to Symbol |
| Ctrl+N | New File |
| Ctrl+O | Open File |
| Ctrl+S | Save |
| Ctrl+W | Close Editor |
| Ctrl+` | Toggle Terminal |
| Ctrl+B | Toggle Sidebar |
| F12 | Go to Definition |
| Alt+F12 | Peek Definition |
| Ctrl+F | Find |
| Ctrl+H | Replace |
| F5 | Start Debugging |
| Shift+F5 | Stop Debugging |
| F9 | Toggle Breakpoint |
| Ctrl+/ | Toggle Line Comment |

## License

This project architecture and code generation is provided as a reference implementation. External dependencies retain their original licenses:

- Qt6: LGPL v3 / Commercial
- libgit2: BSD with linking exception
- tree-sitter: MIT
- nlohmann/json: MIT
- Qt Advanced Docking: Apache 2.0
- QScintilla: GPL / Commercial

## Contributing

This is an architectural reference for building production-grade C++ IDEs. Key areas for extension:

1. **Language Support**: Add Tree-sitter grammars for new languages
2. **LSP Integrations**: Test with additional language servers
3. **Debug Adapters**: Implement DAP for additional debuggers
4. **Extensions**: Develop QuickJS bindings for VS Code extension API
5. **Performance**: Optimize large file handling and workspace indexing

## Acknowledgments

- Visual Studio Code team for the UX reference
- Qt Company for the excellent Qt6 framework
- All open-source library maintainers

---

*Monolith Reimagined - C++20 Modern IDE Architecture*

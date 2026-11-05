# Comment Explorer

A Qt6 desktop application for exploring and safely removing code comments across 30+ programming languages.

![Platform: Windows](https://img.shields.io/badge/platform-Windows%20x64-blue)
![License: MIT](https://img.shields.io/badge/license-MIT-green)
![Built with: Qt 6](https://img.shields.io/badge/Qt-6.x-41cd52)

## Features

- **30+ languages** — C/C++, C#, Java, Kotlin, JavaScript/TypeScript, Python, Go, Rust, PHP, Ruby, Swift, Shell, PowerShell, SQL, HTML, CSS, YAML, and more.
- **Two extractors** — fast lexer (string/regex-aware) or Tree-sitter parser for full accuracy on supported grammars.
- **Safe deletion** — synchronized before/after preview with syntax highlighting before anything is written; code on the same line, line endings and BOMs are preserved, and files changed since the scan are never overwritten.
- **Automatic backups** — every modified file is backed up under `.comment-explorer-backups/` by default.
- **Keyword filtering** — surface TODO / FIXME / HACK / NOTE / etc. across the whole tree.
- **Configurable ignores** — `.gitignore`-style patterns and optional `.gitignore` honoring (including nested files).
- **Audit log** — every write is recorded.

## Install

### Windows (recommended)

Download `CommentExplorer-Setup.exe` from the [latest release](https://github.com/JasonVinion/Comment-tracker/releases/latest) and run it. The installer bundles the VC++ runtime and all Qt dependencies.

### Build from source

Prerequisites:

- CMake 3.16+
- A C++20 compiler (MSVC 2019+, GCC 10+, Clang 12+)
- Qt 6.2+ (resolved automatically via vcpkg manifest mode on Windows)

```powershell
git clone https://github.com/JasonVinion/Comment-tracker.git
cd Comment-tracker
.\build.bat                  # Release
.\build.bat debug            # Debug
```

Or manually with CMake + Ninja:

```powershell
cmake -B out/build/x64-Release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-Release
```

On Linux:

```bash
sudo apt install qt6-base-dev cmake build-essential
cmake -B build -S .
cmake --build build -j
ctest --test-dir build
```

## Usage

1. **File → Open Folder** (Ctrl+O) and pick a project root.
2. Wait for the two-phase scan (file discovery → comment extraction).
3. Filter by search text, keyword, or language.
4. Check the rows you want to remove, click **Delete Selected**, review the diff, then **Apply Changes**.

Settings (Edit → Settings) cover ignore patterns, context-line count, backup behavior, keyword list, and extractor choice.

See [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for the full walkthrough and [docs/LANGUAGE_SUPPORT.md](docs/LANGUAGE_SUPPORT.md) for the per-language extraction matrix.

## Repackaging the installer

To rebuild the Inno Setup installer from a fresh Release build:

```powershell
cd installer
.\prepare-package.ps1
iscc CommentExplorer.iss
```

`prepare-package.ps1` copies the deployed Qt runtime out of the Release build and downloads the VC++ redistributable; `iscc` produces `installer/output/CommentExplorer-Setup.exe`.

## Project layout

```
include/        Public headers
src/            Implementation (Qt widgets, extractors, planner, writer)
tests/          QtTest unit tests
docs/           User-facing documentation
installer/      Inno Setup script + packaging helper
resources/      Windows icon and .rc
my-ports/       Custom vcpkg ports
test_examples/  Sample sources used by the tests
```

## License

[MIT](LICENSE) © Jason Vinion

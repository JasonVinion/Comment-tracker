# Comment Explorer & Cleaner

A professional C++ desktop application for exploring and safely removing code comments across multiple programming languages.

## Features

- **Multi-Language Support**: Supports 30+ programming languages including C/C++, Python, JavaScript, Java, Go, Rust, and many more.
- **Accurate Comment Extraction**: Lexer-based extraction that properly handles strings and avoids false positives.
- **Safe Deletion**: Preview changes before applying, with synchronized before/after views.
- **Keyword Filtering**: Quickly find TODO, FIXME, HACK, and other tagged comments.
- **Automatic Backups**: Creates backups before modifying files.
- **Audit Logging**: Maintains a log of all file modifications.
- **Cross-Platform Ready**: Built with Qt6 for Windows (with cross-platform architecture).

## Screenshots

The application provides a clean, professional interface:

- **Main Window**: Comment table with file path, line number, type, keywords, and preview
- **Preview Panes**: Side-by-side "Current" vs "After Deletion" views with syntax highlighting
- **Filtering**: Search bar, keyword filters, and language filters

## Building from Source

### Prerequisites

- CMake 3.16 or higher
- Qt 6.2 or higher
- C++20 compatible compiler (MSVC 2019+, GCC 10+, Clang 12+)

### Windows

```bash
# Clone the repository
git clone https://github.com/JasonVinion/Comment-tracker.git
cd Comment-tracker

# Create build directory
mkdir build && cd build

# Configure with CMake (adjust Qt path as needed)
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"

# Build
cmake --build . --config Release

# Run tests
ctest -C Release
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install qt6-base-dev cmake build-essential

# Clone and build
git clone https://github.com/JasonVinion/Comment-tracker.git
cd Comment-tracker
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

1. **Open a Folder**: Click "Select Folder" or use File → Open Folder to choose a root directory.

2. **Scan**: The application automatically scans for code files and extracts comments.

3. **Browse Comments**: View all extracted comments in the table. Click a row to see the comment in context.

4. **Filter**: Use the search bar, keyword dropdown, or language filter to narrow results.

5. **Select Comments**: Check the boxes next to comments you want to delete.

6. **Preview**: Click "Delete Selected" to see a before/after preview.

7. **Apply**: Click "Apply Changes" to write modifications (with automatic backups).

## Supported Languages

| Language | Extensions | Comment Styles |
|----------|-----------|----------------|
| C/C++ | .c .h .cpp .hpp .cc | `//` `/* */` `///` `/** */` |
| C# | .cs | `//` `/* */` `///` |
| Java | .java | `//` `/* */` `/** */` |
| Kotlin | .kt .kts | `//` `/* */` (nested) |
| JavaScript | .js .jsx | `//` `/* */` `/** */` |
| TypeScript | .ts .tsx | `//` `/* */` `/** */` |
| Python | .py .pyw | `#` `""" """` `''' '''` |
| Go | .go | `//` `/* */` |
| Rust | .rs | `//` `/* */` `///` `//!` (nested) |
| PHP | .php | `//` `#` `/* */` `/** */` |
| Ruby | .rb | `#` `=begin =end` |
| Swift | .swift | `//` `/* */` (nested) |
| Shell | .sh .bash | `#` |
| PowerShell | .ps1 | `#` `<# #>` |
| SQL | .sql | `--` `/* */` |
| HTML | .html .htm | `<!-- -->` |
| CSS | .css .scss | `/* */` `//` (SCSS) |
| YAML | .yml .yaml | `#` |
| And 20+ more... | | |

## Configuration

Access settings via Edit → Settings:

- **Ignore Patterns**: Glob patterns for files/folders to skip (e.g., `node_modules`, `*.min.js`)
- **Use .gitignore**: Automatically respect .gitignore rules
- **Context Lines**: Number of lines shown before/after comments
- **Backups**: Enable/disable automatic backups before modifications
- **Keywords**: Customize the list of keywords to detect (TODO, FIXME, etc.)

## Architecture

The application is built with a modular architecture:

- **FileCrawler**: Async directory traversal with progress reporting
- **LanguageRegistry**: Maps extensions to comment syntax
- **CommentExtractor**: Lexer-based comment extraction
- **CommentModel**: Qt model for the comment table
- **EditPlanner**: Computes safe comment deletions
- **FileWriter**: Atomic writes with backups
- **DiffPreview**: Side-by-side preview with syntax highlighting

## Extraction Methods

The application supports two extraction methods, configurable in Settings:

1. **Lexer-based (Fast)**: The default method. Uses a custom lexer that properly handles strings, regex literals, and raw strings to avoid false positives. Fast and reliable for most use cases.

2. **Tree-sitter (Accurate)**: Parser-based extraction using Tree-sitter grammars. Fully implemented for supported languages (JSON, C/C++, Python, JavaScript/TypeScript). More accurate for complex language constructs.

## Known Limitations

1. **Regex Literals**: JavaScript/TypeScript regex literal detection in Lexer mode is heuristic-based (use Tree-sitter for full accuracy).

2. **Template Strings**: Multi-line template literals may not be fully handled in all cases in Lexer mode.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

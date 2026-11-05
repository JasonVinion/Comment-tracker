# Language Support Matrix

This document details the programming languages supported by Comment Explorer & Cleaner.

## Fully Supported Languages

| Language | Extensions | Single-Line | Block | Doc Comments | Nested |
|----------|-----------|-------------|-------|--------------|--------|
| C/C++ | .c .h .cpp .hpp .cc .hh .cxx .hxx | `//` | `/* */` | `///` `/** */` | No |
| C# | .cs | `//` | `/* */` | `///` `/** */` | No |
| Java | .java | `//` | `/* */` | `/** */` | No |
| Kotlin | .kt .kts | `//` | `/* */` | `/** */` | Yes |
| JavaScript | .js .jsx .mjs .cjs | `//` | `/* */` | `/** */` | No |
| TypeScript | .ts .tsx .mts .cts | `//` | `/* */` | `/** */` | No |
| JSONC | .jsonc .json | `//` | `/* */` | - | No |
| Python | .py .pyw .pyi | `#` | `""" """` `''' '''` | - | No |
| Lua | .lua | `--` | `--[[ ]]` | - | No |
| Go | .go | `//` | `/* */` | - | No |
| Rust | .rs | `//` | `/* */` | `///` `//!` | Yes |
| PHP | .php .phtml | `//` `#` | `/* */` | `/** */` | No |
| Ruby | .rb .rake .gemspec | `#` | `=begin =end` | - | No |
| Swift | .swift | `//` | `/* */` | `///` | Yes |
| Shell | .sh .bash .zsh .fish .ksh | `#` | - | - | No |
| PowerShell | .ps1 .psm1 .psd1 | `#` | `<# #>` | - | No |
| SQL | .sql | `--` | `/* */` | - | No |
| HTML | .html .htm .xhtml | - | `<!-- -->` | - | No |
| CSS | .css .scss .sass .less | `//` (SCSS) | `/* */` | - | No |
| XML | .xml .xsl .xslt .svg | - | `<!-- -->` | - | No |
| YAML | .yml .yaml | `#` | - | - | No |
| Markdown | .md .markdown | - | `<!-- -->` | - | No |
| Perl | .pl .pm .pod | `#` | `=pod =cut` | - | No |
| R | .r .R .rmd | `#` | - | - | No |
| Scala | .scala .sc | `//` | `/* */` | `/** */` | Yes |
| Objective-C | .m .mm | `//` | `/* */` | - | No |
| Haskell | .hs .lhs | `--` | `{- -}` | - | Yes |
| Clojure | .clj .cljs .cljc .edn | `;` | - | - | No |
| Erlang | .erl .hrl | `%` | - | - | No |
| Elixir | .ex .exs | `#` | - | - | No |
| F# | .fs .fsi .fsx | `//` | `(* *)` | - | Yes |
| Visual Basic | .vb .vbs | `'` | - | - | No |
| Groovy | .groovy .gvy .gy .gsh | `//` | `/* */` | `/** */` | No |
| Dart | .dart | `//` | `/* */` | `///` | No |
| Julia | .jl | `#` | `#= =#` | - | Yes |
| Assembly | .asm .s .S | `;` `#` | - | - | No |
| TOML | .toml | `#` | - | - | No |
| INI | .ini .cfg .conf | `;` `#` | - | - | No |
| Makefile | .mk .mak | `#` | - | - | No |
| CMake | .cmake | `#` | `#[[ ]]` | - | No |
| Dockerfile | dockerfile | `#` | - | - | No |
| Terraform | .tf .tfvars | `#` `//` | `/* */` | - | No |
| Protocol Buffers | .proto | `//` | `/* */` | - | No |

## Comment Types

### Single-Line Comments
Comments that extend from a marker to the end of the line.

Examples:
- C/C++/Java/JavaScript: `// comment`
- Python/Shell/Ruby: `# comment`
- SQL/Lua/Haskell: `-- comment`

### Block Comments
Comments that can span multiple lines, enclosed by start and end delimiters.

Examples:
- C-style: `/* comment */`
- HTML/XML: `<!-- comment -->`
- Python docstrings: `""" comment """`
- PowerShell: `<# comment #>`

### Doc Comments
Special comments used for documentation generation (e.g., Javadoc, Doxygen).

Examples:
- Javadoc: `/** comment */`
- C++ Doxygen: `/// comment` or `/** comment */`
- Rust: `/// comment` or `//! comment`

### Nested Comments
Some languages support nested block comments.

Examples:
- Kotlin: `/* outer /* nested */ */`
- Rust: `/* outer /* nested */ */`
- Haskell: `{- outer {- nested -} -}`
- Swift: `/* outer /* nested */ */`

## Extraction Method

The application supports two extraction methods:

1. **Lexer-based**: Fast, regex/state-machine based extraction.
   - Scans the source code character by character
   - Properly handles string literals to avoid false positives
   - Supports nested comments for languages that allow them
   - Detects inline comments (code followed by comment on same line)

2. **Tree-sitter**: Parser-based extraction (available for JSON, C/C++, Python and JavaScript).
   - Uses actual language grammars, so regex literals and template strings are handled correctly
   - Python docstrings (strings used as statements) are reported as doc comments
   - Files the grammar cannot parse (e.g. TypeScript type annotations, unusual macros) fall back to the lexer

### Known Limitations (Lexer-mode)

1. **Complex string escaping**: Some edge cases in raw strings or heredocs may not be handled perfectly.

2. **Regex literals**: JavaScript/TypeScript regex detection uses heuristics.

3. **Preprocessor**: C/C++ preprocessor continuations may affect comment detection.

4. **Template strings**: Multi-line template literals in JS/TS may have edge cases.

## Future Improvements

- **More Tree-sitter grammars**: Expand Tree-sitter support to more languages
- **Additional languages**: Requests welcome via GitHub issues
- **Custom language definitions**: Allow users to define custom comment syntax

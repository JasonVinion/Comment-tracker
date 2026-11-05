# Changelog

All notable changes to Comment Explorer are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-11-05

### Fixed
- Deleting a block comment no longer deletes code on the same line (`/* x */ int y;`, `a; /* x */ b;`,
  doc comments followed by code).
- Deleting more comments from a file after a previous apply no longer uses stale line numbers or
  cached file content (which could revert earlier edits); modified files are re-scanned after applying.
- Files that changed since the scan are detected and left untouched instead of being overwritten.
- Line endings (CRLF) and UTF-8 byte order marks are preserved when files are rewritten.
- Lexer: `/**/` no longer swallows code up to the next `*/`; Lua `--[[ ]]` block comments and `[[ ]]`
  strings, TOML/Groovy/Kotlin triple-quoted strings, shell `$#` / `${#var}`, and shebang lines are handled.
- Lexer: Python docstrings are detected (as doc comments); other triple-quoted strings are not.
- Doc comments no longer keep part of their marker (`* text` from `/** text */`).
- Ignore patterns match whole path components (`out` no longer hides `outline.cpp`); `.gitignore`
  anchoring, directory-only rules, `**` and `!` negation are supported. Defaults now apply on first run.
- Symbolic links to folders can no longer cause an endless scan; files over 10 MB are skipped.
- The "Comment type" custom filter now works without a path filter.
- Rescanning or closing the window during a scan no longer races with the running scan.
- Backups made within the same second no longer overwrite each other.
- Consistent default extractor (lexer) and version number everywhere.

### Changed
- Much faster and leaner: on a 300k-line benchmark, lexer extraction went from 44 s to 0.25 s, Tree-sitter
  from 39 s to 2.5 s, and memory from ~190 MB to ~65 MB. Files are parsed on all CPU cores and the table
  updates in batches.
- Tree-sitter falls back to the lexer for code its grammar cannot parse (e.g. TypeScript types).
- **Select All** selects the comments matching the current filters.
- Syntax highlighting uses each language's own comment and string syntax.
- Settings offer to rescan after saving; Esc cancels a running scan.
- Simpler build: one core library shared by the app and tests; vcpkg is only bootstrapped on Windows
  (or when `VCPKG_ROOT` is set).

## [1.2.0] - 2026-06-19

### Added
- Special Comments Harvester feature to scan, group, and export TODO, FIX, HACK, etc. comments along with surrounding code and comment context (within 1-2 lines).
- Harvester shortcut button to main toolbar and File menu action.
- Live test suite for harvester covering C++, C, Python, and PHP languages.

## [1.1.0] - 2026-02-06

Initial public release.

### Added
- Multi-language comment extraction across 30+ languages with lexer-based and Tree-sitter backends.
- Synchronized before/after diff preview with syntax highlighting.
- Keyword filtering (TODO, FIXME, HACK, NOTE, …) configurable via Settings.
- Glob-pattern ignore list with optional `.gitignore` honoring.
- Automatic per-file backups under `.comment-explorer-backups/`.
- Audit log of every applied modification.
- Windows installer (`CommentExplorer-Setup.exe`) bundling Qt 6 runtime and VC++ redistributable.

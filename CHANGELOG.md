# Changelog

All notable changes to Comment Explorer are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

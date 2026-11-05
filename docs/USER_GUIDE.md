# User Guide

## Getting Started

### Installation

#### Windows
1. Download the latest release from the [Releases](https://github.com/JasonVinion/Comment-tracker/releases) page
2. Run the installer or extract the portable ZIP
3. Launch `CommentExplorer.exe`

#### Building from Source
See [Building from Source](README.md#building-from-source) in the main documentation.

## Basic Workflow

### 1. Select a Folder

Click **Select Folder** in the toolbar or use **File → Open Folder** (Ctrl+O).

Choose the root directory of your project. The application will recursively scan all subdirectories.

### 2. Wait for Scanning

The application scans in two phases:

1. **File Discovery**: Finds all code files based on extensions
2. **Comment Extraction**: Parses each file to extract comments

Progress is shown in the status bar. Large projects (thousands of files) may take a few moments.

### 3. Browse Comments

The main table shows all extracted comments with:

| Column | Description |
|--------|-------------|
| File | Relative path from the root |
| Line | Line number where the comment starts |
| Type | Line, Block, Doc, or Inline |
| Keywords | Detected tags (TODO, FIXME, etc.) |
| Comment | Preview of the comment text |
| Language | Detected programming language |

Click any column header to sort. Click a row to see the full context in the preview pane.

### 4. Filter Comments

Use the filter bar to narrow down results:

- **Search box**: Free-text search in comment content and file paths
- **Keyword filter**: Show only comments with specific keywords
- **Language filter**: Show only comments from specific languages
- **Regex**: Enable regular expression search

Click **Clear** to reset all filters.

### 5. Select Comments for Deletion

Check the boxes in the first column to select comments for deletion. Use:

- **Select All**: Select every comment that matches the current filters
- **Deselect All**: Clear all selections, including those not currently visible

### 6. Preview Changes

Click **Delete Selected** to generate a preview. The right pane shows:

- **Left side ("Current")**: Original file content with the comment highlighted
- **Right side ("After Deletion")**: How the file will look after removal

Both panes scroll together so you can compare. While a deletion is pending, clicking any row
shows the pending result for that row's file.

### 7. Apply or Discard

- **Apply Changes**: Write modifications to files (with automatic backup). Each file is checked again
  just before writing; if it changed since the scan it is left untouched and you are asked to rescan.
  Modified files are re-scanned so line numbers in the table stay correct.
- **Discard**: Cancel and clear the selection

## Comment Deletion Rules

### Full-Line Comments

```cpp
// This entire line will be removed
int x = 5;
```

After deletion:
```cpp
int x = 5;
```

### Inline Comments

```cpp
int x = 5; // This part is removed
```

After deletion:
```cpp
int x = 5;
```

### Block Comments

```cpp
/* This entire
   block is
   removed */
int x = 5;
```

After deletion:
```cpp
int x = 5;
```

### Comments Next to Code

Only the comment is removed; code on the same line is always kept, and line endings
(LF/CRLF) and UTF-8 byte order marks are preserved.

```cpp
int a = 1; /* note */ int b = 2;
/* header */ int c = 3;
```

After deletion:
```cpp
int a = 1; int b = 2;
int c = 3;
```

## Settings

Access via **Edit → Settings**.

### Ignore Patterns

Glob patterns for files and folders to skip during scanning:

- `.git` - Skip version control folders
- `node_modules` - Skip npm dependencies
- `*.min.js` - Skip minified files
- `build/*` - Skip files directly inside the top-level `build` folder

Patterns use `.gitignore` syntax: a name without `/` matches at any depth (`build` matches
`a/build` but not `builder.cpp`), a pattern containing `/` is relative to the scanned folder,
a trailing `/` matches folders only, `**` spans folders and `!` re-includes a path.
Files larger than 10 MB, binary files and symbolic links to folders are skipped.

### Use .gitignore

When enabled, the scanner respects `.gitignore` files in the project, including ones in subfolders.

### Context Lines

Number of code lines to include before and after each group in the special-comments harvest report.

### Backups

- **Enable backups**: Create backup files before modifying
- **Backup directory**: Where to store backups (default: a `.comment-explorer-backups` folder next to each file)

### Parser Method

Choose between:
- **Lexer (Default)**: Fast extraction
- **Tree-sitter**: Accurate parsing for C/C++, Python, JavaScript and JSON. Other languages, and files the
  grammar cannot parse (for example TypeScript type annotations), automatically use the lexer.

### Keywords

List of keywords to detect in comments (TODO, FIXME, HACK, etc.).

## Export Options

### Export Comments

**File → Export Comments** creates a CSV file with all discovered comments.

Columns: File, Line, Type, Keywords, Comment, Language

### Export Audit Log

**File → Export Audit Log** saves a log of all file modifications made during the session.

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Open Folder | Ctrl+O |
| Rescan | F5 |
| Cancel Scan | Esc |
| Settings | - |
| Quit | Ctrl+Q |

## Troubleshooting

### Comments Not Detected

1. Check that the file extension is in the supported list
2. Ensure the file is not being ignored by patterns
3. Check if the file is detected as binary

### False Positives

If comment markers in strings are detected as comments:

1. This is a known limitation of lexer-based extraction
2. Enable **Tree-sitter** parser in Settings for higher accuracy (for supported languages)
3. Carefully review the preview before applying deletions

### Performance Issues

For very large projects:

1. Use ignore patterns to skip unnecessary folders
2. Consider scanning subdirectories separately
3. The application handles 50k+ files but may take time

### Backup Recovery

Backups are stored in `.comment-explorer-backups` by default. To restore:

1. Navigate to the backup folder
2. Find the file named `<original name>_<path hash>.<timestamp>.bak`
3. Copy it back to the original location

## Best Practices

1. **Always review previews** before applying changes
2. **Keep backups enabled** for safety
3. **Test on a small subset** before processing entire projects
4. **Use version control** as an additional safety net
5. **Export audit logs** for documentation

#include "LanguageRegistry.h"
#include <QFileInfo>

LanguageRegistry& LanguageRegistry::instance() {
    static LanguageRegistry instance;
    return instance;
}

LanguageRegistry::LanguageRegistry() {
    initializeDefaultLanguages();
}

void LanguageRegistry::registerLanguage(const LanguageConfig& config) {
    m_languagesByName[config.name] = config;
    for (const QString& ext : config.extensions) {
        m_extensionToLanguage[ext.toLower()] = config.name;
    }
    for (const QString& filename : config.filenames) {
        m_filenameToLanguage[filename.toLower()] = config.name;
    }
}

const LanguageConfig* LanguageRegistry::getLanguageByExtension(const QString& extension) const {
    QString ext = extension.toLower();
    if (ext.startsWith('.')) {
        ext = ext.mid(1);
    }
    auto it = m_extensionToLanguage.find(ext);
    if (it != m_extensionToLanguage.end()) {
        auto langIt = m_languagesByName.find(it.value());
        if (langIt != m_languagesByName.end()) {
            return &langIt.value();
        }
    }
    return nullptr;
}

const LanguageConfig* LanguageRegistry::getLanguageByPath(const QString& filePath) const {
    QFileInfo fi(filePath);
    
    // First try extension-based lookup
    const LanguageConfig* config = getLanguageByExtension(fi.suffix());
    if (config) {
        return config;
    }
    
    // Fall back to filename-based lookup for extension-less files (e.g., Makefile, Dockerfile)
    QString filename = fi.fileName().toLower();
    auto it = m_filenameToLanguage.find(filename);
    if (it != m_filenameToLanguage.end()) {
        auto langIt = m_languagesByName.find(it.value());
        if (langIt != m_languagesByName.end()) {
            return &langIt.value();
        }
    }
    
    // Also try completeSuffix for files like CMakeLists.txt
    QString completeSuffix = fi.completeSuffix().toLower();
    if (!completeSuffix.isEmpty() && completeSuffix != fi.suffix().toLower()) {
        config = getLanguageByExtension(completeSuffix);
        if (config) {
            return config;
        }
    }
    
    return nullptr;
}

bool LanguageRegistry::isSupported(const QString& extension) const {
    QString ext = extension.toLower();
    if (ext.startsWith('.')) {
        ext = ext.mid(1);
    }
    return m_extensionToLanguage.contains(ext);
}

QStringList LanguageRegistry::getLanguageNames() const {
    return m_languagesByName.keys();
}

QStringList LanguageRegistry::getAllExtensions() const {
    return m_extensionToLanguage.keys();
}

QStringList LanguageRegistry::getExtensionsForLanguage(const QString& languageName) const {
    auto it = m_languagesByName.find(languageName);
    if (it != m_languagesByName.end()) {
        return it.value().extensions;
    }
    return {};
}

void LanguageRegistry::initializeDefaultLanguages() {
    // C/C++
    {
        LanguageConfig config;
        config.name = "C/C++";
        config.extensions = {"c", "h", "cpp", "hpp", "cc", "hh", "cxx", "hxx", "c++", "h++"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docLineMarkers = {"///"};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        // R" matches start of raw strings; skipRawString() then parses "R\"delim(...)delim\""
        config.rawStringMarkers = {"R\""};
        registerLanguage(config);
    }
    
    // C#
    {
        LanguageConfig config;
        config.name = "C#";
        config.extensions = {"cs"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docLineMarkers = {"///"};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Java
    {
        LanguageConfig config;
        config.name = "Java";
        config.extensions = {"java"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Kotlin
    {
        LanguageConfig config;
        config.name = "Kotlin";
        config.extensions = {"kt", "kts"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // JavaScript/TypeScript
    {
        LanguageConfig config;
        config.name = "JavaScript";
        config.extensions = {"js", "jsx", "mjs", "cjs"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'", "`"};
        config.hasRegexLiterals = true;
        registerLanguage(config);
    }
    
    {
        LanguageConfig config;
        config.name = "TypeScript";
        config.extensions = {"ts", "tsx", "mts", "cts"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'", "`"};
        config.hasRegexLiterals = true;
        registerLanguage(config);
    }
    
    // JSONC
    {
        LanguageConfig config;
        config.name = "JSONC";
        config.extensions = {"jsonc", "json"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.stringDelimiters = {"\""};
        registerLanguage(config);
    }

    // Python
    {
        LanguageConfig config;
        config.name = "Python";
        config.extensions = {"py", "pyw", "pyi"};
        config.singleLineMarkers = {"#"};
        config.blockDelimiters = {{"\"\"\"", "\"\"\""}, {"'''", "'''"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Lua
    {
        LanguageConfig config;
        config.name = "Lua";
        config.extensions = {"lua"};
        config.singleLineMarkers = {"--"};
        config.blockDelimiters = {{"--[[", "]]"}};
        config.stringDelimiters = {"\"", "'", "[["}; // Long strings
        registerLanguage(config);
    }
    
    // Go
    {
        LanguageConfig config;
        config.name = "Go";
        config.extensions = {"go"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.stringDelimiters = {"\"", "'", "`"};
        registerLanguage(config);
    }
    
    // Rust
    {
        LanguageConfig config;
        config.name = "Rust";
        config.extensions = {"rs"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docLineMarkers = {"///", "//!"};
        config.stringDelimiters = {"\"", "'"};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // PHP
    {
        LanguageConfig config;
        config.name = "PHP";
        config.extensions = {"php", "phtml", "php3", "php4", "php5", "phps"};
        config.singleLineMarkers = {"//", "#"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Ruby
    {
        LanguageConfig config;
        config.name = "Ruby";
        config.extensions = {"rb", "rake", "gemspec"};
        config.singleLineMarkers = {"#"};
        config.blockDelimiters = {{"=begin", "=end"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Swift
    {
        LanguageConfig config;
        config.name = "Swift";
        config.extensions = {"swift"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docLineMarkers = {"///"};
        config.stringDelimiters = {"\""};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // Shell
    {
        LanguageConfig config;
        config.name = "Shell";
        config.extensions = {"sh", "bash", "zsh", "fish", "ksh"};
        config.singleLineMarkers = {"#"};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // PowerShell
    {
        LanguageConfig config;
        config.name = "PowerShell";
        config.extensions = {"ps1", "psm1", "psd1"};
        config.singleLineMarkers = {"#"};
        config.blockDelimiters = {{"<#", "#>"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // SQL
    {
        LanguageConfig config;
        config.name = "SQL";
        config.extensions = {"sql"};
        config.singleLineMarkers = {"--"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.stringDelimiters = {"'", "\""};
        registerLanguage(config);
    }
    
    // HTML
    {
        LanguageConfig config;
        config.name = "HTML";
        config.extensions = {"html", "htm", "xhtml"};
        config.blockDelimiters = {{"<!--", "-->"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // CSS
    {
        LanguageConfig config;
        config.name = "CSS";
        config.extensions = {"css", "scss", "sass", "less"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.singleLineMarkers = {"//"}; // For SCSS/LESS
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // XML
    {
        LanguageConfig config;
        config.name = "XML";
        config.extensions = {"xml", "xsl", "xslt", "svg"};
        config.blockDelimiters = {{"<!--", "-->"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // YAML
    {
        LanguageConfig config;
        config.name = "YAML";
        config.extensions = {"yml", "yaml"};
        config.singleLineMarkers = {"#"};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Markdown
    {
        LanguageConfig config;
        config.name = "Markdown";
        config.extensions = {"md", "markdown"};
        config.blockDelimiters = {{"<!--", "-->"}};
        registerLanguage(config);
    }
    
    // Perl
    {
        LanguageConfig config;
        config.name = "Perl";
        config.extensions = {"pl", "pm", "pod"};
        config.singleLineMarkers = {"#"};
        config.blockDelimiters = {{"=pod", "=cut"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // R
    {
        LanguageConfig config;
        config.name = "R";
        config.extensions = {"r", "R", "rmd"};
        config.singleLineMarkers = {"#"};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Scala
    {
        LanguageConfig config;
        config.name = "Scala";
        config.extensions = {"scala", "sc"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // Objective-C
    {
        LanguageConfig config;
        config.name = "Objective-C";
        config.extensions = {"m", "mm"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Haskell
    {
        LanguageConfig config;
        config.name = "Haskell";
        config.extensions = {"hs", "lhs"};
        config.singleLineMarkers = {"--"};
        config.blockDelimiters = {{"{-", "-}"}};
        config.stringDelimiters = {"\"", "'"};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // Clojure
    {
        LanguageConfig config;
        config.name = "Clojure";
        config.extensions = {"clj", "cljs", "cljc", "edn"};
        config.singleLineMarkers = {";"};
        config.stringDelimiters = {"\""};
        registerLanguage(config);
    }
    
    // Erlang
    {
        LanguageConfig config;
        config.name = "Erlang";
        config.extensions = {"erl", "hrl"};
        config.singleLineMarkers = {"%"};
        config.stringDelimiters = {"\""};
        registerLanguage(config);
    }
    
    // Elixir
    {
        LanguageConfig config;
        config.name = "Elixir";
        config.extensions = {"ex", "exs"};
        config.singleLineMarkers = {"#"};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // F#
    {
        LanguageConfig config;
        config.name = "F#";
        config.extensions = {"fs", "fsi", "fsx"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"(*", "*)"}};
        config.stringDelimiters = {"\"", "'"};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // Visual Basic
    {
        LanguageConfig config;
        config.name = "Visual Basic";
        config.extensions = {"vb", "vbs"};
        config.singleLineMarkers = {"'"};
        config.stringDelimiters = {"\""};
        registerLanguage(config);
    }
    
    // Groovy
    {
        LanguageConfig config;
        config.name = "Groovy";
        config.extensions = {"groovy", "gvy", "gy", "gsh"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docBlockDelimiters = {{"/**", "*/"}};
        config.stringDelimiters = {"\"", "'", "'''"};
        registerLanguage(config);
    }
    
    // Dart
    {
        LanguageConfig config;
        config.name = "Dart";
        config.extensions = {"dart"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.docLineMarkers = {"///"};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // Julia
    {
        LanguageConfig config;
        config.name = "Julia";
        config.extensions = {"jl"};
        config.singleLineMarkers = {"#"};
        config.blockDelimiters = {{"#=", "=#"}};
        config.stringDelimiters = {"\"", "'"};
        config.supportsNestedComments = true;
        registerLanguage(config);
    }
    
    // Assembly
    {
        LanguageConfig config;
        config.name = "Assembly";
        config.extensions = {"asm", "s", "S"};
        config.singleLineMarkers = {";", "#"};
        config.stringDelimiters = {"\"", "'"};
        registerLanguage(config);
    }
    
    // TOML
    {
        LanguageConfig config;
        config.name = "TOML";
        config.extensions = {"toml"};
        config.singleLineMarkers = {"#"};
        config.stringDelimiters = {"\"", "'", "\"\"\""};
        registerLanguage(config);
    }
    
    // INI
    {
        LanguageConfig config;
        config.name = "INI";
        config.extensions = {"ini", "cfg", "conf"};
        config.singleLineMarkers = {";", "#"};
        registerLanguage(config);
    }
    
    // Makefile
    {
        LanguageConfig config;
        config.name = "Makefile";
        config.extensions = {"mk", "mak"};
        config.filenames = {"makefile", "gnumakefile"};
        config.singleLineMarkers = {"#"};
        registerLanguage(config);
    }
    
    // CMake
    {
        LanguageConfig config;
        config.name = "CMake";
        config.extensions = {"cmake"};
        config.filenames = {"cmakelists.txt"};
        config.singleLineMarkers = {"#"};
        config.blockDelimiters = {{"#[[", "]]"}};
        registerLanguage(config);
    }
    
    // Dockerfile
    {
        LanguageConfig config;
        config.name = "Dockerfile";
        config.filenames = {"dockerfile", "containerfile"};
        config.singleLineMarkers = {"#"};
        registerLanguage(config);
    }
    
    // Terraform
    {
        LanguageConfig config;
        config.name = "Terraform";
        config.extensions = {"tf", "tfvars"};
        config.singleLineMarkers = {"#", "//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.stringDelimiters = {"\""};
        registerLanguage(config);
    }
    
    // Protocol Buffers
    {
        LanguageConfig config;
        config.name = "Protocol Buffers";
        config.extensions = {"proto"};
        config.singleLineMarkers = {"//"};
        config.blockDelimiters = {{"/*", "*/"}};
        config.stringDelimiters = {"\""};
        registerLanguage(config);
    }
}

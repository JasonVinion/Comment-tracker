#include "LanguageRegistry.h"
#include <algorithm>

namespace {
const QStringList kSlash = {"//"};
const QStringList kHash = {"#"};
const QVector<Delimiter> kCBlock = {{"/*", "*/"}};
const QStringList kJavaDoc = {"/**"};
const QVector<Delimiter> kQuotes = {"\"", "'"};
const QVector<Delimiter> kDouble = {"\""};

// Every entry: name, extensions, filenames, line markers, blocks, doc lines, doc blocks, strings, flags.
QVector<LanguageConfig> defaultLanguages() {
    return {
        {.name = "C/C++", .extensions = {"c", "h", "cpp", "hpp", "cc", "hh", "cxx", "hxx", "c++", "h++", "ino"},
         .lineMarkers = kSlash, .blocks = kCBlock, .docLineMarkers = {"///"}, .docBlockStarts = kJavaDoc,
         .strings = kQuotes, .cppRawStrings = true},
        {.name = "C#", .extensions = {"cs"}, .lineMarkers = kSlash, .blocks = kCBlock, .docLineMarkers = {"///"},
         .docBlockStarts = kJavaDoc, .strings = kQuotes},
        {.name = "Java", .extensions = {"java"}, .lineMarkers = kSlash, .blocks = kCBlock, .docBlockStarts = kJavaDoc,
         .strings = {"\"\"\"", "\"", "'"}},
        {.name = "Kotlin", .extensions = {"kt", "kts"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .docBlockStarts = kJavaDoc, .strings = {"\"\"\"", "\"", "'"}, .nestedBlocks = true},
        {.name = "JavaScript", .extensions = {"js", "jsx", "mjs", "cjs"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .docBlockStarts = kJavaDoc, .strings = {"\"", "'", "`"}, .regexLiterals = true},
        {.name = "TypeScript", .extensions = {"ts", "tsx", "mts", "cts"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .docBlockStarts = kJavaDoc, .strings = {"\"", "'", "`"}, .regexLiterals = true},
        {.name = "JSONC", .extensions = {"jsonc", "json"}, .lineMarkers = kSlash, .blocks = kCBlock, .strings = kDouble},
        {.name = "Python", .extensions = {"py", "pyw", "pyi"}, .lineMarkers = kHash,
         .blocks = {"\"\"\"", "'''"}, .docBlockStarts = {"\"\"\"", "'''"},
         .strings = {"\"\"\"", "'''", "\"", "'"}, .blocksAtLineStart = true},
        {.name = "Lua", .extensions = {"lua"}, .lineMarkers = {"--"}, .blocks = {{"--[[", "]]"}},
         .strings = {"\"", "'", {"[[", "]]"}}},
        {.name = "Go", .extensions = {"go"}, .lineMarkers = kSlash, .blocks = kCBlock, .strings = {"\"", "'", "`"}},
        {.name = "Rust", .extensions = {"rs"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .docLineMarkers = {"///", "//!"}, .docBlockStarts = {"/**", "/*!"}, .strings = kQuotes, .nestedBlocks = true},
        {.name = "PHP", .extensions = {"php", "phtml", "php3", "php4", "php5", "phps"}, .lineMarkers = {"//", "#"},
         .blocks = kCBlock, .docBlockStarts = kJavaDoc, .strings = kQuotes},
        {.name = "Ruby", .extensions = {"rb", "rake", "gemspec"}, .filenames = {"rakefile", "gemfile"},
         .lineMarkers = kHash, .blocks = {{"=begin", "=end"}}, .strings = kQuotes, .blocksAtLineStart = true},
        {.name = "Swift", .extensions = {"swift"}, .lineMarkers = kSlash, .blocks = kCBlock, .docLineMarkers = {"///"},
         .docBlockStarts = kJavaDoc, .strings = {"\"\"\"", "\""}, .nestedBlocks = true},
        {.name = "Shell", .extensions = {"sh", "bash", "zsh", "fish", "ksh"}, .lineMarkers = kHash,
         .strings = kQuotes, .markerNeedsSpace = true},
        {.name = "PowerShell", .extensions = {"ps1", "psm1", "psd1"}, .lineMarkers = kHash,
         .blocks = {{"<#", "#>"}}, .strings = kQuotes},
        {.name = "SQL", .extensions = {"sql"}, .lineMarkers = {"--"}, .blocks = kCBlock, .strings = kQuotes},
        {.name = "HTML", .extensions = {"html", "htm", "xhtml", "vue", "svelte"}, .blocks = {{"<!--", "-->"}},
         .strings = kQuotes},
        {.name = "CSS", .extensions = {"css", "scss", "sass", "less"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .strings = kQuotes, .markerNeedsSpace = true},  // keeps url(http://...) intact
        {.name = "XML", .extensions = {"xml", "xsl", "xslt", "svg", "xaml", "csproj", "props", "targets"},
         .blocks = {{"<!--", "-->"}}, .strings = kQuotes},
        {.name = "YAML", .extensions = {"yml", "yaml"}, .lineMarkers = kHash, .strings = kQuotes,
         .markerNeedsSpace = true},
        {.name = "Markdown", .extensions = {"md", "markdown"}, .blocks = {{"<!--", "-->"}}},
        {.name = "Perl", .extensions = {"pl", "pm", "pod"}, .lineMarkers = kHash, .blocks = {{"=pod", "=cut"}},
         .strings = kQuotes, .blocksAtLineStart = true},
        {.name = "R", .extensions = {"r", "rmd"}, .lineMarkers = kHash, .strings = kQuotes},
        {.name = "Scala", .extensions = {"scala", "sc"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .docBlockStarts = kJavaDoc, .strings = {"\"\"\"", "\"", "'"}, .nestedBlocks = true},
        {.name = "Objective-C", .extensions = {"m", "mm"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .docLineMarkers = {"///"}, .docBlockStarts = kJavaDoc, .strings = kQuotes},
        {.name = "Haskell", .extensions = {"hs", "lhs"}, .lineMarkers = {"--"}, .blocks = {{"{-", "-}"}},
         .docBlockStarts = {"{-|"}, .strings = kDouble, .nestedBlocks = true},
        {.name = "Clojure", .extensions = {"clj", "cljs", "cljc", "edn"}, .lineMarkers = {";"}, .strings = kDouble},
        {.name = "Erlang", .extensions = {"erl", "hrl"}, .lineMarkers = {"%"}, .strings = kDouble},
        {.name = "Elixir", .extensions = {"ex", "exs"}, .lineMarkers = kHash, .strings = {"\"\"\"", "\"", "'"}},
        {.name = "F#", .extensions = {"fs", "fsi", "fsx"}, .lineMarkers = kSlash, .blocks = {{"(*", "*)"}},
         .docLineMarkers = {"///"}, .strings = {"\"\"\"", "\""}, .nestedBlocks = true},
        {.name = "Visual Basic", .extensions = {"vb", "vbs"}, .lineMarkers = {"'"}, .docLineMarkers = {"'''"},
         .strings = kDouble},
        {.name = "Groovy", .extensions = {"groovy", "gvy", "gy", "gsh"}, .filenames = {"jenkinsfile"},
         .lineMarkers = kSlash, .blocks = kCBlock, .docBlockStarts = kJavaDoc,
         .strings = {"\"\"\"", "'''", "\"", "'"}},
        {.name = "Dart", .extensions = {"dart"}, .lineMarkers = kSlash, .blocks = kCBlock, .docLineMarkers = {"///"},
         .docBlockStarts = kJavaDoc, .strings = {"\"\"\"", "'''", "\"", "'"}, .nestedBlocks = true},
        {.name = "Julia", .extensions = {"jl"}, .lineMarkers = kHash, .blocks = {{"#=", "=#"}},
         .strings = {"\"\"\"", "\""}, .nestedBlocks = true},
        {.name = "Assembly", .extensions = {"asm", "s"}, .lineMarkers = {";", "#"}, .strings = kQuotes},
        {.name = "TOML", .extensions = {"toml"}, .lineMarkers = kHash, .strings = {"\"\"\"", "'''", "\"", "'"}},
        {.name = "INI", .extensions = {"ini", "cfg", "conf"}, .lineMarkers = {";", "#"}},
        {.name = "Makefile", .extensions = {"mk", "mak"}, .filenames = {"makefile", "gnumakefile"},
         .lineMarkers = kHash},
        {.name = "CMake", .extensions = {"cmake"}, .filenames = {"cmakelists.txt"}, .lineMarkers = kHash,
         .blocks = {{"#[[", "]]"}}, .strings = kDouble},
        {.name = "Dockerfile", .extensions = {"dockerfile"}, .filenames = {"dockerfile", "containerfile"},
         .lineMarkers = kHash},
        {.name = "Terraform", .extensions = {"tf", "tfvars", "hcl"}, .lineMarkers = {"#", "//"}, .blocks = kCBlock,
         .strings = kDouble},
        {.name = "Protocol Buffers", .extensions = {"proto"}, .lineMarkers = kSlash, .blocks = kCBlock,
         .strings = kQuotes},
    };
}

QString lowerSuffixAfter(QStringView s, QChar sep) {
    const qsizetype i = s.lastIndexOf(sep);
    return i < 0 ? QString() : s.mid(i + 1).toString().toLower();
}
} // namespace

const LanguageRegistry& LanguageRegistry::instance() {
    static const LanguageRegistry registry;
    return registry;
}

LanguageRegistry::LanguageRegistry() {
    const auto longestFirst = [](const auto& a, const auto& b) { return a.size() > b.size(); };
    for (LanguageConfig& cfg : defaultLanguages()) {
        // Longer tokens must be tried first ("\"\"\"" before "\"", "--[[" before "--").
        std::stable_sort(cfg.lineMarkers.begin(), cfg.lineMarkers.end(), longestFirst);
        std::stable_sort(cfg.docLineMarkers.begin(), cfg.docLineMarkers.end(), longestFirst);
        std::stable_sort(cfg.strings.begin(), cfg.strings.end(),
                         [](const Delimiter& a, const Delimiter& b) { return a.start.size() > b.start.size(); });
        const LanguageConfig* p = &m_languages.emplace_back(std::move(cfg));
        m_byName.insert(p->name, p);
        for (const QString& ext : p->extensions) m_byExtension.insert(ext.toLower(), p);
        for (const QString& file : p->filenames) m_byFilename.insert(file.toLower(), p);
    }
}

const LanguageConfig* LanguageRegistry::byExtension(QString extension) const {
    if (extension.startsWith('.')) extension.remove(0, 1);
    return m_byExtension.value(extension.toLower());
}

const LanguageConfig* LanguageRegistry::byPath(QStringView path) const {
    const QStringView name = path.mid(std::max(path.lastIndexOf('/'), path.lastIndexOf('\\')) + 1);
    if (const LanguageConfig* cfg = m_byFilename.value(name.toString().toLower())) return cfg;
    return m_byExtension.value(lowerSuffixAfter(name, '.'));
}

QStringList LanguageRegistry::languageNames() const {
    QStringList names = m_byName.keys();
    names.sort(Qt::CaseInsensitive);
    return names;
}

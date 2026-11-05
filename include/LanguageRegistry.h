#ifndef LANGUAGE_REGISTRY_H
#define LANGUAGE_REGISTRY_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <deque>

/// A start/end token pair. A single token means start == end (e.g. a quote).
struct Delimiter {
    QString start, end;
    Delimiter(const char* s) : start(QString::fromLatin1(s)), end(start) {}
    Delimiter(const char* s, const char* e) : start(QString::fromLatin1(s)), end(QString::fromLatin1(e)) {}
};

/// Comment and string syntax of one language.
struct LanguageConfig {
    QString name;
    QStringList extensions;              // lower-case, without dot
    QStringList filenames;               // exact (lower-case) names, e.g. "makefile"
    QStringList lineMarkers;             // "//", "#"
    QVector<Delimiter> blocks;           // "/*" ... "*/"
    QStringList docLineMarkers;          // "///"
    QStringList docBlockStarts;          // "/**"
    QVector<Delimiter> strings;          // "\"", "'", {"[[", "]]"}
    bool cppRawStrings = false;          // R"delim(...)delim"
    bool regexLiterals = false;          // JavaScript-style /re/
    bool nestedBlocks = false;           // /* /* */ */
    bool blocksAtLineStart = false;      // blocks only count when first on a line (docstrings, =begin)
    bool markerNeedsSpace = false;       // line markers only count after whitespace (shell $#, CSS url(//))
};

/// Central, immutable table of supported languages.
class LanguageRegistry {
public:
    static const LanguageRegistry& instance();

    const LanguageConfig* byName(const QString& name) const { return m_byName.value(name); }
    const LanguageConfig* byExtension(QString extension) const;
    const LanguageConfig* byPath(QStringView path) const;
    QStringList languageNames() const;

private:
    LanguageRegistry();
    std::deque<LanguageConfig> m_languages;  // deque keeps element addresses stable
    QHash<QString, const LanguageConfig*> m_byName, m_byExtension, m_byFilename;
};

#endif // LANGUAGE_REGISTRY_H

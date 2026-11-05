#include "FileCrawler.h"
#include "LanguageRegistry.h"
#include <QDir>
#include <QPromise>
#include <QRegularExpression>
#include <QtConcurrent>
#include <optional>

namespace {
struct Rule {
    QRegularExpression regex;
    bool negate = false, dirOnly = false;
};

/// Rules from one ignore source, matched against paths relative to `prefix`.
struct Layer {
    QString prefix;
    QVector<Rule> rules;
};

QString globToRegex(QStringView glob) {
    QString re;
    for (qsizetype i = 0; i < glob.size(); ++i) {
        const QChar c = glob[i];
        if (c == u'*' && glob.mid(i, 3) == u"**/") { re += QLatin1String("(?:.*/)?"); i += 2; }
        else if (c == u'*' && glob.mid(i, 2) == u"**") { re += QLatin1String(".*"); ++i; }
        else if (c == u'*') re += QLatin1String("[^/]*");
        else if (c == u'?') re += QLatin1String("[^/]");
        else if (c == u'[' && glob.indexOf(u']', i + 1) > i + 1) {
            const qsizetype close = glob.indexOf(u']', i + 1);
            QString set = glob.mid(i + 1, close - i - 1).toString();
            if (set.startsWith(u'!')) set[0] = u'^';
            re += u'[' + set + u']';
            i = close;
        } else if (c == u'\\' && i + 1 < glob.size()) re += QRegularExpression::escape(glob.mid(++i, 1).toString());
        else re += QRegularExpression::escape(QString(c));
    }
    return re;
}

std::optional<Rule> compileRule(QString pattern) {
    pattern = pattern.trimmed();
    if (pattern.isEmpty() || pattern.startsWith(u'#')) return std::nullopt;
    Rule rule;
    if ((rule.negate = pattern.startsWith(u'!'))) pattern.remove(0, 1);
    if ((rule.dirOnly = pattern.endsWith(u'/'))) pattern.chop(1);
    const bool anchored = pattern.contains(u'/');  // "a/b" and "/a" are relative to the ignore file
    if (pattern.startsWith(u'/')) pattern.remove(0, 1);
    if (pattern.isEmpty()) return std::nullopt;
    rule.regex = QRegularExpression((anchored ? "^" : "(?:^|/)") + globToRegex(pattern) + "$");
    if (!rule.regex.isValid()) return std::nullopt;
    rule.regex.optimize();
    return rule;
}

QVector<Rule> compileRules(const QStringList& patterns) {
    QVector<Rule> rules;
    for (const QString& p : patterns)
        if (auto rule = compileRule(p)) rules.append(std::move(*rule));
    return rules;
}

bool ignoredBy(const QVector<Layer>& layers, const QString& path, bool isDir) {
    bool ignored = false;  // like git: the last matching rule wins
    for (const Layer& layer : layers) {
        const QString sub = path.mid(layer.prefix.size());
        for (const Rule& r : layer.rules)
            if (r.negate == ignored && (!r.dirOnly || isDir) && r.regex.match(sub).hasMatch()) ignored = !r.negate;
    }
    return ignored;
}

struct Walker {
    const FileCrawler::Options& options;
    QPromise<FileCrawler::Result>* promise;
    QVector<Layer> layers;
    FileCrawler::Result result;
    const LanguageRegistry& languages = LanguageRegistry::instance();

    void walk(const QString& dirPath, const QString& relDir) {
        bool pushed = false;
        if (options.useGitignore) {
            QFile file(dirPath + QLatin1String("/.gitignore"));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QVector<Rule> rules = compileRules(QString::fromUtf8(file.readAll()).split(u'\n'));
                if ((pushed = !rules.isEmpty())) layers.append({relDir.isEmpty() ? QString() : relDir + u'/', rules});
            }
        }
        const QFileInfoList entries = QDir(dirPath).entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
        for (const QFileInfo& entry : entries) {
            if (promise && promise->isCanceled()) break;
            const QString rel = relDir.isEmpty() ? entry.fileName() : relDir + u'/' + entry.fileName();
            const bool isDir = entry.isDir();
            if (ignoredBy(layers, rel, isDir)) continue;
            if (isDir) {
                if (!entry.isSymLink()) walk(entry.absoluteFilePath(), rel);  // symlinks could loop forever
                continue;
            }
            ++result.totalFiles;
            if (languages.byPath(rel) && entry.size() <= options.maxFileSize &&
                !FileCrawler::isBinaryFile(entry.absoluteFilePath()))
                result.files.append(entry.absoluteFilePath());
            if (promise) promise->setProgressValue(result.totalFiles);
        }
        if (pushed) layers.removeLast();
    }
};

FileCrawler::Result runScan(const FileCrawler::Options& options, QPromise<FileCrawler::Result>* promise) {
    Walker walker{options, promise, {{QString(), compileRules(options.ignorePatterns)}}, {}};
    const QDir root(options.rootPath);
    if (!options.rootPath.isEmpty() && root.exists()) walker.walk(root.absolutePath(), QString());
    return std::move(walker.result);
}
} // namespace

QStringList FileCrawler::defaultIgnorePatterns() {
    return {".git", ".svn", ".hg", ".bzr", "node_modules", "vendor", "build", "dist", "out", "bin", "obj",
            "target", ".idea", ".vscode", ".vs", "__pycache__", ".comment-explorer-backups", "*.pyc", "*.pyo",
            ".DS_Store", "Thumbs.db", "*.min.js", "*.min.css", "*.map", "*.lock", "package-lock.json", "*.log"};
}

QFuture<FileCrawler::Result> FileCrawler::scan(const Options& options) {
    return QtConcurrent::run([](QPromise<Result>& promise, const Options& opts) {
        promise.addResult(runScan(opts, &promise));
    }, options);
}

FileCrawler::Result FileCrawler::scanBlocking(const Options& options) { return runScan(options, nullptr); }

bool FileCrawler::isIgnored(const QString& relativePath, bool isDir, const QStringList& patterns) {
    return ignoredBy({{QString(), compileRules(patterns)}}, relativePath, isDir);
}

bool FileCrawler::isBinaryFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return true;
    const QByteArray head = file.read(8192);
    if (head.contains('\0')) return true;
    const auto control = std::count_if(head.begin(), head.end(), [](char ch) {
        const auto c = static_cast<unsigned char>(ch);
        return c < 32 && c != '\t' && c != '\n' && c != '\r' && c != '\f';
    });
    return control * 10 > head.size();  // more than 10% control characters
}

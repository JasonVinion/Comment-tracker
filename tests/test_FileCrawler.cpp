#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "FileCrawler.h"

class TestFileCrawler : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    void createFile(const QString& relPath, const QByteArray& content) {
        const QString path = m_dir.filePath(relPath);
        QDir().mkpath(QFileInfo(path).path());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    QStringList scan(bool useGitignore = true) {
        FileCrawler::Options options;
        options.rootPath = m_dir.path();
        options.ignorePatterns = FileCrawler::defaultIgnorePatterns();
        options.useGitignore = useGitignore;
        QStringList relative;
        for (const QString& f : FileCrawler::scanBlocking(options).files) relative << QDir(m_dir.path()).relativeFilePath(f);
        relative.sort();
        return relative;
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        createFile("src/main.cpp", "int main() { return 0; }");
        createFile("src/utils.h", "#ifndef UTILS_H\n#endif");
        createFile("src/nested/deep.py", "print('hello')");
        createFile("src/outline.cpp", "// not inside an ignored 'out' folder");
        createFile("src/binary.cpp", QByteArray("BIN\0\0DATA", 9));
        createFile("README.md", "# Readme");
        createFile("notes.txt", "unsupported extension");
        createFile("node_modules/dep/index.js", "module.exports = {}");
        createFile("out/generated.cpp", "int g;");
        createFile("lib/bundle.min.js", "x");
        createFile(".git/config.py", "x = 1");
        createFile("CMakeLists.txt", "project(x)");
        // .gitignore support: nested files, anchoring, directory-only rules and negation.
        createFile(".gitignore", "/secret.py\nlogs/\n*.gen.cpp\n!keep.gen.cpp\n");
        createFile("secret.py", "x = 1");
        createFile("src/secret.py", "x = 1");  // not ignored: "/secret.py" is anchored to the root
        createFile("logs/a.py", "x = 1");
        createFile("src/a.gen.cpp", "int a;");
        createFile("src/keep.gen.cpp", "int k;");
        createFile("src/sub/.gitignore", "*.py\n");
        createFile("src/sub/a.py", "x = 1");
        createFile("src/sub/b.cpp", "int b;");
    }

    void testScan() {
        const QStringList expected = {"CMakeLists.txt", "README.md", "src/keep.gen.cpp", "src/main.cpp",
                                      "src/nested/deep.py", "src/outline.cpp", "src/secret.py", "src/sub/b.cpp",
                                      "src/utils.h"};
        QCOMPARE(scan(), expected);
    }

    void testWithoutGitignore() {
        const QStringList files = scan(false);
        QVERIFY(files.contains("secret.py"));
        QVERIFY(files.contains("logs/a.py"));
        QVERIFY(files.contains("src/sub/a.py"));
        QVERIFY(!files.contains("node_modules/dep/index.js"));
    }

    void testAsyncScanAndCancel() {
        FileCrawler::Options options;
        options.rootPath = m_dir.path();
        auto future = FileCrawler::scan(options);
        future.waitForFinished();
        QVERIFY(future.resultCount() == 1);
        QVERIFY(future.result().totalFiles >= future.result().files.size());

        options.rootPath = m_dir.filePath("does-not-exist");
        QCOMPARE(FileCrawler::scanBlocking(options).files.size(), 0);
    }

    void testIgnorePatterns_data() {
        QTest::addColumn<QString>("path");
        QTest::addColumn<bool>("isDir");
        QTest::addColumn<QStringList>("patterns");
        QTest::addColumn<bool>("ignored");
        QTest::newRow("name anywhere") << "a/b/build" << true << QStringList{"build"} << true;
        QTest::newRow("name is not a prefix") << "src/builder.cpp" << false << QStringList{"build"} << false;
        QTest::newRow("name is not a suffix") << "src/rebuild" << true << QStringList{"build"} << false;
        QTest::newRow("glob") << "a/x.min.js" << false << QStringList{"*.min.js"} << true;
        QTest::newRow("glob stays in segment") << "a.min/x.js" << false << QStringList{"*.min.js"} << false;
        QTest::newRow("anchored") << "a/docs/x" << false << QStringList{"docs/x"} << false;
        QTest::newRow("anchored root") << "docs/x" << false << QStringList{"docs/x"} << true;
        QTest::newRow("dir only on file") << "logs" << false << QStringList{"logs/"} << false;
        QTest::newRow("dir only on dir") << "logs" << true << QStringList{"logs/"} << true;
        QTest::newRow("double star") << "a/b/c/test.py" << false << QStringList{"a/**/test.py"} << true;
        QTest::newRow("negation") << "keep.log" << false << QStringList{"*.log", "!keep.log"} << false;
        QTest::newRow("char class") << "file1.txt" << false << QStringList{"file[0-9].txt"} << true;
        QTest::newRow("comment line") << "#x" << false << QStringList{"#x"} << false;
    }

    void testIgnorePatterns() {
        QFETCH(QString, path);
        QFETCH(bool, isDir);
        QFETCH(QStringList, patterns);
        QFETCH(bool, ignored);
        QCOMPARE(FileCrawler::isIgnored(path, isDir, patterns), ignored);
    }

    void testBinaryFileDetection() {
        QVERIFY(FileCrawler::isBinaryFile(m_dir.filePath("src/binary.cpp")));
        QVERIFY(!FileCrawler::isBinaryFile(m_dir.filePath("src/main.cpp")));
        QVERIFY(FileCrawler::isBinaryFile(m_dir.filePath("missing.cpp")));
    }
};

QTEST_GUILESS_MAIN(TestFileCrawler)
#include "test_FileCrawler.moc"

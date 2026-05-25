#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include "FileCrawler.h"

class TestFileCrawler : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testFindCodeFiles();
    void testIgnorePatterns();
    void testBinaryFileDetection();
    void testEmptyDirectory();
    void testNestedDirectories();
    
private:
    void createTestFile(const QString& path, const QString& content);
    
    QTemporaryDir* tempDir;
};

void TestFileCrawler::initTestCase() {
    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
    
    // Create test file structure
    QDir dir(tempDir->path());
    dir.mkpath("src");
    dir.mkpath("src/nested");
    dir.mkpath("node_modules/dep");
    dir.mkpath(".git");
    
    // Create code files
    createTestFile(dir.filePath("src/main.cpp"), "int main() { return 0; }");
    createTestFile(dir.filePath("src/utils.h"), "#ifndef UTILS_H\n#define UTILS_H\n#endif");
    createTestFile(dir.filePath("src/nested/deep.py"), "print('hello')");
    createTestFile(dir.filePath("README.md"), "# Readme");
    
    // Create files that should be ignored
    createTestFile(dir.filePath("node_modules/dep/index.js"), "module.exports = {}");
    createTestFile(dir.filePath(".git/config"), "[core]\n");
}

void TestFileCrawler::cleanupTestCase() {
    delete tempDir;
}

void TestFileCrawler::createTestFile(const QString& path, const QString& content) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();
    }
}

void TestFileCrawler::testFindCodeFiles() {
    FileCrawler crawler;
    crawler.setRootPath(tempDir->path());
    
    QStringList foundFiles;
    QObject::connect(&crawler, &FileCrawler::fileFound, [&](const QString& path) {
        foundFiles.append(path);
    });
    
    QSignalSpy spy(&crawler, &FileCrawler::scanComplete);
    crawler.startScan();
    
    // Wait for scan to complete
    QVERIFY(spy.wait(5000));
    
    // Should find .cpp, .h, .py, .md files
    QVERIFY(foundFiles.size() >= 3);
    
    bool foundCpp = false, foundH = false, foundPy = false;
    for (const QString& f : foundFiles) {
        if (f.endsWith(".cpp")) foundCpp = true;
        if (f.endsWith(".h")) foundH = true;
        if (f.endsWith(".py")) foundPy = true;
    }
    
    QVERIFY(foundCpp);
    QVERIFY(foundH);
    QVERIFY(foundPy);
}

void TestFileCrawler::testIgnorePatterns() {
    FileCrawler crawler;
    crawler.setRootPath(tempDir->path());
    
    QStringList foundFiles;
    QObject::connect(&crawler, &FileCrawler::fileFound, [&](const QString& path) {
        foundFiles.append(path);
    });
    
    QSignalSpy spy(&crawler, &FileCrawler::scanComplete);
    crawler.startScan();
    
    QVERIFY(spy.wait(5000));
    
    // Should NOT find files in node_modules or .git
    for (const QString& f : foundFiles) {
        QVERIFY(!f.contains("node_modules"));
        QVERIFY(!f.contains(".git"));
    }
}

void TestFileCrawler::testBinaryFileDetection() {
    // Create a binary file
    QDir dir(tempDir->path());
    QString binaryPath = dir.filePath("binary.bin");
    
    QFile file(binaryPath);
    if (file.open(QIODevice::WriteOnly)) {
        // Write some binary content with null bytes
        QByteArray data;
        data.append("BIN");
        data.append('\0');
        data.append('\0');
        data.append("DATA");
        file.write(data);
        file.close();
    }
    
    QVERIFY(FileCrawler::isBinaryFile(binaryPath));
    
    // Text file should not be detected as binary
    QVERIFY(!FileCrawler::isBinaryFile(dir.filePath("src/main.cpp")));
}

void TestFileCrawler::testEmptyDirectory() {
    QTemporaryDir emptyDir;
    QVERIFY(emptyDir.isValid());
    
    FileCrawler crawler;
    crawler.setRootPath(emptyDir.path());
    
    QStringList foundFiles;
    QObject::connect(&crawler, &FileCrawler::fileFound, [&](const QString& path) {
        foundFiles.append(path);
    });
    
    QSignalSpy spy(&crawler, &FileCrawler::scanComplete);
    crawler.startScan();
    
    QVERIFY(spy.wait(5000));
    QCOMPARE(foundFiles.size(), 0);
}

void TestFileCrawler::testNestedDirectories() {
    FileCrawler crawler;
    crawler.setRootPath(tempDir->path());
    
    QStringList foundFiles;
    QObject::connect(&crawler, &FileCrawler::fileFound, [&](const QString& path) {
        foundFiles.append(path);
    });
    
    QSignalSpy spy(&crawler, &FileCrawler::scanComplete);
    crawler.startScan();
    
    QVERIFY(spy.wait(5000));
    
    // Should find the deeply nested file
    bool foundDeep = false;
    for (const QString& f : foundFiles) {
        if (f.contains("nested") && f.endsWith(".py")) {
            foundDeep = true;
            break;
        }
    }
    
    QVERIFY(foundDeep);
}

QTEST_MAIN(TestFileCrawler)
#include "test_FileCrawler.moc"

#include <QtTest/QtTest>
#include "CommentExtractor.h"
#include "LanguageRegistry.h"

class TestTreeSitter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testJsonExtraction();
    void testCppExtraction();
    void testPythonExtraction();
    void testJavaScriptExtraction();

private:
    CommentExtractor* extractor;
};

void TestTreeSitter::initTestCase() {
    extractor = new CommentExtractor();
    extractor->setParserMethod(CommentExtractor::ParserMethod::TreeSitter);
}

void TestTreeSitter::cleanupTestCase() {
    delete extractor;
}

void TestTreeSitter::testJsonExtraction() {
    QString code = R"(
{
    // Comment
    "key": "value" /* Block */
}
)";
    auto comments = extractor->extractFromContent(code, "JSONC");

    QCOMPARE(comments.size(), 2);
    QVERIFY(comments[0].cleanText.contains("Comment"));
    QVERIFY(comments[1].cleanText.contains("Block"));
}

void TestTreeSitter::testCppExtraction() {
    QString code = R"(
int main() {
    // Return 0
    return 0; /* Exit */
}
)";
    auto comments = extractor->extractFromContent(code, "C/C++");

    QCOMPARE(comments.size(), 2);
    QVERIFY(comments[0].cleanText.contains("Return 0"));
    QVERIFY(comments[1].cleanText.contains("Exit"));
}

void TestTreeSitter::testPythonExtraction() {
    QString code = R"(
def main():
    # Print hello
    print("Hello")
)";
    auto comments = extractor->extractFromContent(code, "Python");

    QCOMPARE(comments.size(), 1);
    QVERIFY(comments[0].cleanText.contains("Print hello"));
}

void TestTreeSitter::testJavaScriptExtraction() {
    QString code = R"(
function main() {
    // Log
    console.log("Hello"); /* Done */
}
)";
    auto comments = extractor->extractFromContent(code, "JavaScript");

    QCOMPARE(comments.size(), 2);
    QVERIFY(comments[0].cleanText.contains("Log"));
    QVERIFY(comments[1].cleanText.contains("Done"));
}

QTEST_MAIN(TestTreeSitter)
#include "test_TreeSitter.moc"

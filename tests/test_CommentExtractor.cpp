#include <QtTest/QtTest>
#include "CommentExtractor.h"
#include "LanguageRegistry.h"

class TestCommentExtractor : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Basic extraction tests
    void testSingleLineComment();
    void testMultiLineComment();
    void testInlineComment();
    void testDocComment();
    
    // Language-specific tests
    void testCppComments();
    void testPythonComments();
    void testJavaScriptComments();
    void testJsoncComments();
    void testShellComments();
    
    // Edge cases
    void testStringContainingCommentMarker();
    void testNestedComments();
    void testEmptyFile();
    void testNoComments();
    
    // Keyword detection
    void testTodoKeyword();
    void testMultipleKeywords();
    
private:
    CommentExtractor* extractor;
};

void TestCommentExtractor::initTestCase() {
    extractor = new CommentExtractor();
}

void TestCommentExtractor::cleanupTestCase() {
    delete extractor;
}

void TestCommentExtractor::testSingleLineComment() {
    QString code = "int x = 5; // this is a comment\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 1);
    QCOMPARE(comments[0].type, Comment::Type::Inline);
    QVERIFY(comments[0].cleanText.contains("this is a comment"));
}

void TestCommentExtractor::testMultiLineComment() {
    QString code = "/* This is a\n   multi-line comment */\nint x = 5;\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 1);
    QCOMPARE(comments[0].type, Comment::Type::MultiLine);
    QVERIFY(comments[0].cleanText.contains("multi-line"));
}

void TestCommentExtractor::testInlineComment() {
    QString code = "int x = 5; // inline comment\nint y = 10;\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 1);
    QCOMPARE(comments[0].type, Comment::Type::Inline);
    QVERIFY(!comments[0].codeSegment.isEmpty());
}

void TestCommentExtractor::testDocComment() {
    QString code = "/// This is a doc comment\nvoid foo() {}\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 1);
    QCOMPARE(comments[0].type, Comment::Type::DocComment);
}

void TestCommentExtractor::testCppComments() {
    QString code = R"(
// Single line comment
int x = 5; // inline
/* block comment */
/** doc comment */
)";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 4);
}

void TestCommentExtractor::testPythonComments() {
    QString code = R"(
# This is a Python comment
x = 5  # inline comment
"""
Docstring comment
"""
)";
    auto comments = extractor->extractFromContent(code, "Python");
    
    QVERIFY(comments.size() >= 2);
    QVERIFY(comments[0].cleanText.contains("Python comment"));
}

void TestCommentExtractor::testJavaScriptComments() {
    QString code = R"(
// JS comment
/* block */
/** JSDoc */
const x = 5; // inline
)";
    auto comments = extractor->extractFromContent(code, "JavaScript");
    
    QVERIFY(comments.size() >= 3);
}

void TestCommentExtractor::testShellComments() {
    QString code = R"(
# Shell comment
echo "hello"  # inline
)";
    auto comments = extractor->extractFromContent(code, "Shell");
    
    QVERIFY(comments.size() >= 1);
}

void TestCommentExtractor::testJsoncComments() {
    QString code = R"(
{
    // This is a comment
    "key": "value",
    /* Block comment */
    "key2": "value // not a comment"
}
)";
    auto comments = extractor->extractFromContent(code, "JSONC");

    QCOMPARE(comments.size(), 2);
    QVERIFY(comments[0].cleanText.contains("This is a comment"));
    QVERIFY(comments[1].cleanText.contains("Block comment"));
}

void TestCommentExtractor::testStringContainingCommentMarker() {
    QString code = R"(
const s = "This // is not a comment";
const s2 = "/* also not */";
// This is a real comment
)";
    auto comments = extractor->extractFromContent(code, "JavaScript");
    
    // Should only find the real comment
    QCOMPARE(comments.size(), 1);
    QVERIFY(comments[0].cleanText.contains("real comment"));
}

void TestCommentExtractor::testNestedComments() {
    // Rust supports nested comments
    QString code = R"(
/* outer /* nested */ still in comment */
fn main() {}
)";
    auto comments = extractor->extractFromContent(code, "Rust");
    
    QCOMPARE(comments.size(), 1);
}

void TestCommentExtractor::testEmptyFile() {
    QString code = "";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 0);
}

void TestCommentExtractor::testNoComments() {
    QString code = "int x = 5;\nint y = 10;\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 0);
}

void TestCommentExtractor::testTodoKeyword() {
    QString code = "// TODO: Fix this later\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 1);
    QVERIFY(comments[0].keywords.contains("TODO"));
}

void TestCommentExtractor::testMultipleKeywords() {
    QString code = "// TODO FIXME: This is broken and needs fixing\n";
    auto comments = extractor->extractFromContent(code, "C/C++");
    
    QCOMPARE(comments.size(), 1);
    QVERIFY(comments[0].keywords.contains("TODO"));
    QVERIFY(comments[0].keywords.contains("FIXME"));
}

QTEST_MAIN(TestCommentExtractor)
#include "test_CommentExtractor.moc"

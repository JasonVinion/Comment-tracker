#include <QtTest/QtTest>
#include "EditPlanner.h"
#include "CommentExtractor.h"

class TestEditPlanner : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testFullLineCommentRemoval();
    void testInlineCommentRemoval();
    void testBlockCommentRemoval();
    void testMultipleEditsPerFile();
    void testPreserveCode();
    
private:
    EditPlanner* planner;
    CommentExtractor* extractor;
};

void TestEditPlanner::initTestCase() {
    planner = new EditPlanner();
    extractor = new CommentExtractor();
}

void TestEditPlanner::cleanupTestCase() {
    delete planner;
    delete extractor;
}

void TestEditPlanner::testFullLineCommentRemoval() {
    QString content = "// This is a comment\nint x = 5;\n";
    
    // Create a mock comment
    Comment comment;
    comment.id = 1;
    comment.filePath = "/tmp/test.cpp";
    comment.startLine = 1;
    comment.startColumn = 0;
    comment.endLine = 1;
    comment.endColumn = 20;
    comment.type = Comment::Type::SingleLine;
    comment.rawText = "// This is a comment";
    
    QVector<EditOperation> ops;
    EditOperation op;
    op.commentId = 1;
    op.filePath = "/tmp/test.cpp";
    op.startLine = 1;
    op.endLine = 1;
    op.type = EditOperation::Type::RemoveFullLine;
    ops.append(op);
    
    QString result = planner->applyEdits(content, ops);
    
    QVERIFY(!result.contains("// This is a comment"));
    QVERIFY(result.contains("int x = 5"));
}

void TestEditPlanner::testInlineCommentRemoval() {
    QString content = "int x = 5; // inline comment\nint y = 10;\n";
    
    QVector<EditOperation> ops;
    EditOperation op;
    op.commentId = 1;
    op.filePath = "/tmp/test.cpp";
    op.startLine = 1;
    op.startColumn = 11;  // Position of //
    op.endLine = 1;
    op.type = EditOperation::Type::RemoveInline;
    ops.append(op);
    
    QString result = planner->applyEdits(content, ops);
    
    QVERIFY(result.contains("int x = 5"));
    QVERIFY(!result.contains("inline comment"));
}

void TestEditPlanner::testBlockCommentRemoval() {
    QString content = "/* block */\nint x = 5;\n";
    
    QVector<EditOperation> ops;
    EditOperation op;
    op.commentId = 1;
    op.filePath = "/tmp/test.cpp";
    op.startLine = 1;
    op.startColumn = 0;
    op.endLine = 1;
    op.endColumn = 11;
    op.type = EditOperation::Type::RemoveBlock;
    ops.append(op);
    
    QString result = planner->applyEdits(content, ops);
    
    QVERIFY(!result.contains("block"));
    QVERIFY(result.contains("int x = 5"));
}

void TestEditPlanner::testMultipleEditsPerFile() {
    QString content = "// Comment 1\nint x = 5;\n// Comment 2\nint y = 10;\n";
    
    QVector<EditOperation> ops;
    
    // Add in reverse order (line 3 first, then line 1)
    EditOperation op2;
    op2.commentId = 2;
    op2.filePath = "/tmp/test.cpp";
    op2.startLine = 3;
    op2.endLine = 3;
    op2.type = EditOperation::Type::RemoveFullLine;
    ops.append(op2);
    
    EditOperation op1;
    op1.commentId = 1;
    op1.filePath = "/tmp/test.cpp";
    op1.startLine = 1;
    op1.endLine = 1;
    op1.type = EditOperation::Type::RemoveFullLine;
    ops.append(op1);
    
    QString result = planner->applyEdits(content, ops);
    
    QVERIFY(!result.contains("Comment 1"));
    QVERIFY(!result.contains("Comment 2"));
    QVERIFY(result.contains("int x = 5"));
    QVERIFY(result.contains("int y = 10"));
}

void TestEditPlanner::testPreserveCode() {
    QString content = "int foo() { // comment\n    return 42;\n}\n";
    
    QVector<EditOperation> ops;
    EditOperation op;
    op.commentId = 1;
    op.filePath = "/tmp/test.cpp";
    op.startLine = 1;
    op.startColumn = 12;
    op.endLine = 1;
    op.type = EditOperation::Type::RemoveInline;
    ops.append(op);
    
    QString result = planner->applyEdits(content, ops);
    
    // Code should be preserved
    QVERIFY(result.contains("int foo()"));
    QVERIFY(result.contains("return 42"));
    // Comment should be removed
    QVERIFY(!result.contains("// comment"));
}

QTEST_MAIN(TestEditPlanner)
#include "test_EditPlanner.moc"

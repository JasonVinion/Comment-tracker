#include <QtTest/QtTest>
#include <QDir>
#include <QCoreApplication>
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
    
    // Harvesting live test
    void testHarvesting();
    
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

void TestCommentExtractor::testHarvesting() {
    QString testDir;
    QDir current(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (current.exists("test_examples")) {
            testDir = current.filePath("test_examples");
            break;
        }
        if (!current.cdUp()) break;
    }
    
    QDir dir(testDir);
    QVERIFY(dir.exists());
    
    QStringList files = dir.entryList({"*.cpp", "*.py", "*.c", "*.php"}, QDir::Files);
    QVERIFY(files.size() >= 4);
    
    QVector<Comment> allComments;
    for (const QString& file : files) {
        QString fullPath = dir.filePath(file);
        auto comments = extractor->extractFromFile(fullPath, file);
        allComments.append(comments);
    }
    
    QMap<QString, QVector<Comment>> commentsByFile;
    for (const Comment& c : allComments) {
        commentsByFile[c.filePath].append(c);
    }
    
    static const QStringList matchTerms = {
        "todo", "fix", "hack", "fix later", 
        "partial implmentation", "partial implementation",
        "simple implmentation", "simple implementation",
        "example", "do later", "unstable",
        "xxx", "bug", "deprecated", "warning", "optimize", "review"
    };
    
    auto isSpecial = [](const Comment& comment) -> bool {
        QString cleanTextLower = comment.cleanText.toLower();
        QString rawTextLower = comment.rawText.toLower();
        
        for (const QString& term : matchTerms) {
            if (cleanTextLower.contains(term) || rawTextLower.contains(term)) {
                return true;
            }
        }
        return false;
    };
    
    int exportedGroupsCount = 0;
    int exportedCommentsCount = 0;
    
    for (auto it = commentsByFile.begin(); it != commentsByFile.end(); ++it) {
        QVector<Comment> fileComments = it.value();
        
        std::sort(fileComments.begin(), fileComments.end(), [](const Comment& a, const Comment& b) {
            if (a.startLine != b.startLine) {
                return a.startLine < b.startLine;
            }
            return a.startColumn < b.startColumn;
        });
        
        QVector<QVector<Comment>> groups;
        if (!fileComments.isEmpty()) {
            QVector<Comment> currentGroup;
            currentGroup.append(fileComments[0]);
            
            for (int i = 1; i < fileComments.size(); ++i) {
                const Comment& prev = currentGroup.last();
                const Comment& curr = fileComments[i];
                
                int gap = qMax(0, curr.startLine - prev.endLine - 1);
                if (gap <= 2) {
                    currentGroup.append(curr);
                } else {
                    groups.append(currentGroup);
                    currentGroup.clear();
                    currentGroup.append(curr);
                }
            }
            if (!currentGroup.isEmpty()) {
                groups.append(currentGroup);
            }
        }
        
        for (const auto& group : groups) {
            bool hasSpecial = false;
            for (const Comment& c : group) {
                if (isSpecial(c)) {
                    hasSpecial = true;
                    break;
                }
            }
            
            if (hasSpecial) {
                exportedGroupsCount++;
                exportedCommentsCount += group.size();
                
                qDebug() << "Harvested Group in" << group.first().relativePath << "Lines:" << group.first().startLine << "-" << group.last().endLine;
                for (const Comment& c : group) {
                    qDebug() << "  * Line" << c.startLine << "(" << c.typeString() << "):" << c.cleanText;
                }
                
                // Assert that the group is not empty and has a valid file path
                QVERIFY(!group.isEmpty());
                QVERIFY(!group.first().relativePath.isEmpty());
            }
        }
    }
    
    // We expect to find special comment groups in all 4 files
    QVERIFY(exportedGroupsCount >= 4);
}

QTEST_MAIN(TestCommentExtractor)
#include "test_CommentExtractor.moc"

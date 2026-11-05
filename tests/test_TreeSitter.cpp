#include <QtTest/QtTest>
#include <QtConcurrent>
#include "CommentExtractor.h"

class TestTreeSitter : public QObject {
    Q_OBJECT

    CommentExtractor m_extractor;

    QVector<Comment> extract(const QString& code, const QString& language) {
        m_extractor.setParserMethod(CommentExtractor::ParserMethod::TreeSitter);
        return m_extractor.extractFromContent(code, language);
    }

private slots:
    void testJsonExtraction() {
        auto comments = extract("{\n    // Comment\n    \"key\": \"value\" /* Block */\n}\n", "JSONC");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].cleanText(), QString("Comment"));
        QCOMPARE(comments[1].cleanText(), QString("Block"));
        QCOMPARE(comments[1].type, Comment::Type::Inline);
    }

    void testCppExtraction() {
        auto comments = extract("int main() {\n    // Return 0\n    return 0; /* Exit */\n}\n", "C/C++");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].cleanText(), QString("Return 0"));
        QCOMPARE(comments[0].startLine, 2);
        QCOMPARE(comments[0].startColumn, 4);
        QCOMPARE(comments[1].cleanText(), QString("Exit"));
    }

    void testPythonExtraction() {
        auto comments = extract("def main():\n    \"\"\"Docstring.\"\"\"\n    # Print hello\n    print(\"Hello\")\n",
                                "Python");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].type, Comment::Type::DocComment);
        QCOMPARE(comments[0].cleanText(), QString("Docstring."));
        QCOMPARE(comments[1].cleanText(), QString("Print hello"));
    }

    void testJavaScriptExtraction() {
        auto comments = extract("function main() {\n    // Log\n    console.log(\"Hello\"); /* Done */\n}\n",
                                "JavaScript");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].cleanText(), QString("Log"));
        QCOMPARE(comments[1].cleanText(), QString("Done"));
    }

    void testParsersAreThreadSafe() {
        QVector<int> counts(64);
        QtConcurrent::blockingMap(counts, [this](int& count) {
            count = int(m_extractor.extractFromContent("// a\nint x; /* b */\n", "C/C++").size());
        });
        QVERIFY(std::all_of(counts.begin(), counts.end(), [](int c) { return c == 2; }));
    }
};

QTEST_GUILESS_MAIN(TestTreeSitter)
#include "test_TreeSitter.moc"

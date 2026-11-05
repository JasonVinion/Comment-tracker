#include <QtTest/QtTest>
#include "CommentExtractor.h"
#include "LanguageRegistry.h"

using Method = CommentExtractor::ParserMethod;
Q_DECLARE_METATYPE(CommentExtractor::ParserMethod)

/// Every test runs once with the lexer and once with Tree-sitter
/// (which falls back to the lexer for languages without a grammar).
class TestCommentExtractor : public QObject {
    Q_OBJECT

    QVector<Comment> extract(const QString& code, const QString& language) {
        QFETCH_GLOBAL(Method, method);
        m_extractor.setParserMethod(method);
        return m_extractor.extractFromContent(code, language);
    }
    bool isLexer() {
        QFETCH_GLOBAL(Method, method);
        return method == Method::Lexer;
    }

    CommentExtractor m_extractor;

private slots:
    void initTestCase_data() {
        QTest::addColumn<Method>("method");
        QTest::newRow("lexer") << Method::Lexer;
        QTest::newRow("tree-sitter") << Method::TreeSitter;
    }

    void testSingleLineComment() {
        auto comments = extract("int x = 5; // this is a comment\n", "C/C++");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].type, Comment::Type::Inline);
        QCOMPARE(comments[0].cleanText(), QString("this is a comment"));
        QCOMPARE(comments[0].startColumn, 11);
        QCOMPARE(comments[0].endColumn, 31);
    }

    void testMultiLineComment() {
        auto comments = extract("/* This is a\n   multi-line comment */\nint x = 5;\n", "C/C++");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].type, Comment::Type::MultiLine);
        QCOMPARE(comments[0].startLine, 1);
        QCOMPARE(comments[0].endLine, 2);
        QVERIFY(comments[0].cleanText().startsWith("This is a"));
        QVERIFY(comments[0].cleanText().endsWith("multi-line comment"));
    }

    void testDocComment() {
        auto comments = extract("/// This is a doc comment\nvoid foo() {}\n/** block doc */\nvoid bar() {}\n", "C/C++");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].type, Comment::Type::DocComment);
        QCOMPARE(comments[1].type, Comment::Type::DocComment);
        QCOMPARE(comments[1].cleanText(), QString("block doc"));
    }

    void testCppComments() {
        QCOMPARE(extract("\n// Single line comment\nint x = 5; // inline\n/* block comment */\n/** doc comment */\n",
                         "C/C++").size(), 4);
    }

    void testEmptyBlockDoesNotSwallowCode() {
        // "/**/" used to be read as an unterminated doc comment that ran to the next "*/".
        auto comments = extract("int a; /**/ int b;\nint c; /* real */\n", "C/C++");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].rawText, QString("/**/"));
        QCOMPARE(comments[0].type, Comment::Type::Inline);
        QCOMPARE(comments[1].rawText, QString("/* real */"));
    }

    void testPythonComments() {
        auto comments = extract("# This is a Python comment\nx = 5  # inline comment\n\"\"\"\nDocstring comment\n\"\"\"\n",
                                "Python");
        QCOMPARE(comments.size(), 3);
        QCOMPARE(comments[0].cleanText(), QString("This is a Python comment"));
        QCOMPARE(comments[1].type, Comment::Type::Inline);
        QCOMPARE(comments[2].type, Comment::Type::DocComment);
        QCOMPARE(comments[2].cleanText(), QString("Docstring comment"));
    }

    void testPythonStringsAreNotComments() {
        auto comments = extract("x = \"\"\"\n# not a comment\n\"\"\"\ncall(\n    \"\"\"also data\"\"\"\n)\ns = '#'\n",
                                "Python");
        QCOMPARE(comments.size(), 0);
    }

    void testJavaScriptComments() {
        auto comments = extract("// JS comment\n/* block */\n/** JSDoc */\nconst x = 5; // inline\n", "JavaScript");
        QCOMPARE(comments.size(), 4);
        QCOMPARE(comments[2].type, Comment::Type::DocComment);
    }

    void testJavaScriptRegexIsNotComment() {
        auto comments = extract("const re = /\\/\\/ not a comment/g;\nconst half = a / b; // real\n", "JavaScript");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].cleanText(), QString("real"));
    }

    void testTypeScriptFallsBackGracefully() {
        // The JavaScript grammar cannot parse type annotations; extraction must still work.
        auto comments = extract("const x: number = 5; // typed\ninterface A { b: string } /* c */\n", "TypeScript");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].cleanText(), QString("typed"));
        QCOMPARE(comments[1].cleanText(), QString("c"));
    }

    void testJsoncComments() {
        auto comments = extract("{\n    // This is a comment\n    \"key\": \"value\",\n    /* Block comment */\n"
                                "    \"key2\": \"value // not a comment\"\n}\n", "JSONC");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].cleanText(), QString("This is a comment"));
        QCOMPARE(comments[1].cleanText(), QString("Block comment"));
    }

    void testShellComments() {
        auto comments = extract("#!/bin/bash\n# Shell comment\necho \"hello\" $# ${#arr[@]} # inline\n", "Shell");
        QCOMPARE(comments.size(), 2);  // the shebang, $# and ${#...} are not comments
        QCOMPARE(comments[0].cleanText(), QString("Shell comment"));
        QCOMPARE(comments[1].cleanText(), QString("inline"));
    }

    void testLuaComments() {
        auto comments = extract("--[[ block\nstill comment ]]\ns = [[ long -- string ]]\n-- real\n", "Lua");
        QCOMPARE(comments.size(), 2);
        QCOMPARE(comments[0].type, Comment::Type::MultiLine);
        QCOMPARE(comments[0].endLine, 2);
        QCOMPARE(comments[1].cleanText(), QString("real"));
    }

    void testTomlTripleQuotedString() {
        auto comments = extract("a = \"\"\"\nx # not a comment\n\"\"\"\n# real\n", "TOML");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].cleanText(), QString("real"));
    }

    void testStringContainingCommentMarker() {
        auto comments = extract("const s = \"This // is not a comment\";\nconst s2 = '/* also not */';\n"
                                "// This is a real comment\n", "JavaScript");
        QCOMPARE(comments.size(), 1);
        QVERIFY(comments[0].cleanText().contains("real comment"));
    }

    void testCssUrlsAndPhpAttributes() {
        auto css = extract("a { background: url(http://x.org/a.png); } // scss comment\n/* block */\n", "CSS");
        QCOMPARE(css.size(), 2);
        QCOMPARE(css[0].cleanText(), QString("scss comment"));
        auto php = extract("<?php\n#[Attribute]\nclass A {} # real\n", "PHP");
        QCOMPARE(php.size(), 1);
        QCOMPARE(php[0].cleanText(), QString("real"));
    }

    void testCppRawString() {
        auto comments = extract("auto s = R\"x(// not a comment )\" still)x\"; // real\n", "C/C++");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].cleanText(), QString("real"));
    }

    void testNestedComments() {
        auto comments = extract("/* outer /* nested */ still in comment */\nfn main() {}\n", "Rust");
        QCOMPARE(comments.size(), 1);
        QVERIFY(comments[0].rawText.endsWith("still in comment */"));
    }

    void testUnicodePositions() {
        auto comments = extract("auto s = \"héllo😀\"; // ünïcode\n", "C/C++");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].rawText, QString("// ünïcode"));
        QCOMPARE(comments[0].startColumn, 20);  // UTF-16 columns, matching QString
    }

    void testEmptyFile() { QCOMPARE(extract("", "C/C++").size(), 0); }
    void testNoComments() { QCOMPARE(extract("int x = 5;\nint y = 10;\n", "C/C++").size(), 0); }
    void testUnknownLanguage() { QCOMPARE(extract("// x\n", "Klingon").size(), 0); }

    void testTodoKeyword() {
        auto comments = extract("// TODO: Fix this later\n", "C/C++");
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].keywords, QStringList({"TODO"}));
    }

    void testMultipleKeywords() {
        auto comments = extract("// todo FIXME: This is broken, DEBUG output only\n", "C/C++");
        QCOMPARE(comments.size(), 1);
        QVERIFY(comments[0].keywords.contains("TODO"));
        QVERIFY(comments[0].keywords.contains("FIXME"));
        QVERIFY(!comments[0].keywords.contains("BUG"));  // whole words only
    }

    void testUniqueIds() {
        auto comments = extract("// a\n// b\n", "C/C++");
        QCOMPARE(comments.size(), 2);
        QVERIFY(comments[0].id != comments[1].id);
    }

    void testLanguageLookup() {
        const auto& registry = LanguageRegistry::instance();
        QCOMPARE(registry.byPath(u"/x/CMakeLists.txt")->name, QString("CMake"));
        QCOMPARE(registry.byPath(u"/x/Makefile")->name, QString("Makefile"));
        QCOMPARE(registry.byPath(u"/x/Dockerfile")->name, QString("Dockerfile"));
        QCOMPARE(registry.byPath(u"src/main.CPP")->name, QString("C/C++"));
        QCOMPARE(registry.byExtension(".py")->name, QString("Python"));
        QVERIFY(!registry.byPath(u"/x/readme"));
        QVERIFY(!registry.byPath(u"/x/archive.zip"));
    }

    void testExampleFiles() {
        // The bundled examples each contain at least one special (TODO, fix later, ...) comment.
        const QDir dir(TEST_EXAMPLES_DIR);
        const QStringList files = dir.entryList({"*.cpp", "*.py", "*.c", "*.php"}, QDir::Files);
        QCOMPARE(files.size(), 4);
        QFETCH_GLOBAL(Method, method);
        m_extractor.setParserMethod(method);
        for (const QString& file : files) {
            const auto comments = m_extractor.extractFromFile(dir.filePath(file), file);
            QVERIFY2(comments.size() >= 3, qPrintable(file));
            QCOMPARE(comments[0].relativePath, file);
            const bool special = std::any_of(comments.begin(), comments.end(), [](const Comment& c) {
                const QString t = c.cleanText().toLower();
                return t.contains("todo") || t.contains("fix") || t.contains("unstable") ||
                       t.contains("example") || t.contains("partial implmentation") || t.contains("do later");
            });
            QVERIFY2(special, qPrintable(file));
        }
    }
};

QTEST_GUILESS_MAIN(TestCommentExtractor)
#include "test_CommentExtractor.moc"

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "CommentExtractor.h"
#include "EditPlanner.h"
#include "FileWriter.h"

class TestEditPlanner : public QObject {
    Q_OBJECT

    /// Extracts every comment from `code` and deletes the ones whose raw text is in `remove`
    /// (or all of them when `remove` is empty).
    static QString removeComments(const QString& code, const QStringList& remove = {},
                                  const QString& language = "C/C++") {
        CommentExtractor extractor;
        QVector<EditRange> ranges;
        for (const Comment& c : extractor.extractFromContent(code, language))
            if (remove.isEmpty() || remove.contains(c.rawText)) ranges << EditPlanner::rangeFor(c);
        QString error;
        const QString result = EditPlanner::applyEdits(code, ranges, &error);
        if (!error.isEmpty()) qWarning() << error;
        return result;
    }

    static QByteArray readAll(const QString& path) {
        QFile f(path);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }
    static void writeAll(const QString& path, const QByteArray& data) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
    }

private slots:
    void testFullLineCommentRemoval() {
        QCOMPARE(removeComments("// This is a comment\nint x = 5;\n"), QString("int x = 5;\n"));
        QCOMPARE(removeComments("int x = 5;\n    // indented, last line"), QString("int x = 5;\n"));
    }

    void testInlineCommentRemoval() {
        QCOMPARE(removeComments("int x = 5; // inline comment\nint y = 10;\n"), QString("int x = 5;\nint y = 10;\n"));
    }

    void testBlockCommentRemoval() {
        QCOMPARE(removeComments("/* block */\nint x = 5;\n"), QString("int x = 5;\n"));
        QCOMPARE(removeComments("int a;\n  /* multi\n     line */\nint b;\n"), QString("int a;\nint b;\n"));
    }

    void testCodeAfterBlockCommentIsKept() {
        // Regression: these used to delete the code next to the comment.
        QCOMPARE(removeComments("/* x */ int keep = 1;\n"), QString("int keep = 1;\n"));
        QCOMPARE(removeComments("    /* x */ int keep = 1;\n"), QString("    int keep = 1;\n"));
        QCOMPARE(removeComments("int y; /* a */ int z;\n"), QString("int y; int z;\n"));
        QCOMPARE(removeComments("int a = f(1,/*b*/2);\n"), QString("int a = f(1,2);\n"));
        QCOMPARE(removeComments("int ab = a/**/b;\n"), QString("int ab = a b;\n"));  // tokens must not fuse
        QCOMPARE(removeComments("/** doc\n */ int x;\n"), QString("int x;\n"));
        QCOMPARE(removeComments("int x; /* a\n b */ int y;\n"), QString("int x; int y;\n"));
    }

    void testSeveralCommentsOnOneLine() {
        QCOMPARE(removeComments("/* a */ /* b */\nint x;\n"), QString("int x;\n"));
        QCOMPARE(removeComments("int x; /* a */ // b\n"), QString("int x;\n"));
        QCOMPARE(removeComments("/* a */ /* b */ int x;\n"), QString("int x;\n"));
        QCOMPARE(removeComments("int x; /* a */ /* b */ int y;\n", {"/* b */"}), QString("int x; /* a */ int y;\n"));
    }

    void testMultipleEditsPerFile() {
        QCOMPARE(removeComments("// Comment 1\nint x = 5;\n// Comment 2\nint y = 10; // three\n"),
                 QString("int x = 5;\nint y = 10;\n"));
    }

    void testOnlySelectedCommentsRemoved() {
        QCOMPARE(removeComments("// keep\n// drop\nint x;\n", {"// drop"}), QString("// keep\nint x;\n"));
    }

    void testOverlappingRangesMerge() {
        const QString code = "int a; /* one */ int b;\n";
        EditRange r{0, 1, 7, 1, 16, {}};
        QCOMPARE(EditPlanner::applyEdits(code, {r, r}), QString("int a; int b;\n"));
    }

    void testPythonDocstringRemoval() {
        QCOMPARE(removeComments("def f():\n    \"\"\"Doc.\"\"\"\n    return 1  # one\n", {}, "Python"),
                 QString("def f():\n    return 1\n"));
    }

    void testStaleRangeIsRejected() {
        EditRange r{1, 1, 0, 1, 10, "// comment"};
        QString error;
        const QString code = "int moved; // comment\n";
        QCOMPARE(EditPlanner::applyEdits(code, {r}, &error), code);
        QVERIFY(error.contains("changed"));

        error.clear();
        r = EditRange{1, 9, 0, 9, 3, {}};
        EditPlanner::applyEdits(code, {r}, &error);
        QVERIFY(!error.isEmpty());
    }

    void testPlanGroupsByFile() {
        Comment a, b, c;
        a.filePath = c.filePath = "/a.cpp";
        b.filePath = "/b.cpp";
        const EditPlan plan = EditPlanner::plan({&a, &b, &c});
        QCOMPARE(plan.size(), 2);
        QCOMPARE(plan["/a.cpp"].size(), 2);
    }

    void testWriterPreservesLineEndingsAndBom() {
        QTemporaryDir dir;
        const QString path = dir.filePath("crlf.cpp");
        writeAll(path, "\xEF\xBB\xBF// remove me\r\nint x; // and me\r\nint y;\r\n");

        CommentExtractor extractor;
        const auto comments = extractor.extractFromFile(path, "crlf.cpp");
        QCOMPARE(comments.size(), 2);
        QVector<EditRange> ranges;
        for (const Comment& c : comments) ranges << EditPlanner::rangeFor(c);

        FileWriter writer;
        writer.setBackupDirectory(dir.filePath("backups"));
        QString error;
        QVERIFY2(writer.apply(path, ranges, &error), qPrintable(error));
        QCOMPARE(readAll(path), QByteArray("\xEF\xBB\xBFint x;\r\nint y;\r\n"));

        // The backup holds the untouched original.
        const QStringList backups = QDir(dir.filePath("backups")).entryList(QDir::Files);
        QCOMPARE(backups.size(), 1);
        QVERIFY(readAll(dir.filePath("backups/" + backups[0])).startsWith("\xEF\xBB\xBF// remove me\r\n"));
        QVERIFY(writer.auditLog().size() >= 2);

        // Applying the same (now stale) plan again must not touch the file.
        QVERIFY(!writer.apply(path, ranges, &error));
        QCOMPARE(readAll(path), QByteArray("\xEF\xBB\xBFint x;\r\nint y;\r\n"));
    }
};

QTEST_GUILESS_MAIN(TestEditPlanner)
#include "test_EditPlanner.moc"

#include "EditPlanner.h"
#include <algorithm>

namespace {
bool isBlank(QStringView s) {
    return std::all_of(s.begin(), s.end(), [](QChar c) { return c.isSpace(); });
}
} // namespace

EditRange EditPlanner::rangeFor(const Comment& c) {
    return {c.id, c.startLine, c.startColumn, c.endLine, c.endColumn, c.rawText};
}

EditPlan EditPlanner::plan(const QVector<const Comment*>& comments) {
    EditPlan plan;
    for (const Comment* c : comments) plan[c->filePath].append(rangeFor(*c));
    return plan;
}

QString EditPlanner::applyEdits(const QString& content, const QVector<EditRange>& ranges, QString* error) {
    const LineIndex lines(content);
    const QStringView text(content);
    QVector<std::pair<int, int>> spans;
    spans.reserve(ranges.size());
    for (const EditRange& r : ranges) {
        const int s = lines.offset(r.startLine, r.startColumn), e = lines.offset(r.endLine, r.endColumn);
        const char* problem = (s < 0 || e < s) ? "position is outside the file"
                            : (!r.expectedText.isNull() && text.mid(s, e - s) != r.expectedText)
                                ? "the file changed since it was scanned; please rescan"
                                : nullptr;
        if (problem) {
            if (error) *error = QStringLiteral("Comment at line %1: %2").arg(r.startLine).arg(QLatin1String(problem));
            return content;
        }
        spans.append({s, e});
    }
    std::sort(spans.begin(), spans.end());

    const int n = int(content.size());
    QString out;
    out.reserve(n);
    int pos = 0;
    for (int i = 0; i < spans.size(); ++i) {
        const int s = std::max(spans[i].first, pos);
        int e = spans[i].second;
        while (i + 1 < spans.size() && spans[i + 1].first <= e) e = std::max(e, spans[++i].second);
        if (e <= s) continue;

        out += text.mid(pos, s - pos);
        const int outLineStart = int(out.lastIndexOf(u'\n')) + 1;
        int lineEnd = int(content.indexOf(u'\n', e));
        if (lineEnd < 0) lineEnd = n;
        const bool blankBefore = isBlank(QStringView(out).mid(outLineStart));
        const bool blankAfter = isBlank(text.mid(e, lineEnd - e));

        if (blankBefore && blankAfter) {          // comment-only line: drop it entirely
            out.truncate(outLineStart);
            pos = lineEnd < n ? lineEnd + 1 : n;
        } else if (blankAfter) {                  // trailing comment: trim spaces before it
            while (out.size() > outLineStart && out.back().isSpace()) out.chop(1);
            pos = lineEnd;
        } else if (blankBefore) {                 // leading comment: code keeps the indentation
            for (pos = e; pos < lineEnd && text[pos].isSpace();) ++pos;
        } else {                                  // code on both sides: one space keeps tokens apart
            const bool hadSpace = out.back().isSpace() || text[e].isSpace();
            while (out.back().isSpace()) out.chop(1);
            for (pos = e; text[pos].isSpace();) ++pos;
            const auto isPunct = [](QChar c) { return QStringView(u"()[]{},;").contains(c); };
            if (hadSpace || !(isPunct(out.back()) || isPunct(text[pos]))) out += u' ';
        }
    }
    out += text.mid(pos);
    return out;
}

EditResult EditPlanner::preview(const QString& filePath, const QVector<EditRange>& ranges) {
    EditResult result;
    QString error;
    if (!result.original.read(filePath, &error)) {
        result.errorMessage = QStringLiteral("Cannot read %1: %2").arg(filePath, error);
        return result;
    }
    result.modifiedContent = applyEdits(result.original.text, ranges, &result.errorMessage);
    return result;
}

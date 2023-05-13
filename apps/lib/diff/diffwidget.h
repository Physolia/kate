/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "diffeditor.h"
#include <QWidget>

#include "kateprivate_export.h"

#include <KTextEditor/Document>

namespace KSyntaxHighlighting
{
class SyntaxHighlighter;
}

/**
 * - Small wrapper over DiffWidget. It tries to reuse an existing diff widget instead of opening a new one
 * - Two DiffWidgets are considered equal if their DiffParams.arguments match
 * - The diff is always recalculated
 */
class DiffWidgetManager
{
public:
    static void openDiff(const QByteArray &diff, DiffParams p, class KTextEditor::MainWindow *mw);
    static void diffDocs(KTextEditor::Document *l, KTextEditor::Document *r, class KTextEditor::MainWindow *mw);

private:
    static DiffWidget *existingDiffWidgetForParams(KTextEditor::MainWindow *mw, const DiffParams &p);
};

class KATE_PRIVATE_EXPORT DiffWidget : public QWidget
{
    Q_OBJECT
    friend DiffWidgetManager;
    friend class DiffWidgetTests;

public:
    explicit DiffWidget(DiffParams p, QWidget *parent = nullptr);
    ~DiffWidget();

    void diffDocs(KTextEditor::Document *l, KTextEditor::Document *r);

    void openDiff(const QByteArray &diff);

    Q_INVOKABLE bool shouldClose()
    {
        return true;
    }

    bool isHunk(int line) const;
    bool isFileNameLine(int line) const
    {
        return m_linesWithFileName.contains(line);
    }
    int hunkLineCount(int hunkLine);

protected:
    void showEvent(QShowEvent *e) override;

private:
    void clearData();
    void handleStyleChange(int);
    void handleStageUnstage(DiffEditor *e, int startLine, int endLine, int actionType, DiffParams::Flag);
    void handleStageUnstage_unified(int startLine, int endLine, int actionType, DiffParams::Flag);
    void handleStageUnstage_sideBySide(DiffEditor *e, int startLine, int endLine, int actionType, DiffParams::Flag);
    void handleStageUnstage_raw(int startLine, int endLine, int actionType, DiffParams::Flag flags);
    void doStageUnStage(int startLine, int endLine, int actionType, DiffParams::Flag);
    void onError(const QByteArray &error, int code);
    void parseAndShowDiff(const QByteArray &raw);
    void parseAndShowDiffUnified(const QByteArray &raw);
    enum ApplyFlags { None = 0, Staged = 1, Discard = 2 };
    void applyDiff(const QString &diff, ApplyFlags flags);
    void runGitDiff();
    static QStringList diffDocsGitArgs(KTextEditor::Document *l, KTextEditor::Document *r);
    void showWithFullContext();

    void jumpToNextFile();
    void jumpToPrevFile();
    void jumpToNextHunk();
    void jumpToPrevHunk();

    // Struct representing a view line to diff line in the original diff
    // Lines which are added or removed are stored, context lines and
    // other metaData is ignored
    struct ViewLineToDiffLine {
        int line;
        int diffLine;
        // Only used in side-by-side mode
        bool added;
    };

    class DiffEditor *m_left;
    class DiffEditor *m_right;
    class QPlainTextEdit *const m_commitInfo;
    class Toolbar *const m_toolbar;
    KSyntaxHighlighting::AbstractHighlighter *leftHl;
    KSyntaxHighlighting::AbstractHighlighter *rightHl;
    DiffStyle m_style = SideBySide;
    DiffParams m_params;
    QByteArray m_rawDiff; // Raw diff saved as is
    QVector<ViewLineToDiffLine> m_lineToRawDiffLine;
    QVector<ViewLineToDiffLine> m_lineToDiffHunkLine;
    QVector<int> m_linesWithFileName;
    bool m_stopScrollSync = false;
    bool m_blockShowEvent = true;
};

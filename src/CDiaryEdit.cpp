/**********************************************************************************************

  DSP Solutions GmbH & Co. KG
  http://www.dspsolutions.de/

  Author:      Oliver Eichler
  Email:       oliver.eichler@dspsolutions.de
  Phone:       +49-941-83055-1
  Fax:         +49-941-83055-79

  File:        CDiaryEdit.cpp

  Module:

  Description:

  Created:     06/10/2011

  (C) 2011 DSP Solutions. All rights reserved.


**********************************************************************************************/

#include "CDiaryEdit.h"
#include "CGeoDB.h"
#include "CDiary.h"
#include "CDiaryDB.h"
#include "CMainWindow.h"
#include "CTabWidget.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CTrack.h"
#include "CTrackDB.h"
#include "QTextHtmlExporter.h"
#include "CMainWindow.h"
#include "CCanvas.h"

#include <QtGui>

class CDiaryEditLock
{
    public:
        CDiaryEditLock(CDiaryEdit * d) : d(d){d->isInternalEdit += 1;}
        ~CDiaryEditLock(){d->isInternalEdit -= 1;}
    private:
        CDiaryEdit * d;
};


CDiaryEdit::CDiaryEdit(CDiary& diary, QWidget * parent)
: QWidget(parent)
, isInternalEdit(0)
, diary(diary)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setupUi(this);

    connect(textEdit, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this, SLOT(slotCurrentCharFormatChanged(const QTextCharFormat &)));
    connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(slotCursorPositionChanged()));

    toolSave->setIcon(QIcon(":/icons/save.png"));
    connect(toolSave, SIGNAL(clicked(bool)), this, SLOT(slotSave()));

    toolReload->setIcon(QIcon(":/icons/refresh.png"));
    connect(toolReload, SIGNAL(clicked(bool)), this, SLOT(slotReload()));

    toolPrint->setIcon(QIcon(":/icons/iconPrint22x22.png"));
    connect(toolPrint, SIGNAL(clicked(bool)), this, SLOT(slotPrintPreview()));

    connect(textEdit, SIGNAL(textChanged()), this, SLOT(setWindowModified()));
    connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));

    toolExit->setIcon(QIcon(":/icons/iconExit16x16.png"));
    connect(toolExit, SIGNAL(clicked(bool)), this, SLOT(close()));

    actionTextBold = new QAction(QIcon(":/icons/textbold.png"), tr("&Bold"), this);
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    connect(actionTextBold, SIGNAL(triggered()), this, SLOT(slotTextBold()));
    actionTextBold->setCheckable(true);
    toolBold->setDefaultAction(actionTextBold);

    actionTextItalic = new QAction(QIcon(":/icons/textitalic.png"), tr("&Italic"), this);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    connect(actionTextItalic, SIGNAL(triggered()), this, SLOT(slotTextItalic()));
    actionTextItalic->setCheckable(true);
    toolItalic->setDefaultAction(actionTextItalic);

    actionTextUnderline = new QAction(QIcon(":/icons/textunder.png"), tr("&Underline"), this);
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    connect(actionTextUnderline, SIGNAL(triggered()), this, SLOT(slotTextUnderline()));
    actionTextUnderline->setCheckable(true);
    toolUnder->setDefaultAction(actionTextUnderline);

    QPixmap pix(24, 24);
    pix.fill(Qt::black);
    actionTextColor = new QAction(pix, tr("&Color..."), this);
    connect(actionTextColor, SIGNAL(triggered()), this, SLOT(slotTextColor()));
    toolColor->setDefaultAction(actionTextColor);

    comboStyle->addItem("standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    connect(comboStyle, SIGNAL(activated(int)), this, SLOT(slotTextStyle(int)));

    QAction *a;
    a = actionUndo = new QAction(QIcon(":/icons/editundo.png"), tr("&Undo"), this);
    a->setShortcut(QKeySequence::Undo);
    toolUndo->setDefaultAction(a);

    a = actionRedo = new QAction(QIcon(":/icons/editredo.png"), tr("&Redo"), this);
    a->setShortcut(QKeySequence::Redo);
    toolRedo->setDefaultAction(a);

    a = actionCut = new QAction(QIcon(":/icons/editcut.png"), tr("Cu&t"), this);
    a->setShortcut(QKeySequence::Cut);
    toolCut->setDefaultAction(a);

    a = actionCopy = new QAction(QIcon(":/icons/editcopy.png"), tr("&Copy"), this);
    a->setShortcut(QKeySequence::Copy);
    toolCopy->setDefaultAction(a);

    a = actionPaste = new QAction(QIcon(":/icons/editpaste.png"), tr("&Paste"), this);
    a->setShortcut(QKeySequence::Paste);
    toolPaste->setDefaultAction(a);

    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
    actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(textEdit->document()->isRedoAvailable());

    connect(textEdit->document(), SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
    connect(textEdit->document(), SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));

    connect(actionUndo, SIGNAL(triggered()), textEdit, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), textEdit, SLOT(redo()));

    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), textEdit, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), textEdit, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), textEdit, SLOT(paste()));

    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));

    textEdit->setFocus();
    colorChanged(textEdit->textColor());
}

CDiaryEdit::~CDiaryEdit()
{
    collectData();
}



void CDiaryEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection())
    {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}

void CDiaryEdit::fontChanged(const QFont &f)
{
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}

void CDiaryEdit::resizeEvent(QResizeEvent * e)
{        
    QWidget::resizeEvent(e);

    CDiaryEditLock lock(this);
    textEdit->clear();
    textEdit->document()->setTextWidth(textEdit->size().width() - 20);
    draw(*textEdit->document());
}

void CDiaryEdit::setWindowModified()
{
    setWindowModified(true);
}

void CDiaryEdit::setWindowModified(bool yes)
{
    if(isInternalEdit || !yes) return;

    emit CDiaryDB::self().emitSigModified(diary.getKey());
    emit CDiaryDB::self().emitSigChanged();

    if(!diary.modified)
    {
        diary.modified = yes;
        setTabTitle();
    }

}

void CDiaryEdit::slotSave()
{
    collectData();

    if(!CGeoDB::self().setProjectDiaryData(diary.keyProjectGeoDB, diary))
    {
        QMessageBox::warning(0, tr("Failed..."), tr("Failed to save diary to database. Probably because it was not created from a database project."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    diary.modified = false;

    setTabTitle();
}

void CDiaryEdit::slotReload()
{
    slotReload(true);
}

void CDiaryEdit::slotReload(bool fromDB)
{
    CDiaryEditLock lock(this);
    if(fromDB)
    {
        if(CGeoDB::self().getProjectDiaryData(diary.keyProjectGeoDB, diary))
        {
            diary.modified = false;
        }
    }

    setTabTitle();

    textEdit->clear();
    textEdit->document()->setTextWidth(textEdit->size().width() - 20);
    draw(*textEdit->document());
}

void CDiaryEdit::slotPrintPreview()
{
    CDiaryEditLock lock(this);
    collectData();

    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Diary"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    QTextDocument doc;
    QSizeF pageSize = printer.pageRect(QPrinter::DevicePixel).size();
    doc.setPageSize(pageSize);
    draw(doc);

    QImage img;
    theMainWindow->getCanvas()->print(img, pageSize.toSize() - QSize(10,10));

    doc.rootFrame()->lastCursorPosition().insertImage(img);
    doc.print(&printer);

    textEdit->clear();
    textEdit->document()->setTextWidth(textEdit->size().width() - 20);
    draw(*textEdit->document());
}

void CDiaryEdit::slotClipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void CDiaryEdit::slotTextBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void CDiaryEdit::slotTextUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void CDiaryEdit::slotTextItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void CDiaryEdit::slotTextColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
    {
        return;
    }
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void CDiaryEdit::slotTextStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();

    if (styleIndex != 0)
    {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex)
        {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList())
        {
            listFmt = cursor.currentList()->format();
        }
        else
        {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    }
    else
    {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}


void CDiaryEdit::slotCurrentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void CDiaryEdit::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    actionTextColor->setIcon(pix);
}


void CDiaryEdit::slotCursorPositionChanged()
{
    if(isInternalEdit) return;

    QTextCursor cursor = textEdit->textCursor();

    if(!diary.diaryFrame.isNull())
    {
        if(diary.diaryFrame->firstCursorPosition() <= cursor && cursor <= diary.diaryFrame->lastCursorPosition())
        {
            return;
        }
    }

    for(int i = 1; i <= diary.wpts.count(); i++)
    {
        if(diary.tblWpt->cellAt(i, eComment).firstCursorPosition() <= cursor && cursor <= diary.tblWpt->cellAt(i, eComment).lastCursorPosition())
        {
            return;
        }

        if(diary.tblWpt->cellAt(i, eSym).firstCursorPosition() <= cursor && cursor <= diary.tblWpt->cellAt(i, eInfo).lastCursorPosition())
        {
            textEdit->setTextCursor(diary.tblWpt->cellAt(i, eComment).lastCursorPosition());
            return;
        }
    }

    for(int i = 1; i <= diary.trks.count(); i++)
    {
        if(diary.tblTrk->cellAt(i, eComment).firstCursorPosition() <= cursor && cursor <= diary.tblTrk->cellAt(i, eComment).lastCursorPosition())
        {
            return;
        }

        if(diary.tblTrk->cellAt(i, eSym).firstCursorPosition() <= cursor && cursor <= diary.tblTrk->cellAt(i, eInfo).lastCursorPosition())
        {
            textEdit->setTextCursor(diary.tblTrk->cellAt(i, eComment).lastCursorPosition());
            return;
        }
    }


    if(!diary.diaryFrame.isNull())
    {
        textEdit->setTextCursor(diary.diaryFrame->lastCursorPosition());
    }

}


void CDiaryEdit::setTabTitle()
{
    CDiaryEditLock lock(this);
    CTabWidget * tab = theMainWindow->getCanvasTab();
    if(tab)
    {
        int idx = tab->indexOf(this);
        if(diary.modified)
        {
            tab->setTabText(idx, tr("Diary - %1 *").arg(diary.getName()));
        }
        else
        {
            tab->setTabText(idx, tr("Diary - %1").arg(diary.getName()));
        }
    }
}

static bool qSortWptLessTime(CWpt * p1, CWpt * p2)
{
    return p1->getTimestamp() < p2->getTimestamp();
}

static bool qSortTrkLessTime(CTrack * t1, CTrack * t2)
{
    return t1->getStartTimestamp() < t2->getStartTimestamp();
}

#define CHAR_PER_LINE 100
#define ROOT_FRAME_MARGIN 5

void CDiaryEdit::draw(QTextDocument& doc)
{
    CDiaryEditLock lock(this);
    QFontMetrics fm(QFont(font().family(),10));

    int cnt;
    int w = doc.textWidth();
    int pointSize = ((10 * (w - 2 * ROOT_FRAME_MARGIN)) / (CHAR_PER_LINE *  fm.width("X")));

    if(pointSize == 0) return;

    QFont f = textEdit->font();
    f.setPointSize(pointSize);
    textEdit->setFont(f);    

    QTextCharFormat fmtCharHeading1;
    fmtCharHeading1.setFont(f);
    fmtCharHeading1.setFontWeight(QFont::Black);
    fmtCharHeading1.setFontPointSize(f.pointSize() + 8);

    QTextCharFormat fmtCharHeading2;
    fmtCharHeading2.setFont(f);
    fmtCharHeading2.setFontWeight(QFont::Black);
    fmtCharHeading2.setFontPointSize(f.pointSize() + 4);

    QTextCharFormat fmtCharStandard;
    fmtCharStandard.setFont(f);

    QTextCharFormat fmtCharHeader;
    fmtCharHeader.setFont(f);
    fmtCharHeader.setBackground(QColor("#c6e3c0"));
    fmtCharHeader.setFontWeight(QFont::Bold);

    QTextBlockFormat fmtBlockStandard;
    fmtBlockStandard.setTopMargin(10);
    fmtBlockStandard.setBottomMargin(10);
    fmtBlockStandard.setAlignment(Qt::AlignJustify);

    QTextFrameFormat fmtFrameStandard;
    fmtFrameStandard.setTopMargin(5);
    fmtFrameStandard.setBottomMargin(5);
    fmtFrameStandard.setWidth(w - 2 * ROOT_FRAME_MARGIN);

    QTextFrameFormat fmtFrameRoot;
    fmtFrameRoot.setTopMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setBottomMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setLeftMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setRightMargin(ROOT_FRAME_MARGIN);

    QTextTableFormat fmtTableStandard;
    fmtTableStandard.setBorder(1);
    fmtTableStandard.setBorderBrush(Qt::black);
    fmtTableStandard.setCellPadding(4);
    fmtTableStandard.setCellSpacing(0);
    fmtTableStandard.setHeaderRowCount(1);
    fmtTableStandard.setTopMargin(10);
    fmtTableStandard.setBottomMargin(20);
    fmtTableStandard.setWidth(w - 2 * ROOT_FRAME_MARGIN);

    QVector<QTextLength> constraints;
    constraints << QTextLength(QTextLength::FixedLength, 32);
    constraints << QTextLength(QTextLength::VariableLength, 50);
    constraints << QTextLength(QTextLength::VariableLength, 100);
    fmtTableStandard.setColumnWidthConstraints(constraints);

    doc.rootFrame()->setFrameFormat(fmtFrameRoot);
    QTextCursor cursor = doc.rootFrame()->firstCursorPosition();

    cursor.insertText(diary.getName(), fmtCharHeading1);
    cursor.setCharFormat(fmtCharStandard);
    cursor.setBlockFormat(fmtBlockStandard);

    diary.diaryFrame = cursor.insertFrame(fmtFrameStandard);
    {
        QTextCursor cursor1(diary.diaryFrame);

        cursor1.setCharFormat(fmtCharStandard);
        cursor1.setBlockFormat(fmtBlockStandard);

        if(diary.getComment().isEmpty())
        {
            cursor1.insertText(tr("Add your own text here..."));
        }
        else
        {
            cursor1.insertHtml(diary.getComment());
        }
        cursor.setPosition(cursor1.position()+1);
    }


    if(!diary.getWpts().isEmpty())
    {
        QList<CWpt*>& wpts = diary.getWpts();
        cursor.insertText(tr("Waypoints"),fmtCharHeading2);

        QTextTable * table = cursor.insertTable(wpts.count()+1, eMax, fmtTableStandard);
        diary.tblWpt = table;
        table->cellAt(0,eSym).setFormat(fmtCharHeader);
        table->cellAt(0,eInfo).setFormat(fmtCharHeader);
        table->cellAt(0,eComment).setFormat(fmtCharHeader);

        table->cellAt(0,eInfo).firstCursorPosition().insertText(tr("Info"));
        table->cellAt(0,eComment).firstCursorPosition().insertText(tr("Comment"));

        cnt = 1;
        qSort(wpts.begin(), wpts.end(), qSortWptLessTime);

        foreach(CWpt * wpt, wpts)
        {


            table->cellAt(cnt,eSym).firstCursorPosition().insertImage(wpt->getIcon().toImage().scaledToWidth(16, Qt::SmoothTransformation));
            table->cellAt(cnt,eInfo).firstCursorPosition().insertText(wpt->getInfo(), fmtCharStandard);

            QTextCursor c = table->cellAt(cnt,eComment).firstCursorPosition();
            c.setCharFormat(fmtCharStandard);
            c.setBlockFormat(fmtBlockStandard);
            c.insertHtml(wpt->getComment());

            cnt++;
        }

        cursor.setPosition(table->lastPosition() + 1);
    }

    if(!diary.getTrks().isEmpty())
    {
        QList<CTrack*>& trks = diary.getTrks();
        cursor.insertText(tr("Tracks"),fmtCharHeading2);

        QTextTable * table = cursor.insertTable(trks.count()+1, eMax, fmtTableStandard);
        diary.tblTrk = table;
        table->cellAt(0,eSym).setFormat(fmtCharHeader);
        table->cellAt(0,eInfo).setFormat(fmtCharHeader);
        table->cellAt(0,eComment).setFormat(fmtCharHeader);

        table->cellAt(0,eInfo).firstCursorPosition().insertText(tr("Info"));
        table->cellAt(0,eComment).firstCursorPosition().insertText(tr("Comment"));

        cnt = 1;
        qSort(trks.begin(), trks.end(), qSortTrkLessTime);

        foreach(CTrack * trk, trks)
        {
            table->cellAt(cnt,eSym).firstCursorPosition().insertImage(trk->getIcon().toImage().scaledToWidth(16, Qt::SmoothTransformation));
            table->cellAt(cnt,eInfo).firstCursorPosition().insertText(trk->getInfo(), fmtCharStandard);

            QTextCursor c = table->cellAt(cnt,eComment).firstCursorPosition();
            c.setCharFormat(fmtCharStandard);
            c.setBlockFormat(fmtBlockStandard);
            c.insertHtml(trk->getComment());

            cnt++;
        }

        cursor.setPosition(table->lastPosition() + 1);
    }

    doc.clearUndoRedoStacks();
}

//static QString toPlainText(const QTextTableCell& cell)
//{
//    QString str;
//    for (QTextFrame::iterator frm = cell.begin(); frm != cell.end(); ++frm)
//    {
//        const QTextBlock& blk = frm.currentBlock();
//        for(QTextBlock::iterator frgm = blk.begin(); frgm != blk.end(); ++frgm)
//        {
//            str += frgm.fragment().text() + "\n";
//        }
//    }

//    return str;
//}

//static QString toPlainText(const QTextFrame& frame)
//{
//    QString str;
//    for (QTextFrame::iterator frm = frame.begin(); frm != frame.end(); ++frm)
//    {
//        const QTextBlock& blk = frm.currentBlock();
//        for(QTextBlock::iterator frgm = blk.begin(); frgm != blk.end(); ++frgm)
//        {
//            str += frgm.fragment().text() + "\n";
//        }
//    }

//    return str;
//}

void CDiaryEdit::collectData()
{
    int cnt;
    if(!diary.diaryFrame.isNull())
    {
        QString comment = QLGT::QTextHtmlExporter(textEdit->document()).toHtml(*diary.diaryFrame);
        diary.setComment(comment.trimmed());
    }
    cnt = 1;
    QList<CWpt*>& wpts = diary.getWpts();
    foreach(CWpt* wpt, wpts)
    {
        wpt->setComment(QLGT::QTextHtmlExporter(textEdit->document()).toHtml(diary.tblWpt->cellAt(cnt, 2)));
        cnt++;
    }

    cnt = 1;
    QList<CTrack*>& trks = diary.getTrks();
    foreach(CTrack* trk, trks)
    {
        trk->setComment(QLGT::QTextHtmlExporter(textEdit->document()).toHtml(diary.tblTrk->cellAt(cnt, 2)));
        cnt++;
    }
}



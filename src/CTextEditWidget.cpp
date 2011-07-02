/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

**********************************************************************************************/
/****************************************************************************
 **
 ** Copyright (C) 2004-2007 Trolltech ASA. All rights reserved.
 **
 ** This file is part of the demonstration applications of the Qt Toolkit.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 2.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of
 ** this file.  Please review the following information to ensure GNU
 ** General Public Licensing requirements will be met:
 ** http://trolltech.com/products/qt/licenses/licensing/opensource/
 **
 ** If you are unsure which license is appropriate for your use, please
 ** review the following information:
 ** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
 ** or contact the sales department at sales@trolltech.com.
 **
 ** In addition, as a special exception, Trolltech gives you certain
 ** additional rights. These rights are described in the Trolltech GPL
 ** Exception version 1.0, which can be found at
 ** http://www.trolltech.com/products/qt/gplexception/ and in the file
 ** GPL_EXCEPTION.txt in this package.
 **
 ** In addition, as a special exception, Trolltech, as the sole copyright
 ** holder for Qt Designer, grants users of the Qt/Eclipse Integration
 ** plug-in the right for the Qt/Eclipse Integration to link to
 ** functionality provided by Qt Designer and its related libraries.
 **
 ** Trolltech reserves all rights not expressly granted herein.
 **
 ** Trolltech ASA (c) 2007
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ****************************************************************************/
#include "CTextEditWidget.h"
#include "CDiaryDB.h"
#include "CWptDB.h"
#include "CWptToolWidget.h"
#include "CTrackDB.h"
#include "CMainWindow.h"
#include "CWpt.h"
#include "CTrack.h"
#include "WptIcons.h"
#include "GeoMath.h"
#include "IUnit.h"
#include "CGeoDB.h"
#include "CTabWidget.h"
#include "CResources.h"
#include "CCanvas.h"


#include <QtGui>
#include <QTextHtmlExporter.h>


CTextEditWidget::CTextEditWidget(QWidget * parent)
: QWidget(parent)
, modified(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setupUi(this);

    actionTextBold = new QAction(QIcon(":/icons/textbold.png"), tr("&Bold"), this);
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    connect(actionTextBold, SIGNAL(triggered()), this, SLOT(textBold()));
    actionTextBold->setCheckable(true);
    toolBold->setDefaultAction(actionTextBold);

    actionTextItalic = new QAction(QIcon(":/icons/textitalic.png"), tr("&Italic"), this);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    connect(actionTextItalic, SIGNAL(triggered()), this, SLOT(textItalic()));
    actionTextItalic->setCheckable(true);
    toolItalic->setDefaultAction(actionTextItalic);

    actionTextUnderline = new QAction(QIcon(":/icons/textunder.png"), tr("&Underline"), this);
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    connect(actionTextUnderline, SIGNAL(triggered()), this, SLOT(textUnderline()));
    actionTextUnderline->setCheckable(true);
    toolUnder->setDefaultAction(actionTextUnderline);

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction *)), this, SLOT(textAlign(QAction *)));

    actionAlignLeft = new QAction(QIcon(":/icons/textleft.png"), tr("&Left"), grp);
    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    toolLeft->setDefaultAction(actionAlignLeft);
    actionAlignCenter = new QAction(QIcon(":/icons/textcenter.png"), tr("C&enter"), grp);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    toolCenter->setDefaultAction(actionAlignCenter);
    actionAlignRight = new QAction(QIcon(":/icons/textright.png"), tr("&Right"), grp);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    toolRight->setDefaultAction(actionAlignRight);
    actionAlignJustify = new QAction(QIcon(":/icons/textjustify.png"), tr("&Justify"), grp);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    toolBlock->setDefaultAction(actionAlignJustify);

    QPixmap pix(24, 24);
    pix.fill(Qt::black);
    actionTextColor = new QAction(pix, tr("&Color..."), this);
    connect(actionTextColor, SIGNAL(triggered()), this, SLOT(textColor()));
    toolColor->setDefaultAction(actionTextColor);

    comboStyle->addItem("standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    connect(comboStyle, SIGNAL(activated(int)), this, SLOT(textStyle(int)));

    connect(comboFont, SIGNAL(activated(const QString &)), this, SLOT(textFamily(const QString &)));

    comboSize->setObjectName("comboSize");
    comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        comboSize->addItem(QString::number(size));

    connect(comboSize, SIGNAL(activated(const QString &)), this, SLOT(textSize(const QString &)));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(QApplication::font().pointSize())));

    connect(textEdit, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this, SLOT(currentCharFormatChanged(const QTextCharFormat &)));
    connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));

    textEdit->setFocus();

    fontChanged(textEdit->font());
    colorChanged(textEdit->textColor());
    alignmentChanged(textEdit->alignment());

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

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

}


CTextEditWidget::~CTextEditWidget()
{
}

void CTextEditWidget::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void CTextEditWidget::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void CTextEditWidget::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void CTextEditWidget::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        textEdit->setAlignment(Qt::AlignLeft);
    else if (a == actionAlignCenter)
        textEdit->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        textEdit->setAlignment(Qt::AlignRight);
    else if (a == actionAlignJustify)
        textEdit->setAlignment(Qt::AlignJustify);
}


void CTextEditWidget::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}


void CTextEditWidget::textSize(const QString &p)
{
    QTextCharFormat fmt;
    fmt.setFontPointSize(p.toFloat());
    mergeFormatOnWordOrSelection(fmt);
}


void CTextEditWidget::textStyle(int styleIndex)
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


void CTextEditWidget::textColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}


void CTextEditWidget::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}


void CTextEditWidget::fontChanged(const QFont &f)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}


void CTextEditWidget::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    actionTextColor->setIcon(pix);
}


void CTextEditWidget::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
    {
        actionAlignLeft->setChecked(true);
    }
    else if (a & Qt::AlignHCenter)
    {
        actionAlignCenter->setChecked(true);
    }
    else if (a & Qt::AlignRight)
    {
        actionAlignRight->setChecked(true);
    }
    else if (a & Qt::AlignJustify)
    {
        actionAlignJustify->setChecked(true);
    }
}


void CTextEditWidget::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}


void CTextEditWidget::cursorPositionChanged()
{
    alignmentChanged(textEdit->alignment());
}



void CTextEditWidget::clipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}


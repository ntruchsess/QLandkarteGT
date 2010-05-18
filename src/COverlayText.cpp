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

#include "COverlayText.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "COverlayDB.h"
#include "CDlgEditText.h"

#include <QtGui>

COverlayText::COverlayText(const QString& text, const QRect& rect, QObject * parent)
: IOverlay(parent, "Text", QPixmap(":/icons/iconText16x16"))
, rect(rect)
, doMove(false)
, doSize(false)
, doSpecialCursor(false)
, sometext(text)

{
    rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
    rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
    rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
    rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

    rectDoc  = QRect(rect.topLeft()     + QPoint(5,20)  , rect.size() - QSize(10, 40));

    rectMouse = rect;
    rectMouse.setTopLeft(rectMouse.topLeft() - QPoint(8,8));
    rectMouse.setBottomRight(rectMouse.bottomRight() + QPoint(8,8));

    doc = new QTextDocument(this);
    doc->setHtml(sometext);
    doc->setPageSize(rectDoc.size());

}


COverlayText::~COverlayText()
{

}


bool COverlayText::isCloseEnough(const QPoint& pt)
{
    return rectMouse.contains(pt);
}


QString COverlayText::getInfo()
{
    QString text = doc->toPlainText();
    if(text.isEmpty())
    {
        return tr("no text");
    }
    else if(text.length() < 40)
    {
        return text;
    }
    else
    {
        return text.left(37) + "...";
    }
}


void COverlayText::draw(QPainter& p)
{
    p.setBrush(Qt::white);
    p.setPen(Qt::black);
    p.drawRect(rect);

    if(selected == this)
    {
        p.setBrush(Qt::white);
        p.setPen(QPen(Qt::red, 2));
        p.drawRect(rect);

        p.drawPixmap(rectMove, QPixmap(":/icons/iconMoveMap16x16.png"));
        p.drawPixmap(rectSize, QPixmap(":/icons/iconSize16x16.png"));
        p.drawPixmap(rectDel, QPixmap(":/icons/iconClear16x16.png"));
        p.drawPixmap(rectEdit, QPixmap(":/icons/iconEdit16x16.png"));
    }
    p.save();
    p.setClipRect(rectDoc);
    p.translate(rectDoc.topLeft());
    doc->drawContents(&p);
    p.restore();
}


void COverlayText::mouseMoveEvent(QMouseEvent * e)
{
    QPoint pos = e->pos();
    if(doMove)
    {
        rect.moveTopLeft(pos);
        rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

        rectMouse = rect;
        rectMouse.setTopLeft(rectMouse.topLeft() - QPoint(8,8));
        rectMouse.setBottomRight(rectMouse.bottomRight() + QPoint(8,8));

        rectDoc  = QRect(rect.topLeft()     + QPoint(5,20)  , rect.size() - QSize(10, 40));
        doc->setPageSize(rectDoc.size());

        theMainWindow->getCanvas()->update();
    }
    else if(doSize)
    {
        rect.setBottomRight(pos);
        rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

        rectMouse = rect;
        rectMouse.setTopLeft(rectMouse.topLeft() - QPoint(8,8));
        rectMouse.setBottomRight(rectMouse.bottomRight() + QPoint(8,8));

        rectDoc  = QRect(rect.topLeft()     + QPoint(5,20)  , rect.size() - QSize(10, 40));
        doc->setPageSize(rectDoc.size());

        theMainWindow->getCanvas()->update();
    }
    else if(rectMove.contains(pos) || rectSize.contains(pos) || rectEdit.contains(pos) || rectDel.contains(pos))
    {
        if(!doSpecialCursor)
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            doSpecialCursor = true;
        }
    }
    else
    {
        if(doSpecialCursor)
        {
            QApplication::restoreOverrideCursor();
            doSpecialCursor = false;
        }
    }
}


void COverlayText::mousePressEvent(QMouseEvent * e)
{
    if(rectMove.contains(e->pos()))
    {
        doMove = true;
    }
    else if(rectSize.contains(e->pos()))
    {
        doSize = true;
    }
    else if(rectEdit.contains(e->pos()))
    {
        CDlgEditText dlg(sometext, theMainWindow->getCanvas());
        dlg.exec();
        doc->setHtml(sometext);
        theMainWindow->getCanvas()->update();
        emit sigChanged();
    }
    else if(rectDel.contains(e->pos()))
    {
        QStringList keys(key);
        COverlayDB::self().delOverlays(keys);
        QApplication::restoreOverrideCursor();
        doSpecialCursor = false;
    }
}


void COverlayText::mouseReleaseEvent(QMouseEvent * e)
{
    if(doSize || doMove)
    {
        emit sigChanged();
    }
    doSize = doMove = false;
}


void COverlayText::save(QDataStream& s)
{
    s << rect << sometext;
}


void COverlayText::load(QDataStream& s)
{
    s >> rect >> sometext;
}

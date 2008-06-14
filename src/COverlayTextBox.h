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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

**********************************************************************************************/

#ifndef COVERLAYTEXTBOX_H
#define COVERLAYTEXTBOX_H

#include "IOverlay.h"
#include <QRect>
#include <QPolygon>
class QPointF;
class QTextDocument;

class COverlayTextBox : public IOverlay
{
    Q_OBJECT;
    public:
        COverlayTextBox(const QString& text, double lon, double lat, const QPoint& anchor, const QRect& rect, QObject * parent);
        virtual ~COverlayTextBox();

        void draw(QPainter& p);

        static QPolygon makePolyline(const QPoint& anchor, const QRect& r);

        QRect getRect();

        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

        void save(QDataStream& s);
        void load(QDataStream& s);

        bool mouseActionInProgress(){return doMove || doSize || doPos;}

        QString getInfo();

    private:
        friend class COverlayDB;
        /// anchor point longitude [rad]
        double lon;
        /// anchor point latitude [rad]
        double lat;
        /// the text box rectangle [px]
        QRect rect;
        /// the anchor point [px]
        QPoint pt;
        /// the resulting polylin, normalized to the anchor point
        QPolygon polyline;

        QRect rectMove;
        QRect rectSize;
        QRect rectEdit;
        QRect rectDel;
        QRect rectAnchor;

        QRect rectDoc;

        QString text;
        QTextDocument * doc;

        bool doMove;
        bool doSize;
        bool doPos;
        bool doSpecialCursor;
};

#endif //COVERLAYTEXTBOX_H


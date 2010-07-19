/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef COVERLAYDISTANCEEDITWIDGET_H
#define COVERLAYDISTANCEEDITWIDGET_H

#include "ui_IOverlayDistanceEditWidget.h"
#include <QWidget>
#include <QPointer>

class COverlayDistance;

class COverlayDistanceEditWidget : public QWidget, private Ui::IOverlayDistanceEditWidget
{
    Q_OBJECT;
    public:
        COverlayDistanceEditWidget(QWidget * parent, COverlayDistance * ovl);
        virtual ~COverlayDistanceEditWidget();

        bool isAboutToClose();

    private slots:
        void slotApply();
        void slotChanged();
        void slotSelectionChanged();
        void slotItemSelectionChanged();
        void slotContextMenu(const QPoint& pos);
        void slotCopy();
        void slotDelete();


    private:
        friend class COverlayDistance;
        enum columns_e {eNo, ePos};

        QPointer<COverlayDistance> ovl;
        QMenu * contextMenu;
};

#endif //COVERLAYDISTANCEEDITWIDGET_H


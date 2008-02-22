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
#ifndef CTRACKTOOLWIDGET_H
#define CTRACKTOOLWIDGET_H

#include <QWidget>
#include "ui_ITrackToolWidget.h"

class QToolBox;
class QListWidgetItem;
class QMenu;

class CTrackToolWidget : public QWidget, private Ui::ITrackToolWidget
{
    Q_OBJECT
    public:
        CTrackToolWidget(QToolBox * parent);
        virtual ~CTrackToolWidget();


    protected:
        void keyPressEvent(QKeyEvent * e);

    private slots:
        void slotDBChanged();
        void slotItemDoubleClicked(QListWidgetItem * item);
        void slotItemClicked(QListWidgetItem * item);
        void slotContextMenu(const QPoint& pos);
        void slotEdit();
        void slotDelete();


    private:
        bool originator;

        QMenu * contextMenu;

};

#endif //CTRACKTOOLWIDGET_H


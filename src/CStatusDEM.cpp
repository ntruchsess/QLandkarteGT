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

#include "CStatusDEM.h"
#include "CMainWindow.h"
#include "CCanvas.h"

#include <QtGui>

CStatusDEM::CStatusDEM(QWidget * parent)
: QWidget(parent)
, overlay(IMap::eNone)
{
    setupUi(this);

    QSettings cfg;
    overlay = (IMap::overlay_e)cfg.value("map/overlay",overlay).toInt();

    switch(overlay)
    {
        case IMap::eNone:
            radioNone->setChecked(true);
            break;
        case IMap::eShading:
            radioShading->setChecked(true);
            break;
        case IMap::eContour:
            radioContour->setChecked(true);
            break;
    }

    connect(radioNone, SIGNAL(clicked(bool)), this, SLOT(slotShowShading()));
    connect(radioShading, SIGNAL(clicked(bool)), this, SLOT(slotShowShading()));
    connect(radioContour, SIGNAL(clicked(bool)), this, SLOT(slotShowShading()));

}


CStatusDEM::~CStatusDEM()
{
    QSettings cfg;
    cfg.setValue("map/overlay",overlay);

}


void CStatusDEM::slotShowShading()
{
    QString button = sender()->objectName();
    if(button == "radioNone")
    {
        overlay = IMap::eNone;
    }
    else if(button == "radioShading")
    {
        overlay = IMap::eShading;
    }
    else if(button == "radioContour")
    {
        overlay = IMap::eContour;
    }

    theMainWindow->getCanvas()->update();
}

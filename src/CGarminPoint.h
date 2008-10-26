/**********************************************************************************************
    Copyright (C) 2006, 2007 Oliver Eichler oliver.eichler@gmx.de

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

  Garmin and MapSource are registered trademarks or trademarks of Garmin Ltd.
  or one of its subsidiaries.

  This source is based on John Mechalas documentation "Garmin IMG File Format" found
  at sourceforge. The missing bits and error were rectified by the source code of
  Konstantin Galichsky (kg@geopainting.com), http://www.geopainting.com

**********************************************************************************************/
#ifndef CGARMINPOINT_H
#define CGARMINPOINT_H

#include <QtGlobal>
#include <QStringList>

class CGarminTile;

class CGarminPoint
{
    public:
        CGarminPoint();
        virtual ~CGarminPoint();

        quint32 decode(qint32 iCenterLon, qint32 iCenterLat, quint32 shift, const quint8 * pData);

        quint16 type;
        bool isLbl6;
        bool hasSubType;

        //QString label;
        double lon;
        double lat;

        QStringList labels;

        quint32 lbl_ptr;
};

#endif //CGARMINPOINT_H


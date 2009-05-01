/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CQlb.h"
#include "CWpt.h"
#include "CTrack.h"
#include "CRoute.h"
#include "CDiary.h"
#include "IOverlay.h"

#include <QtCore>

CQlb::CQlb(QObject * parent)
: QObject(parent)
{

}


CQlb::~CQlb()
{

}


CQlb& CQlb::operator <<(CWpt& wpt)
{
    QDataStream stream(&wpts, QIODevice::Append);
    stream << wpt;

    return *this;
}


CQlb& CQlb::operator <<(CTrack& trk)
{
    QDataStream stream(&trks, QIODevice::Append);
    stream << trk;

    return *this;
}

CQlb& CQlb::operator <<(CRoute& rte)
{
    QDataStream stream(&rtes, QIODevice::Append);
    stream << rte;

    return *this;
}


CQlb& CQlb::operator <<(CDiary& dry)
{
    QDataStream stream(&drys, QIODevice::Append);
    stream << dry;

    return *this;
}


CQlb& CQlb::operator <<(IOverlay& ovl)
{
    QDataStream stream(&ovls, QIODevice::Append);
    stream << ovl;

    return *this;
}


void CQlb::load(const QString& filename)
{
    qint32 type;

    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QDataStream stream(&file);

    stream >> type;
    while(type != eEnd) {

        switch(type) {

            case eWpt:
                stream >> wpts;
                break;

            case eTrack:
                stream >> trks;
                break;

            case eDiary:
                stream >> drys;
                break;

            case eOverlay:
                stream >> ovls;
                break;

            case eRoute:
                stream >> rtes;
                break;

            default:
                file.close();
                return;
        }

        stream >> type;
    }

    file.close();
}


void CQlb::save(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QDataStream stream(&file);

    stream << (qint32)eWpt << wpts;
    stream << (qint32)eTrack << trks;
    stream << (qint32)eRoute << rtes;
    stream << (qint32)eDiary << drys;
    stream << (qint32)eOverlay << ovls;
    stream << (qint32)eEnd;

    file.close();
}

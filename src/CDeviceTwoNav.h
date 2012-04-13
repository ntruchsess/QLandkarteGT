/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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
#ifndef CDEVICETWONAV_H
#define CDEVICETWONAV_H

#include "IDevice.h"

class CDeviceTwoNav : public IDevice
{
    Q_OBJECT;
    public:
        CDeviceTwoNav(QObject * parent);
        virtual ~CDeviceTwoNav();

        void uploadWpts(const QList<CWpt*>& wpts);
        void downloadWpts(QList<CWpt*>& wpts);

        void uploadTracks(const QList<CTrack*>& trks);
        void downloadTracks(QList<CTrack*>& trks);

        void uploadRoutes(const QList<CRoute*>& rtes);
        void downloadRoutes(QList<CRoute*>& rtes);

        void uploadMap(const QList<IMapSelection*>& mss);

        void downloadScreenshot(QImage& image);

    private:
        bool aquire(QDir& dir);
        void readWptFile(const QString &filename, QList<CWpt *> &wpts);

        QString iconTwoNav2QlGt(const QString& sym);
        QString iconQlGt2TwoNav(const QString& sym);

        QString pathRoot;
        QString pathData;
        QString pathDay;

};

#endif //CDEVICETWONAV_H


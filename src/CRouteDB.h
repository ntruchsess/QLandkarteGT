/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CROUTEDB_H
#define CROUTEDB_H

#include "IDB.h"

#include <QMap>
#include <QRectF>
#include <QString>
#include <QPixmap>

class CRoute;
class QDomDocument;

class CRouteDB : public IDB
{
    Q_OBJECT;
    public:
        virtual ~CRouteDB();

        static CRouteDB& self(){return *m_self;}

        void addRoute(CRoute * route, bool silent);

        void delRoute(const QString& key, bool silent);

        CRoute * getRouteByKey(const QString& key);


        void delRoutes(const QStringList& keys);
        /// load database data from gpx
        void loadGPX(CGpx& gpx);
        /// save database data to gpx
        void saveGPX(CGpx& gpx, const QStringList& keys);

        /// load database data from QLandkarte binary
        void loadQLB(CQlb& qlb, bool newKey);
        /// save database data to QLandkarte binary
        void saveQLB(CQlb& qlb);

        void upload(const QStringList& keys);
        void download();
        void clear();

        int count(){return routes.count();}

        const QMap<QString,CRoute*>& getRoutes(){return routes;}

        void draw(QPainter& p, const QRect& rect, bool& needsRedraw);

        void highlightRoute(const QString& key);
        CRoute* highlightedRoute();

        QRectF getBoundingRectF(const QString key);

        struct keys_t{QString key; QString name; QPixmap icon;};
        /// get all keys in the database
        QList<keys_t> keys();

        static bool keyLessThanAlpha(keys_t&  s1, keys_t&  s2);

        void makeVisible(const QStringList& keys);

        void loadSecondaryRoute(const QString& key, QDomDocument& xml);

        void reset(const QString& key);

        signals:
        void sigHighlightRoute(CRoute * route);

    private:
        friend class CMainWindow;

        CRouteDB(QTabWidget * tb, QObject * parent);
        void drawLine(const QPolygon& line, const QRect& extViewport, QPainter& p);

        quint32 cnt;
        static CRouteDB * m_self;

        QMap<QString,CRoute*> routes;
};
#endif                           //CROUTEDB_H

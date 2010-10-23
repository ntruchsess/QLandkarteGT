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
#ifndef CROUTE_H
#define CROUTE_H

#include "IItem.h"

#include <QObject>
#include <QPixmap>
#include <QList>
#include <QPolygon>
#include <QDataStream>
#include <QFile>
#include <projects.h>

class QDomDocument;

class CRoute : public IItem
{
    Q_OBJECT;
    public:
        CRoute(QObject * parent);
        virtual ~CRoute();

        enum type_e {eEnd, eBase, eRtePts};
        struct rtept_t
        {
            float lon;
            float lat;

            QString action;
        };


        /// set the highlight flag
        void setHighlight(bool yes){highlight = yes;}
        /// get the value of the highlight flag
        bool isHighlighted(){return highlight;}
        /// add a new position point
        /**
            @param lon the longitude in degree
            @param lat the latitude in degree
        */
        void addPosition(const double lon, const double lat);

        QPolygon& getPolyline(){return polyline;}
        QPolygon& getPoints(){return points;}


        QVector<rtept_t>& getPriRtePoints(){return priRoute;}
        QVector<rtept_t>& getSecRtePoints(){return secRoute;}

        QRectF getBoundingRectF();

        /// get a summary of item's data to display on screen or in the toolview
        QString getInfo();

        double getDistance(){return dist;}

        /// set the icon defined by a string
        void setIcon(const QString& str);

        void loadSecondaryRoute(QDomDocument& xml);

        signals:
        void sigChanged();

    private:
        friend class CRouteDB;
        friend QDataStream& operator >>(QDataStream& s, CRoute& route);
        friend QDataStream& operator <<(QDataStream& s, CRoute& route);

        void calcDistance();

        /// primary route, just the basic points like A to B via C
        QVector<rtept_t> priRoute;
        /// the actual route distance
        double dist;

        bool highlight;

        /// the Qt polyline for faster processing
        QPolygon polyline;

        QPolygon points;

        bool firstTime;

        /// secondary route with all intermediate points from auto routing
        QVector<rtept_t> secRoute;



};

QDataStream& operator >>(QDataStream& s, CRoute& route);
QDataStream& operator <<(QDataStream& s, CRoute& route);

void operator >>(QFile& f, CRoute& route);
void operator <<(QFile& f, CRoute& route);
#endif                           //CROUTE_H

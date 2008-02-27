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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

**********************************************************************************************/

#ifndef IMAP_H
#define IMAP_H


#include <QObject>
#include <QRect>
#include <QSize>

#include <projects.h>

class QPainter;
class CCanvas;

/// base class to any map render object
class IMap : public QObject
{
    Q_OBJECT
    public:
        IMap(CCanvas * parent);
        virtual ~IMap();

        enum overlay_e {eNone, eShading, eContour};

        /// draw map
        virtual void draw(QPainter& p);
        /// convert a point on the screen [px] to world coordinates [m]
        /**
            The conversion will be done in place.

            @param u x (longitude) value
            @param v y (latitude) value
        */
        virtual void convertPt2M(double& u, double& v) = 0;
        /// convert world coordinates [m] into a point on the screen [px]
        /**
            The conversion will be done in place.

            @param u longitude (x) value
            @param v latitude (y) value
        */
        virtual void convertM2Pt(double& u, double& v) = 0;
        /// convert a point on the screen [px] to geo. coordinates [rad]
        /**
            The conversion will be done in place.

            @param u x (longitude) value
            @param v y (latitude) value
        */
        virtual void convertPt2Rad(double& u, double& v);
        /// convert geo. coordinates [rad] into a point on the screen [px]
        /**
            The conversion will be done in place.

            @param u longitude (x) value
            @param v latitude (y) value
        */
        virtual void convertRad2Pt(double& u, double& v);
        /// move the map [px]
        /**
            @param old the (old) starting point
            @param next the (new) location of the starting point
        */
        virtual void move(const QPoint& old, const QPoint& next) = 0;
        /// zoom map around point [px]
        /**
            @param zoomIn set true to increase resolution and false to decrease resolution
            @param p      define center (steady point) of the transformation
        */
        virtual void zoom(bool zoomIn, const QPoint& p) = 0;
        /// zoom map to fit area
        /**
            @param lon1 the westbound value in [rad]
            @param lat1 the northbound value in [rad]
            @param lon2 the eastbound value in [rad]
            @param lat2 the southbound value in [rad]
        */
        virtual void zoom(double lon1, double lat1, double lon2, double lat2) = 0;
        /// select an area of the map for export [px]
        /**
            @param rect area within the current viewport
        */
        virtual void select(const QRect& rect) = 0;

        /// get the top left and bottom right corner
        /**
            @param lon1 reference to store westbound longitude in [rad]
            @param lat1 reference to store northbound latitude in [rad]
            @param lon2 reference to store eastbound longitude in [rad]
            @param lat2 reference to store southbound latitude in [rad]
        */
        virtual void dimensions(double& lon1, double& lat1, double& lon2, double& lat2) = 0;

        /// get the elevation of the given point
        /**

            @param lon the longitude in [rad]
            @param lat the latitude in [rad]

            @return The elevation at the point or WPT_NOFLOAT if no elevation data is loaded.
        */
        virtual float getElevation(float lon, float lat);

        virtual void setOverlay(enum overlay_e type){overlay = type;}

        virtual overlay_e getOverlay(){return overlay;}

        virtual const QString& getFilename(){return filename;}
    public slots:
        /// change visible size of map
        /**
            @param size size of the new viewport (display) area [px]
        */
        virtual void resize(const QSize& size);


    protected:
        QString filename;
        /// canvas / viewport rectangle [px]
        QRect rect;
        /// canvas / viewport size
        QSize size;
        /// absolute zoom factor (1...x)
        qint32 zoomidx;

        /// source projection of the current map file
        /**
            Has to be set by subclass. Destruction has to be
            handeled by subclass.
        */
        PJ * pjsrc;
        /// target projection
        /**
            Is set by IMap() to WGS84. Will be freed by ~IMap()
        */
        PJ * pjtar;

        overlay_e overlay;
};

#endif //IMAP_H


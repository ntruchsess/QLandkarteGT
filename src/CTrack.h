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

#ifndef CTRACK_H
#define CTRACK_H

#include <QObject>
#include <QVector>
#include <QColor>
#include <QPolygon>
#include <QDateTime>
#include "CWpt.h"

#include "CGpxExtension.h" //TODO: include von gpx ext

#define MAX_TRACK_SIZE 10000

class CFlags
{
    public:
        CFlags(quint32 f=0) { flags = f; changed = true; };
        virtual ~CFlags() {};
        const quint32 flag() const { return flags; };
        void setFlags( quint32 f ) { if ( flags != f ) changed = true; flags = f; };
        quint32 operator  & (quint32 f) const { return flags&f; };
        quint32 operator |= (quint32 f) { if ( flags != (flags|f) ) changed = true; flags|=f; return flags; };
        quint32 operator &= (quint32 f) { if ( flags != (flags&f) ) changed = true; flags&=f; return flags; };
        quint32 operator >> (quint32 & f) { if ( flags != f ) changed = true; flags = f; return flags; };
        const bool isChanged() const { return changed; };
        void setChanged(bool b) { changed = b; };
    protected:
        /// display flags
        quint32 flags;
        bool changed;
};

QDataStream& operator >>(QDataStream& s, CFlags& flag);
QDataStream& operator <<(QDataStream& s, CFlags& flag);

class CTrack : public QObject
{
    Q_OBJECT;
    public:
        CTrack(QObject * parent);

	    CGpxExtTr tr_ext;		//TODO: CGpxExtPt -> tr_ext		

        virtual ~CTrack();
        int ref;
        enum type_e {eEnd,eBase,eTrkPts,eTrain,eTrkExt1};

        struct pt_t
        {
            enum flag_e
            {
                eSelected  = 1   ///< selected by track info view
                ,eCursor    = 2  ///< selected by cursor
                ,eDeleted   = 4  ///< mark point as deleted
                ,eFocus     = 8  ///< mark current point of user focus
            };

            pt_t() : idx(-1), lon(WPT_NOFLOAT), lat(WPT_NOFLOAT), ele(WPT_NOFLOAT), timestamp(0), timestamp_msec(0),
                speed(WPT_NOFLOAT), avgspeed(0), delta(WPT_NOFLOAT), azimuth(WPT_NOFLOAT), distance(WPT_NOFLOAT),
                ascend(0), descend(0), heartReateBpm(-1), cadenceRpm(-1), slope(0),
                fix(""), sat(0), velocity(WPT_NOFLOAT), heading(WPT_NOFLOAT),
                vdop(WPT_NOFLOAT), hdop(WPT_NOFLOAT), pdop(WPT_NOFLOAT), 
                flags(0), px_valid(FALSE), dem(WPT_NOFLOAT), editItem(NULL){}

			

            bool operator==(const pt_t& pt){return pt.idx == idx;}

            /// index counter for easy QVector access
            qint32  idx;
            /// longitude [deg]
            float   lon;
            /// latitude [deg]
            float   lat;
            /// elevation [m]
            float   ele;
            /// timestamp for track creation
            quint32 timestamp;
            quint32 timestamp_msec;

            /// secondary data: the speed between this and the previous point
            float speed;
            /// secondary data: the short term average speed
            float avgspeed;
            /// secondary data: the distance between this and the previous point
            float delta;
            /// secondary data: the azimuth to the next point
            double azimuth;
            /// secondary data: the total distance of all visible points up to this point
            float distance;
            /// secondary data: the total ascend of all visible points up to this point
            float ascend;
            /// secondary data: the total descend of all visible points up to this point
            float descend;
            /// secondary data: the heart rate in bpm
            int heartReateBpm;
            /// secondary data: cadence in rpm
            int cadenceRpm;
            /// secondary data: slope in %
            float slope;

            // extended data 1
            QString fix;
            qint32  sat;
            float   altitude;    ///< [m] Altitude, Meters, above mean sea level
            float   height;      ///< [m] Height of geoid (mean sea level) above WGS84 ellipsoid
            float   velocity;    ///< [m/s] Ground speed, meters per hour
            float   heading;     ///< [] Track angle in degrees True
            float   magnetic;    ///< [] Magnetic Variation
            float   vdop;        ///< Vertical dilution of precision (VDOP)
            float   hdop;        ///< Horizontal dilution of precision (HDOP)
            float   pdop;        ///< PDOP (dilution of precision)
            float   x;           ///< [m] cartesian gps coordinate
            float   y;           ///< [m] cartesian gps coordinate
            float   z;           ///< [m] cartesian gps coordinate
            float   vx;          ///< [m/s] velocity
            float   vy;          ///< [m/s] velocity
            float   vz;          ///< [m/s] velocity
		
			CGpxExtPt gpx_exts;		//TODO: CGpxExtPt -> gpx_exts

            /// display flags
            CFlags flags;
            /// the current location in pixel
            QPoint px;
            bool px_valid;

            float  dem;

            /// QTreeWidgetItem
            QObject * editItem;
        };

        /// set color by id
        void setColor(unsigned i);
        /// get QT color
        const QColor& getColor(){return color;}
        const QPixmap& getBullet(){return bullet;}
        unsigned getColorIdx(){return colorIdx;}
        /// set track name
        void setName(const QString& n){name = n;}
        /// get track name
        const QString& getName(){return name;}
        /// get unique track key
        const QString& key();

        /// set the highlight flag
        void setHighlight(bool yes){highlight = yes;}
        /// get the value of the highlight flag
        bool isHighlighted(){return highlight;}

        /// append point to track
        CTrack& operator<<(pt_t& pt);
        /// rebuild secondary track data from primary
        void rebuild(bool reindex);
        /// get list of track points
        QList<pt_t>& getTrackPoints(){return track;};
        /// get polyline representation of track
        QPolygon& getPolyline(){return polyline;}
        /// get the total distance of the track in [m]
        double getTotalDistance(){return totalDistance;}
        /// get the total time covered by the track in seconds
        int getTotalTime(){return totalTime;}
        /// select tarckpoint by index
        void setPointOfFocus(int idx);
        ///
        QDateTime getStartTimestamp();
        QDateTime getEndTimestamp();
        /// get the ascend in [m]
        double getAscend(){return totalAscend;}
        /// get the descend in [m]
        double getDescend(){return totalDescend;}

        QRectF getBoundingRectF();
        void sortByTimestamp();
        CTrack& operator+=(const CTrack& trk);

        static const QColor lineColors[];
        static const QString bulletColors[];

        /// track name
        QString name;

        bool hasTraineeData() { return traineeData;};
        void setTraineeData() { traineeData = true;};
        bool hasExt1Data() { return ext1Data;};
        void setExt1Data() { ext1Data = true;};

        void hide(bool ok);
        bool isHidden(){return m_hide;}

        signals:
        void sigChanged();

    private:
        friend class CTrackDB;
        friend QDataStream& operator >>(QDataStream& s, CTrack& track);
        friend QDataStream& operator <<(QDataStream& s, CTrack& track);
        void genKey();

        static QDir path;
        static quint32 keycnt;

        /// unique key to address tarck
        QString _key_;
        /// creation timestamp
        quint32 timestamp;
        /// a comment string
        QString comment;
        /// a track URL
        QString url;
        /// the track line color
        QColor  color;

        QPixmap bullet;
        /// the track line color by index
        unsigned colorIdx;
        /// the track points
        QList<pt_t> track;

        /// set true to draw track highlighted
        bool highlight;

        /// total time covered by all track points
        quint32 totalTime;
        /// total distance of track [m]
        double  totalDistance;

        /// total ascend in [m]
        double totalAscend;
        /// total descend in [m]
        double totalDescend;

        /// the Qt polyline for faster processing
        QPolygon polyline;

        float avgspeed0;
        float avgspeed1;

        bool traineeData;
        bool ext1Data;

        bool firstTime;

        bool m_hide;

};

QDataStream& operator >>(QDataStream& s, CTrack& track);
QDataStream& operator <<(QDataStream& s, CTrack& track);

void operator >>(QFile& f, CTrack& track);
void operator <<(QFile& f, CTrack& track);
#endif                           //CTRACK_H

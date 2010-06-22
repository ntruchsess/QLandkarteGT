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
#include <stdio.h>

#include "CTrackDB.h"
#include "CTrack.h"
#include "CTrackToolWidget.h"
#include "CQlb.h"
#include "CGpx.h"
#include "CResources.h"
#include "IDevice.h"
#include "CDlgCombineTracks.h"
#include "CMapDB.h"
#include "IMap.h"

#include <QtGui>
#include "CUndoStackModel.h"
#include "CTrackUndoCommandDelete.h"
#include "CTrackUndoCommandSelect.h"

#if WIN32
#include <math.h>
#include <float.h>
#ifndef __MINGW32__
typedef __int32 int32_t;
#endif
#define isnan _isnan
#define FP_NAN NAN
#endif

CTrackDB * CTrackDB::m_self = 0;

bool CTrackDB::keyLessThanAlpha(CTrackDB::keys_t&  s1, CTrackDB::keys_t&  s2)
{
    return s1.name.toLower() < s2.name.toLower();
}


CTrackDB::CTrackDB(QTabWidget * tb, QObject * parent)
: IDB(tb,parent)
, cnt(0)
, showBullets(true)
{
    m_self      = this;

    QSettings cfg;
    showBullets = cfg.value("tracks/showBullets", showBullets).toBool();
    toolview    = new CTrackToolWidget(tb);
    undoStack   = CUndoStackModel::getInstance();

}


CTrackDB::~CTrackDB()
{
    QSettings cfg;
    cfg.setValue("tracks/showBullets", showBullets);
}


void CTrackDB::clear()
{
    cnt = 0;
    delTracks(tracks.keys());
    emit sigChanged();
}


CTrackToolWidget * CTrackDB::getToolWidget()
{
    return qobject_cast<CTrackToolWidget*>(toolview);
}


QRectF CTrackDB::getBoundingRectF(const QString key)
{
    if(!tracks.contains(key))
    {
        return QRectF();
    }
    return tracks.value(key)->getBoundingRectF();
}


QRectF CTrackDB::getBoundingRectF()
{
    QRectF r;
    foreach(CTrack *track, tracks.values())
    {
        r = r.united(track->getBoundingRectF());
    }
    return r;
}


void CTrackDB::loadQLB(CQlb& qlb)
{
    QDataStream stream(&qlb.tracks(),QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_5);

    while(!stream.atEnd())
    {
        CTrack * track = new CTrack(this);
        stream >> *track;
        addTrack(track, true);
    }

    emit sigChanged();

}


void CTrackDB::loadQLB(CQlb& qlb, bool asDuplicat)
{
    QDataStream stream(&qlb.tracks(),QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_5);

    while(!stream.atEnd())
    {
        CTrack * track = new CTrack(this);
        stream >> *track;
        if(asDuplicat)
        {
            track->genKey();
        }
        addTrack(track, true);
    }

    emit sigChanged();

}


void CTrackDB::saveQLB(CQlb& qlb)
{
    QMap<QString, CTrack*>::const_iterator track = tracks.begin();
    while(track != tracks.end())
    {
        qlb << *(*track);
        ++track;
    }
}


void CTrackDB::loadGPX(CGpx& gpx)
{
    QDomElement tmpelem;
    QDomElement trk = gpx.firstChildElement("gpx").firstChildElement("trk");
    while (!trk.isNull())
    {
        CTrack* track = new CTrack(this);
                                 //preset a random color
        track->setColor((rand() % 13)+1);

        /*
         *  Global track information
         */

        QMap<QString,QDomElement> trkmap = CGpx::mapChildElements(trk);

        // GPX 1.1

        tmpelem = trkmap.value("name");
        if(!tmpelem.isNull()) track->setName(tmpelem.text());

        tmpelem = trkmap.value("desc");
        if(!tmpelem.isNull()) track->comment = tmpelem.text();

        tmpelem = trkmap.value("link");
        if(!tmpelem.isNull())
        {
            track->url = tmpelem.attribute("href");

            // For now we are ignoring the following elements:
            // * text - hyperlink text (string)
            // * type - content mime type (string)
            // When the URL is somehow supported, we may need those
        }

        // For now we are ignoring the following elements:
        // * cmt - GPS comment (string)
        // * src - data source (string)
        // * number - track number (integer)
        // * type - track classification (string)
        // I haven't seen any software using those, but who knows

        tmpelem = trkmap.value("extensions");
        if(!tmpelem.isNull())
        {
            QMap<QString,QDomElement> extensionsmap = CGpx::mapChildElements(tmpelem);

            // Garmin extensions v3

            tmpelem = extensionsmap.value(CGpx::gpxx_ns + ":" + "TrackExtension");
            if(!tmpelem.isNull())
            {
                QMap<QString,QDomElement> trackextensionmap = CGpx::mapChildElements(tmpelem);

                tmpelem = trackextensionmap.value(CGpx::gpxx_ns + ":" + "DisplayColor");
                if (!tmpelem.isNull())
                {
                    int colorID = gpx.getTrackColorMap().right(tmpelem.text(), -1);
                    if (colorID >= 0) track->setColor(colorID);
                }
            }
        }

        // QLandkarteGT backward compatibility

        if (gpx.version() == CGpx::qlVer_1_0)
        {
            tmpelem = trkmap.value("extension");
            if(!tmpelem.isNull())
            {
                QMap<QString,QDomElement> trkextensionmap = CGpx::mapChildElements(tmpelem);

                tmpelem = trkextensionmap.value("color");
                if(!tmpelem.isNull()) track->setColor(tmpelem.text().toUInt());
            }
        }

        /*
         *  Trackpoint information
         */

        bool foundTraineeData = false;

        QDomElement trkseg = trk.firstChildElement("trkseg");
        while(!trkseg.isNull())
        {
            QDomElement trkpt = trkseg.firstChildElement("trkpt");
            while (!trkpt.isNull())
            {
                CTrack::pt_t pt;

                QMap<QString,QDomElement> trkptmap = CGpx::mapChildElements(trkpt);

                // GPX 1.1

                pt.lon = trkpt.attribute("lon").toDouble();
                pt.lat = trkpt.attribute("lat").toDouble();

                tmpelem = trkptmap.value("ele");
                if(!tmpelem.isNull()) pt.ele = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("time");
                if(!tmpelem.isNull())
                {
                    QString timetext = tmpelem.text();

                    QString format = "yyyy-MM-dd'T'hh:mm:ss";
                    if (timetext.indexOf(".") != -1) format += ".zzz";
                                 // bugfix for badly coded gpx files
                    if (timetext.indexOf("Z") != -1) format += "'Z'";

                    QDateTime datetime = QDateTime::fromString(timetext, format);
                    datetime.setTimeSpec(Qt::UTC);

                    pt.timestamp = datetime.toTime_t();
                    pt.timestamp_msec = datetime.time().msec();
                }

                tmpelem = trkptmap.value("fix");
                if(!tmpelem.isNull()) pt.fix = tmpelem.text();

                tmpelem = trkptmap.value("sat");
                if(!tmpelem.isNull()) pt.sat = tmpelem.text().toUInt();

                tmpelem = trkptmap.value("hdop");
                if(!tmpelem.isNull()) pt.hdop = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("vdop");
                if(!tmpelem.isNull()) pt.vdop = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("pdop");
                if(!tmpelem.isNull()) pt.pdop = tmpelem.text().toDouble();

                // For now we are ignoring the following elements:
                // * magvar - magnetic variation (degrees)
                // * geoidheight - height of geoid above WGS84 (meters)
                // * name - waypoint name (string)
                // * cmt - GPS waypoint comment (string)
                // * desc - desctiption (string)
                // * src - data source (string)
                // * link - link to additional information (link)
                // * sym - symbol name (string)
                // * type - waypoint type (string)
                // * ageofdgpsdata - seconds since last DGPS update (decimal)
                // * dgpsid - DGPS station ID (ID)

                // GPX 1.0 backward compatibility, to be kept

                tmpelem = trkptmap.value("course");
                if(!tmpelem.isNull()) pt.heading = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("speed");
                if(!tmpelem.isNull()) pt.velocity = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("extensions");
                if(!tmpelem.isNull())
                {
#ifdef GPX_EXTENSIONS
                                 //TODO: Abholen der Extension
                    pt.gpx_exts.setValues(tmpelem);

                                 //TODO: Auslesen der Namen der Ext
                    track->tr_ext.addKey2List(tmpelem);
#endif

                    QMap<QString,QDomElement> extensionsmap = CGpx::mapChildElements(tmpelem);

                    // Garmin extensions v3

                    // For now we are ignoring the following elements:
                    // * Depth - depth (meters)
                    // * Temperature - temperature (Celsius)

                    // Garmin Trackpoint Extension v1

                    // For now we are ignoring the following elements:
                    // * atemp - ambient temperature (Celsius)
                    // * wtemp - water temperature (Celsius)
                    // * depth - depth (meters)

                    tmpelem = extensionsmap.value(CGpx::gpxtpx_ns + ":" + "TrackPointExtension");
                    if(!tmpelem.isNull())
                    {
                        QMap<QString,QDomElement> trackpointextensionmap = CGpx::mapChildElements(tmpelem);

                        tmpelem = trackpointextensionmap.value(CGpx::gpxtpx_ns + ":" + "hr");
                        if(!tmpelem.isNull())
                        {
                            pt.heartReateBpm = tmpelem.text().toUInt();
                            foundTraineeData = true;
                        }

                        tmpelem = trackpointextensionmap.value(CGpx::gpxtpx_ns + ":" + "cad");
                        if(!tmpelem.isNull())
                        {
                            pt.cadenceRpm = tmpelem.text().toUInt();
                            foundTraineeData = true;
                        }
                    }

                    // TrekBuddy extensions

                    tmpelem = extensionsmap.value(CGpx::rmc_ns + ":" + "course");
                    if(!tmpelem.isNull()) pt.heading = tmpelem.text().toDouble();

                    tmpelem = extensionsmap.value(CGpx::rmc_ns + ":" + "speed");
                    if(!tmpelem.isNull()) pt.velocity = tmpelem.text().toDouble();

                    // QLandkarteGT extensions

                    if (gpx.version() >= CGpx::qlVer_1_1)
                    {
                        tmpelem = extensionsmap.value(CGpx::ql_ns + ":" + "flags");
                        if(!tmpelem.isNull())
                        {
                            pt.flags = tmpelem.text().toUInt();
                            pt.flags &= ~CTrack::pt_t::eFocus;
                            pt.flags &= ~CTrack::pt_t::eSelected;
                            pt.flags &= ~CTrack::pt_t::eCursor;
                        }
                    }
                }

                // QLandkarteGT backward compatibility

                if (gpx.version() == CGpx::qlVer_1_0)
                {
                    tmpelem = trkptmap.value("extension");
                    if(!tmpelem.isNull())
                    {
                        QMap<QString,QDomElement> extensionmap = CGpx::mapChildElements(tmpelem);

                        tmpelem = extensionmap.value("flags");
                        if(!tmpelem.isNull())
                        {
                            pt.flags = tmpelem.text().toUInt();
                            pt.flags &= ~CTrack::pt_t::eFocus;
                            pt.flags &= ~CTrack::pt_t::eSelected;
                            pt.flags &= ~CTrack::pt_t::eCursor;
                        }
                    }
                }

                *track << pt;
                trkpt = trkpt.nextSiblingElement("trkpt");
            }

            trkseg = trkseg.nextSiblingElement("trkseg");
        }

        if (foundTraineeData) track->setTraineeData();

        if(track->getTrackPoints().count() > 0) addTrack(track, true);
        else delete track;

        trk = trk.nextSiblingElement("trk");
    }

    emit sigChanged();
}


void CTrackDB::saveGPX(CGpx& gpx, const QStringList& keys)
{
    QString str;
    QDomElement root = gpx.documentElement();
    QMap<QString,CTrack*>::iterator track = tracks.begin();
    while(track != tracks.end())
    {
        if(!keys.isEmpty() && !keys.contains((*track)->key()))
        {
            ++track;
            continue;
        }

        QDomElement trk = gpx.createElement("trk");
        root.appendChild(trk);

        QDomElement name = gpx.createElement("name");
        trk.appendChild(name);
        QDomText _name_ = gpx.createTextNode((*track)->getName());
        name.appendChild(_name_);

        QDomElement ext = gpx.createElement("extensions");
        trk.appendChild(ext);

        QDomElement gpxx_ext = gpx.createElement("gpxx:TrackExtension");
        ext.appendChild(gpxx_ext);

        QDomElement color = gpx.createElement("gpxx:DisplayColor");
        gpxx_ext.appendChild(color);

        QString colname = gpx.getTrackColorMap().left((*track)->getColorIdx());
        QDomText _color_ = gpx.createTextNode(colname);
        color.appendChild(_color_);

        QDomElement trkseg = gpx.createElement("trkseg");
        trk.appendChild(trkseg);

        QList<CTrack::pt_t>& pts = (*track)->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator pt = pts.begin();
        while(pt != pts.end())
        {
            QDomElement trkpt = gpx.createElement("trkpt");
            if (gpx.getExportFlag() && (pt->flags.flag() & CTrack::pt_t::eDeleted))
            {
                // skip deleted points when exporting
                ++pt;
                continue;
            }
            trkseg.appendChild(trkpt);
            str.sprintf("%1.8f", pt->lat);
            trkpt.setAttribute("lat",str);
            str.sprintf("%1.8f", pt->lon);
            trkpt.setAttribute("lon",str);

            if(pt->ele != WPT_NOFLOAT)
            {
                QDomElement ele = gpx.createElement("ele");
                trkpt.appendChild(ele);
                QDomText _ele_ = gpx.createTextNode(QString::number(pt->ele));
                ele.appendChild(_ele_);
            }

            if(pt->timestamp != 0x000000000 && pt->timestamp != 0xFFFFFFFF)
            {
                QDateTime t = QDateTime::fromTime_t(pt->timestamp).toUTC();
                t = t.addMSecs(pt->timestamp_msec);
                QDomElement time = gpx.createElement("time");
                trkpt.appendChild(time);
                QDomText _time_ = gpx.createTextNode(t.toString("yyyy-MM-dd'T'hh:mm:ss.zzz'Z'"));
                time.appendChild(_time_);
            }

            if(pt->hdop != WPT_NOFLOAT)
            {
                QDomElement hdop = gpx.createElement("hdop");
                trkpt.appendChild(hdop);
                QDomText _hdop_ = gpx.createTextNode(QString::number(pt->hdop));
                hdop.appendChild(_hdop_);
            }
            if(pt->vdop != WPT_NOFLOAT)
            {
                QDomElement vdop = gpx.createElement("vdop");
                trkpt.appendChild(vdop);
                QDomText _vdop_ = gpx.createTextNode(QString::number(pt->vdop));
                vdop.appendChild(_vdop_);
            }
            if(pt->pdop != WPT_NOFLOAT)
            {
                QDomElement pdop = gpx.createElement("pdop");
                trkpt.appendChild(pdop);
                QDomText _pdop_ = gpx.createTextNode(QString::number(pt->pdop));
                pdop.appendChild(_pdop_);
            }

            if(pt->fix != "")
            {
                QDomElement fix = gpx.createElement("fix");
                trkpt.appendChild(fix);
                QDomText _fix_ = gpx.createTextNode(pt->fix);
                fix.appendChild(_fix_);
            }

            if(pt->sat != 0)
            {
                QDomElement sat = gpx.createElement("sat");
                trkpt.appendChild(sat);
                QDomText _sat_ = gpx.createTextNode(QString::number(pt->sat));
                sat.appendChild(_sat_);
            }

            // gpx extensions
            if((!gpx.getExportFlag() && pt->flags.flag() != 0) ||
                pt->heading != WPT_NOFLOAT ||
                pt->velocity != WPT_NOFLOAT)
            {
                QDomElement extensions = gpx.createElement("extensions");
                trkpt.appendChild(extensions);

                if(!gpx.getExportFlag() && pt->flags.flag() != 0)
                {
                    QDomElement flags = gpx.createElement("ql:flags");
                    extensions.appendChild(flags);
                    QDomText _flags_ = gpx.createTextNode(QString::number(pt->flags.flag()));
                    flags.appendChild(_flags_);
                }

                if(pt->heading != WPT_NOFLOAT)
                {
                    QDomElement heading = gpx.createElement("rmc:course");
                    extensions.appendChild(heading);
                    QDomText _heading_ = gpx.createTextNode(QString::number(pt->heading));
                    heading.appendChild(_heading_);
                }

                if(pt->velocity != WPT_NOFLOAT)
                {
                    QDomElement velocity = gpx.createElement("rmc:speed");
                    extensions.appendChild(velocity);
                    QDomText _velocity_ = gpx.createTextNode(QString::number(pt->velocity));
                    velocity.appendChild(_velocity_);
                }
            }

            ++pt;
        }

        ++track;
    }

}


void CTrackDB::addTrack(CTrack* track, bool silent)
{
    if(track->getName().isEmpty())
    {
        track->setName(tr("Track%1").arg(cnt++));
    }
    track->rebuild(false);
    delTrack(track->key(), silent);
    tracks[track->key()] = track;

    connect(track,SIGNAL(sigChanged()),SIGNAL(sigChanged()));
    if(!silent)
    {
        emit sigChanged();
        emit sigModified();
    }
}


void CTrackDB::delTrack(const QString& key, bool silent)
{
    if(!tracks.contains(key)) return;
    undoStack->push(new CTrackUndoCommandDelete(this,key,silent));
}


void CTrackDB::delTracks(const QStringList& keys)
{
    undoStack->beginMacro("delTracks");
    foreach(QString key,keys)
    {
        delTrack(key,false);
    }
    undoStack->endMacro();
}


void CTrackDB::highlightTrack(const QString& key)
{
    QMap<QString,CTrack*>::iterator track = tracks.begin();
    while(track != tracks.end())
    {
        (*track)->setHighlight(false);
        ++track;
    }

    if(tracks.contains(key))
    {
        tracks[key]->setHighlight(true);
        emit sigHighlightTrack(tracks[key]);
    }
    else
    {
        emit sigHighlightTrack(0);
    }

    emit sigChanged();

}


void CTrackDB::hideTrack(const QStringList& keys, bool hide)
{
    QString key;
    foreach(key, keys)
    {
        if(tracks.contains(key))
        {
            CTrack * track = tracks[key];
            track->hide(hide);
            if(track->isHighlighted() && hide)
            {
                tracks[key]->setHighlight(false);
                emit sigHighlightTrack(tracks[key]);
            }
        }
    }
    emit sigChanged();
}


CTrack* CTrackDB::highlightedTrack()
{

    QMap<QString,CTrack*>::iterator track = tracks.begin();
    while(track != tracks.end())
    {
        if((*track)->isHighlighted()) return *track;
        ++track;
    }
    return 0;

}


void CTrackDB::upload(const QStringList& keys)
{
    if(tracks.isEmpty()) return;

    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<CTrack*> tmptrks;

        if(keys.isEmpty())
        {
            tmptrks = tracks.values();
        }
        else
        {
            QString key;
            foreach(key, keys)
            {
                tmptrks << tracks[key];
            }
        }
        dev->uploadTracks(tmptrks);
    }
}


void CTrackDB::download()
{
    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<CTrack*> tmptrk;
        dev->downloadTracks(tmptrk);

        if(tmptrk.isEmpty()) return;

        CTrack * trk;
        foreach(trk,tmptrk)
        {
            addTrack(trk, true);
        }
    }

    emit sigChanged();
    emit sigModified();
}


void CTrackDB::CombineTracks()
{
    CDlgCombineTracks dlg(0);
    dlg.exec();
}


void CTrackDB::splitTrack(int idx)
{
    CTrack * theTrack = highlightedTrack();
    if(theTrack == 0) return;

    int i;
    QList<CTrack::pt_t>& track          = theTrack->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = track.begin();

    CTrack * track1 = new CTrack(this);
    track1->setName(theTrack->getName() + "_1");
    for(i = 0; (i <= idx) && (trkpt != track.end()); ++i)
    {
        *track1 << *trkpt++;
    }

    CTrack * track2 = new CTrack(this);
    track2->setName(theTrack->getName() + "_2");
    for( ;(trkpt != track.end()); ++i)
    {
        *track2 << *trkpt++;
    }

    addTrack(track1, true);
    addTrack(track2, true);
    delTrack(theTrack->key(), true);

    emit sigChanged();
    emit sigModified();
}


void CTrackDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{
    QPointF arrow[4] =
    {
        QPointF( 20.0, 7.0),     //front
        QPointF( 0.0, 0.0),      //upper tail
        QPointF( 5.0, 7.0),      //mid tail
        QPointF( 0.0, 15.0)      //lower tail
    };

    QPoint focus(-1,-1);
    QVector<QPoint> selected;
    IMap& map = CMapDB::self().getMap();

    /// @todo it would be nice to use antialiasing here, but right now performance is bad
    p.setRenderHint(QPainter::Antialiasing, !map.getFastDrawFlag());
    p.setRenderHint(QPainter::Antialiasing, false);

    //     QMap<QString,CTrack*> tracks                = CTrackDB::self().getTracks();
    QMap<QString,CTrack*>::iterator track       = tracks.begin();
    QMap<QString,CTrack*>::iterator highlighted = tracks.end();

    QPixmap bullet_red(":/icons/bullet_red.png");

    while(track != tracks.end())
    {

        if((*track)->m_hide)
        {

            ++track;
            continue;
        }

        QPolygon& line = (*track)->getPolyline();
        line.clear();

        bool firstTime = (*track)->firstTime;

        QList<CTrack::pt_t>& trkpts = (*track)->getTrackPoints();
        QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();
        while(trkpt != trkpts.end())
        {

            if ( needsRedraw || firstTime)
            {
                double u = trkpt->lon * DEG_TO_RAD;
                double v = trkpt->lat * DEG_TO_RAD;

                map.convertRad2Pt(u,v);
                trkpt->px = QPoint(u,v);

            }

            if((*track)->isHighlighted() && trkpt->flags & CTrack::pt_t::eSelected)
            {
                selected << trkpt->px;
            }

            if((*track)->isHighlighted() && trkpt->flags & CTrack::pt_t::eFocus)
            {
                focus = trkpt->px;
            }

            // skip deleted points, however if they are selected the
            // selection mark is shown
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }

            line << trkpt->px;
            ++trkpt;
        }

        if(!rect.intersects(line.boundingRect()))
        {
            ++track; continue;
        }

        if((*track)->isHighlighted())
        {
            // store highlighted track to draw it later
            // it must be drawn above all other tracks
            highlighted = track;
        }
        else
        {
            // draw normal track
            QPen pen1(Qt::white,5);
            pen1.setCapStyle(Qt::RoundCap);
            pen1.setJoinStyle(Qt::RoundJoin);

            QPen pen2((*track)->getColor(),3);
            pen2.setCapStyle(Qt::RoundCap);
            pen2.setJoinStyle(Qt::RoundJoin);

            p.setPen(pen1);
            p.drawPolyline(line);
            p.setPen(pen2);
            p.drawPolyline(line);

            // draw direction arrows
            QPoint  pt, pt1, ptt;
            bool    start = true;
            double  heading;

            //generate arrow pic
            QImage arrow_pic(21,16, QImage::Format_ARGB32);
            arrow_pic.fill( qRgba(0,0,0,0));
            QPainter t_paint(&arrow_pic);
            t_paint.setRenderHint(QPainter::Antialiasing, true);
            t_paint.setPen(QPen(Qt::white, 1));
            t_paint.setBrush((*track)->getColor());
            t_paint.drawPolygon(arrow, 4);
            t_paint.end();


            foreach(pt,line)
            {
                if(start)        // no arrow on  the first loop
                {
                    start = false;
                }
                else
                {
                    if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7) continue;
                                 // keep distance
                    if((abs(pt.x() - ptt.x()) + abs(pt.y() - ptt.y())) > 50)
                    {
                        if(0 != pt.x() - pt1.x() && (pt.y() - pt1.y()))
                        {
                            heading = ( atan2((double)(pt.y() - pt1.y()), (double)(pt.x() - pt1.x())) * 180.) / PI;

                            p.save();
                            // draw arrow between bullets
                            p.translate((pt.x() + pt1.x())/2,(pt.y() + pt1.y())/2);
                            p.rotate(heading);
                            p.drawImage(-11, -7, arrow_pic);
                            p.restore();
                                 //remember last point
                            ptt = pt;
                        }
                    }
                }
                pt1 = pt;
            }
        }

        (*track)->firstTime = false;
        ++track;
    }

    // if there is a highlighted track, draw it
    if(highlighted != tracks.end())
    {
        track = highlighted;

        QPixmap bullet = (*track)->getBullet();

        QPolygon& line = (*track)->getPolyline();

        // draw skunk line
        QPen pen1(Qt::white,11);
        pen1.setCapStyle(Qt::RoundCap);
        pen1.setJoinStyle(Qt::RoundJoin);

        QPen pen2((*track)->getColor(),7);
        pen2.setCapStyle(Qt::RoundCap);
        pen2.setJoinStyle(Qt::RoundJoin);

        p.setPen(pen1);
        p.drawPolyline(line);
        p.setPen(pen2);
        p.drawPolyline(line);

        // draw bubbles and direction arrows
        QPoint  pt, pt1, ptt;
        bool    start = true;
        double  heading;

        //generate arrow pic
        QImage arrow_pic(21,15, QImage::Format_ARGB32);
        arrow_pic.fill( qRgba(0,0,0,0));
        QPainter t_paint(&arrow_pic);
        t_paint.setRenderHint(QPainter::Antialiasing, true);
        t_paint.setPen(QPen(Qt::white, 2));
        t_paint.setBrush((*track)->getColor());
        t_paint.drawPolygon(arrow, 4);
        t_paint.end();


        foreach(pt,line)
        {
            if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7) continue;
            if(showBullets)
            {
                p.drawPixmap(pt.x() - 3 ,pt.y() - 3, bullet);
            }

            if(start)            // no arrow on  the first loop
            {
                start = false;
            }
            else
            {
                                 // keep distance
                if((abs(pt.x() - ptt.x()) + abs(pt.y() - ptt.y())) > 50)
                {
                    if(0 != pt.x() - pt1.x() && (pt.y() - pt1.y()))
                    {
                        heading = ( atan2((double)(pt.y() - pt1.y()), (double)(pt.x() - pt1.x())) * 180.) / PI;
                        p.save();
                        // draw arrow between bullets
                        p.translate((pt.x() + pt1.x())/2, (pt.y() + pt1.y())/2);
                        p.rotate(heading);
                        p.drawImage(-11, -7, arrow_pic);
                        p.restore();
                        ptt = pt;//remember last point
                    }
                }
            }
            pt1 = pt;
        }

        pt1 = QPoint();
        foreach(pt,selected)
        {
            if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7) continue;
            p.drawPixmap(pt.x() - 5 ,pt.y() - 5, bullet_red);
            pt1 = pt;
        }

        if(focus != QPoint(-1,-1))
        {
            p.setPen(QPen(Qt::white,3));
            p.drawLine(focus + QPoint(0,-20),focus + QPoint(0,20));
            p.drawLine(focus + QPoint(-20,0),focus + QPoint(20,0));
            p.setPen(Qt::red);
            p.drawLine(focus + QPoint(0,-20),focus + QPoint(0,20));
            p.drawLine(focus + QPoint(-20,0),focus + QPoint(20,0));
        }
    }

    p.setRenderHint(QPainter::Antialiasing, false);
}


void CTrackDB::select(const QRect& rect, bool select /*= true*/)
{
    CTrack * track = highlightedTrack();
    if(track == 0) return;

    undoStack->push(new CTrackUndoCommandSelect(track, rect, select));
}


void CTrackDB::copyToClipboard(bool deleteSelection /* = false */)
{
    CTrack * track = highlightedTrack();
    if(track == 0)
    {
        QMessageBox::warning(0,tr("Failed..."), tr("Failed to copy track. You must select a track or track points of a track."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    QClipboard *clipboard = QApplication::clipboard();

    CTrack *tmpTrack = new CTrack(0);

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();

    while(trkpt != trkpts.end())
    {
        if (trkpt->flags & CTrack::pt_t::eSelected)
        {
            *tmpTrack << *trkpt;
        }

        ++trkpt;
    }

    CQlb qlb(this);
    if(tmpTrack->getTrackPoints().count())
    {
        qlb << *tmpTrack;
    }
    else
    {
        qlb << *track;
    }
    QBuffer buffer;
    qlb.save(&buffer);
    QMimeData *md = new QMimeData;
    buffer.open(QIODevice::ReadOnly);
    md->setData("qlandkartegt/qlb",buffer.readAll());
    buffer.close();
    clipboard->clear();
    clipboard->setMimeData(md);

    delete tmpTrack;
}


void CTrackDB::pasteFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    qDebug() << clipboard->mimeData()->formats();
    if (clipboard->mimeData()->hasFormat("qlandkartegt/qlb"))
    {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        buffer.write(clipboard->mimeData()->data("qlandkartegt/qlb"));
        buffer.close();
        CQlb qlb(this);
        qlb.load(&buffer);
        loadQLB(qlb, true);
    }
}


CTrack *CTrackDB::take(const QString& key, bool silent)
{
    CTrack *track =  tracks.take(key);

    if (!silent)
    {
        emit sigChanged();
        emit sigModified();
    }
    return track;
}


void CTrackDB::insert(const QString& key, CTrack *track, bool silent)
{
    tracks.insert(key,track);
    if (!silent)
    {
        emit sigChanged();
        emit sigModified();
    }
}


void CTrackDB::emitSigChanged()
{
    emit sigChanged();
}


void CTrackDB::emitSigModified()
{
    emit sigModified();
}


void CTrackDB::revertTrack(const QString& key)
{
    if(!tracks.contains(key))
    {
        return;
    }
    CTrack *torg =  tracks[key];
    CTrack *tnew = new CTrack(this);

    tnew->name = torg->name + tr("_rev");

    QList<CTrack::pt_t> track = torg->track;
    while(track.size())
    {
        CTrack::pt_t pt = track.takeLast();

        pt.timestamp        = 0;
        pt.timestamp_msec   = 0;

        *tnew << pt;
    }

    addTrack(tnew, false);

}

QList<CTrackDB::keys_t> CTrackDB::keys()
{
    QList<keys_t> k;

    QString k1;
    QStringList ks = tracks.keys();

    foreach(k1, ks)
    {
        QPixmap icon(16,16);
        keys_t k2;
        CTrack * track = tracks[k1];

        k2.key      = k1;
        k2.name     = track->name;
        k2.comment  = track->comment.left(32);

        icon.fill(track->getColor());
        k2.icon     = icon;

        k << k2;
    }

    return k;
}

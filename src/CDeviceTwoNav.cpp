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


#include "CMainWindow.h"
#include "CCanvas.h"
#include "CDeviceTwoNav.h"
#include "CSettings.h"
#include "CGpx.h"
#include "CWptDB.h"
#include "CTrackDB.h"
#include "GeoMath.h"
#include "CResources.h"
#include "CTrack.h"
#include "CDlgDeviceExportPath.h"

#include <QtGui>



struct twonav_icon_t
{
    const char * twonav;
    const char * qlgt;
};

static twonav_icon_t TwoNavIcons[] =
{
     {"City (Capitol)","City (Capitol)"}
    ,{"City (Large)","City (Large)"}
    ,{"City (Medium)","City (Medium)"}
    ,{"City (Small)","City (Small)"}
    ,{"City (Small)","Small City"}
    ,{"Closed Box","Geocache"}
    ,{"Open Box","Geocache Found"}
    ,{"Red Flag","Flag, Red"}
    ,{"Blue Flag","Flag, Blue"}
    ,{"Green Flag","Flag, Green"}
    ,{"Red Booble","Pin, Red"}
    ,{"Blue Booble","Pin, Blue"}
    ,{"Green Booble","Pin, Green"}
    ,{"Red Cube","Block, Red"}
    ,{"Blue Cube","Block, Blue"}
    ,{"Green Cube","Block, Green"}
    ,{"Blue Diamond","Blue Diamond"}
    ,{"Green Diamond","Green Diamond"}
    ,{"Red Diamond","Red Diamond"}
    ,{"Traditional Cache","Traditional Cache"}
    ,{"Multi-cache","Multi-cache"}
    ,{"Unknown Cache","Unknown Cache"}
    ,{"Wherigo","Wherigo Cache"}
    ,{"Event Cache","Event Cache"}
    ,{"Earthcache","Earthcache"}
    ,{"Letterbox","Letterbox Hybrid"}
    ,{"Virtual Cache","Virtual Cache"}
    ,{"Webcam Cache","Webcam Cache"}
    ,{0,0}
};

CDeviceTwoNav::CDeviceTwoNav(QObject *parent)
: IDevice("TwoNav", parent)
, wpt(0)
{

}

CDeviceTwoNav::~CDeviceTwoNav()
{

}


QString CDeviceTwoNav::iconTwoNav2QlGt(const QString& sym)
{
    int i = 0;
    while(TwoNavIcons[i].qlgt)
    {
        if(sym == TwoNavIcons[i].twonav)
        {
            return TwoNavIcons[i].qlgt;
        }

        i++;
    }

    return sym;
}

QString CDeviceTwoNav::iconQlGt2TwoNav(const QString& sym)
{
    int i = 0;
    while(TwoNavIcons[i].qlgt)
    {
        if(sym == TwoNavIcons[i].qlgt)
        {
            return TwoNavIcons[i].twonav;
        }

        i++;
    }

    return sym;
}


bool CDeviceTwoNav::aquire(QDir& dir)
{
    SETTINGS;
    QString path = cfg.value("device/path","").toString();
    dir.setPath(path);

    pathRoot    = "";
    pathData    = "TwoNavData/Data";
    pathDay     = "";

    if(!dir.exists() || !dir.exists(pathData))
    {
        while(1)
        {
            pathRoot    = "";
            pathData    = "TwoNavData/Data";
            pathDay     = "";

            path = QFileDialog::getExistingDirectory(0, tr("Path to TwoNav device..."), dir.absolutePath());
            if(path.isEmpty())
            {
                return false;
            }
            dir.setPath(path);

            if(!dir.exists("TwoNavData/Data"))
            {
                QMessageBox::information(0,tr("Error..."), tr("I need a path with 'TwoNavData/Data' as subdirectory"),QMessageBox::Retry,QMessageBox::Retry);
                continue;
            }

            break;
        }
    }

    pathRoot = dir.absolutePath();
    pathData = dir.absoluteFilePath("TwoNavData/Data");

    cfg.setValue("device/path", pathRoot);
    return true;
}

void CDeviceTwoNav::createDayPath(const QString& what)
{

    QDir dir;
    QString subdir;
    dir.cd(pathData);

    CDlgDeviceExportPath dlg(what, dir, subdir, 0);
    dlg.exec();

    pathDay = dir.absoluteFilePath(subdir);
    if(!dir.exists(pathDay))
    {
        dir.mkpath(pathDay);
    }
}


void CDeviceTwoNav::uploadWpts(const QList<CWpt*>& wpts)
{
//    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Upload wapoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);

    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    createDayPath(tr("waypoints"));
    dir.cd(pathDay);

    // export normal waypoints.
    {
        QFile file(dir.absoluteFilePath("MyWaypoints.wpt"));
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << "B  UTF-8" << endl;
        out << "G  WGS 84" << endl;
        out << "U  1" << endl;
        foreach(CWpt * wpt, wpts)
        {
            // do not export sticky waypoints
            if(wpt->sticky) continue;
            // geocaches are exported in a separate file
            if(wpt->isGeoCache()) continue;
            // waypoints attached to another waypoint are handled with the parent waypoint
            if(!wpt->getParentWpt().isEmpty()) continue;

            writeWaypointData(out, wpt, dir);
        }
        file.close();
    }
    // export geocaches.
    {
        QFile file(dir.absoluteFilePath("GeoCaches.wpt"));
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << "B  UTF-8" << endl;
        out << "G  WGS 84" << endl;
        out << "U  1" << endl;
        foreach(CWpt * wpt, wpts)
        {
            // do not export sticky waypoints
            if(wpt->sticky) continue;
            // only accept geocaches
            if(!wpt->isGeoCache()) continue;

            // write basic data
            writeWaypointData(out, wpt, dir);

            // write geocache data
            QDomDocument doc;
            QDomElement gpxCache = doc.createElement("groundspeak:cache");
            wpt->saveTwoNavExt(gpxCache, true);
            doc.appendChild(gpxCache);

            out << "e" << endl;
            out << doc.toString();
            out << "ee" << endl;

            foreach(CWpt * wpt2, wpts)
            {
                if(wpt2->getParentWpt() == wpt->getName())
                {
                    writeWaypointData(out, wpt2, dir);
                }
            }
        }
        file.close();
    }
    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Upload waypoints finished!"));
}

void CDeviceTwoNav::downloadWpts(QList<CWpt*>& wpts)
{
//    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Download wapoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);

    QStringList files;
    QStringList subdirs;
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathData);
    subdirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    subdirs << pathData;

    foreach(const QString& subdir, subdirs)
    {
        dir.cd(subdir);

        files = dir.entryList(QStringList("*gpx"));
        foreach(const QString& filename, files)
        {
            CGpx gpx(this, CGpx::eCleanExport);
            gpx.load(dir.absoluteFilePath(filename));
            CWptDB::self().loadGPX(gpx);
        }

        files = dir.entryList(QStringList("*wpt"));
        foreach(const QString& filename, files)
        {
            readWptFile(dir,filename, wpts);
        }

        dir.cdUp();
    }

    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Download waypoints finished!"));
}

void CDeviceTwoNav::uploadTracks(const QList<CTrack*>& trks)
{
//    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Upload tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);

    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    createDayPath(tr("tracks"));
    dir.cd(pathDay);

    for(int i = 0; i < trks.count(); i++)
    {
        CTrack& trk     = *trks[i];
        QString name    = trk.getName();
        name            = name.replace(" ","_");

        QFile file(dir.absoluteFilePath(name + ".trk"));
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << "B  UTF-8" << endl;
        out << "G  WGS 84" << endl;
        out << "U  1" << endl;

        writeTrkData(out, trk, dir);

        file.close();
    }
    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Upload tracks finished!"));
}

void CDeviceTwoNav::downloadTracks(QList<CTrack*>& trks)
{
//    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Download tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
    QStringList files;
    QStringList subdirs;
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathData);
    subdirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    subdirs << pathData;

    foreach(const QString& subdir, subdirs)
    {
        dir.cd(subdir);

        files = dir.entryList(QStringList("*gpx"));
        foreach(const QString& filename, files)
        {
            CGpx gpx(this, CGpx::eCleanExport);
            gpx.load(dir.absoluteFilePath(filename));
            CTrackDB::self().loadGPX(gpx);
        }

        files = dir.entryList(QStringList("*trk"));
        foreach(const QString& filename, files)
        {
            readTrkFile(dir,filename, trks);
        }

        dir.cdUp();
    }

    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Download tracks finished!"));
}

void CDeviceTwoNav::uploadRoutes(const QList<CRoute*>& rtes)
{
    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Upload routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}

void CDeviceTwoNav::downloadRoutes(QList<CRoute*>& rtes)
{
    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}

void CDeviceTwoNav::uploadMap(const QList<IMapSelection*>& mss)
{
    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Upload maps is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}

void CDeviceTwoNav::downloadScreenshot(QImage& image)
{
    QMessageBox::information(0,tr("Error..."), tr("TwoNav: Download screenshots is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}

struct wpt_t
{
    wpt_t(): lon(0), lat(0), ele(0), prox(0){}
    QDateTime time;
    QString name;
    QString comment;
    QString symbol;
    QString key;
    QString url;

    float lon;
    float lat;
    float ele;
    float prox;

};


void CDeviceTwoNav::readWptFile(QDir& dir, const QString& filename, QList<CWpt*>& wpts)
{
    wpt_t tmpwpt;
    QString line("start");
    QFile file(dir.absoluteFilePath(filename));
    file.open(QIODevice::ReadOnly);
    QTextStream in(&file);
    in.setCodec(QTextCodec::codecForName("UTF-8"));

    while(!line.isEmpty())
    {
        line = in.readLine();

        switch(line[0].toAscii())
        {
        case 'B':
        {
            QString name        = line.mid(1).simplified();
            QTextCodec * codec  = QTextCodec::codecForName(name.toAscii());
            if(codec)
            {
                in.setCodec(codec);
            }
            break;
        }
        case 'G':
        {
            QString name  = line.mid(1).simplified();
            if(name != "WGS 84")
            {
                QMessageBox::information(0,tr("Error..."), tr("Only support lon/lat WGS 84 format."),QMessageBox::Abort,QMessageBox::Abort);
                return;
            }
            break;
        }
        case 'U':
        {
            QString name  = line.mid(1).simplified();
            if(name != "1")
            {
                QMessageBox::information(0,tr("Error..."), tr("Only support lon/lat WGS 84 format."),QMessageBox::Abort,QMessageBox::Abort);
                return;
            }
            break;
        }
        case 'W':
        {
            wpt = 0;

            tmpwpt = wpt_t();
            QStringList values = line.split(' ', QString::SkipEmptyParts);

            tmpwpt.name    = values[1];
            GPS_Math_Str_To_Deg(values[3].replace("\272","") + " " + values[4].replace("\272",""), tmpwpt.lon, tmpwpt.lat);
            tmpwpt.time    = QDateTime::fromString(values[5] + " " + values[6], "dd-MMM-yyyy hh:mm:ss");
            tmpwpt.time.setTimeSpec(Qt::UTC);
            tmpwpt.ele     = values[7].toFloat();

            if(values.size() > 7)
            {
                QStringList list = values.mid(8);
                tmpwpt.comment = list.join(" ");
            }

            break;
        }
        case 'w':
        {
            QStringList values = line.mid(1).simplified().split(',', QString::KeepEmptyParts);

            tmpwpt.symbol  = iconTwoNav2QlGt(values[0]);

            tmpwpt.url     = values[7];
            tmpwpt.prox    = values[8].toFloat();
            tmpwpt.key     = values[9];


            if(tmpwpt.prox == 0)
            {
                tmpwpt.prox = WPT_NOFLOAT;
            }
            if(tmpwpt.ele == 0)
            {
                tmpwpt.ele = WPT_NOFLOAT;
            }
            if(tmpwpt.key == "0")
            {
                tmpwpt.key.clear();
            }

            tmpwpt.name = tmpwpt.name.replace("_", " ");

            wpt = new CWpt(&CWptDB::self());

            wpt->setName(tmpwpt.name);
            wpt->setComment(tmpwpt.comment);
            wpt->setIcon(tmpwpt.symbol);
            wpt->setKey(tmpwpt.key);
            wpt->lon        = tmpwpt.lon;
            wpt->lat        = tmpwpt.lat;
            wpt->ele        = tmpwpt.ele;
            wpt->prx        = tmpwpt.prox;
            wpt->urlname    = tmpwpt.url;
            wpt->timestamp  = tmpwpt.time.toTime_t();

            wpts << wpt;

//            qDebug() << tmpwpt.name << tmpwpt.symbol << tmpwpt.time << tmpwpt.lon << tmpwpt.lat << tmpwpt.ele << tmpwpt.prox << tmpwpt.comment << tmpwpt.key;
            break;
        }
        case 'e':
        {
            QString str;

            if(wpt == 0) break;

            while(!in.atEnd())
            {
                line = in.readLine();
                if(line == "ee") break;

                str += line;
            }

            QDomDocument doc;
            QString errorMsg;
            int errorLine = 0;
            int errorColumn = 0;
            doc.setContent(str, &errorMsg, &errorLine, &errorColumn);
            wpt->loadTwoNavExt(doc.namedItem("groundspeak:cache"));
            break;
        }
        case 'a':
        {
            CWpt::image_t img;
            QString fn = line.mid(1).simplified();

#ifndef WIN32
            fn = fn.replace("\\","/");
#endif
            QFileInfo fi(dir.absoluteFilePath(fn));
            img.pixmap.load(dir.absoluteFilePath(fn));
            if(!img.pixmap.isNull())
            {
                img.filename    = fi.fileName();
                img.info        = fi.baseName();
                wpt->images << img;
            }


            break;
        }

        }
    }

}

QString CDeviceTwoNav::makeUniqueName(const QString name, QDir& dir)
{
    int cnt = 0;

    QFileInfo fi(name);
    QString tmp(name);

    while(dir.exists(tmp))
    {
        tmp = QString("%1_%2.%3").arg(fi.baseName()).arg(cnt++).arg(fi.completeSuffix());
    }

    return tmp;
}

void CDeviceTwoNav::writeTrkData(QTextStream& out, CTrack& trk, QDir& dir)
{
    QString name    = trk.getName();
    name    = name.replace(" ","_");

    QList<CTrack::wpt_t> wpts;
    trk.scaleWpt2Track(wpts);
    QList<CTrack::wpt_t>::iterator wpt = wpts.begin();
    while(wpt != wpts.end())
    {
        if(wpt->d > WPT_TO_TRACK_DIST)
        {
            wpt = wpts.erase(wpt);
            continue;
        }
        ++wpt;
    }


    QColor color = trk.getColor();

    QStringList list;
    list << "C";
    list << QString::number(color.red());
    list << QString::number(color.green());
    list << QString::number(color.blue());
    list << "5"; // ???
    list << "1";    // ???
    out << list.join(" ") << endl;

    out << "s " << name << endl;
    out << "k " << trk.getKey() << endl;

    QList<CTrack::pt_t>& trkpts = trk.getTrackPoints();
    foreach(const CTrack::pt_t& trkpt, trkpts)
    {
        list.clear();

        list << "T";
        list << "A";
        list << (trkpt.lat > 0 ? QString("%1\272N") : QString("%1\272S")).arg(trkpt.lat,0,'f');
        list << (trkpt.lon > 0 ? QString("%1\272E") : QString("%1\272W")).arg(trkpt.lon,0,'f');
        list << QDateTime::fromTime_t(trkpt.timestamp).toString("dd-MMM-yyyy");
        list << QDateTime::fromTime_t(trkpt.timestamp).toString("hh:mm:ss");
        list << "s";
        list << QString("%1").arg(trkpt.ele == WPT_NOFLOAT ? 0 : trkpt.ele,0,'f');
        list << "0.000000";
        list << "0.000000";
        list << "0.000000";
        list << "0";
        list << "-1000.000000";
        list << "-1.000000";
        list << "-1";
        list << "-1.000000";
        list << "-1";
        list << "-1";
        list << "-1";
        list << "-1.000000";

        out << list.join(" ") << endl;

        foreach(const CTrack::wpt_t& wpt, wpts)
        {
            if(wpt.trkpt == trkpt)
            {
                QString comment = wpt.wpt->getComment();
                if(comment.isEmpty())
                {
                    comment = wpt.wpt->getDescription();
                    if(comment.isEmpty())
                    {
                        comment = wpt.wpt->getName();
                    }
                }
                IItem::removeHtml(comment);
                comment = comment.replace("\n","%0A%0D");

                QString iconName    = wpt.wpt->getIconString();
                QPixmap icon        = wpt.wpt->getIcon();
                icon                = icon.scaledToWidth(15, Qt::SmoothTransformation);
                iconName            = iconQlGt2TwoNav(iconName);
                iconName            = iconName.replace(" ", "_");

                icon.save(dir.absoluteFilePath(iconName + ".png"));

                list.clear();
                list << (iconName + ".png");
                list << "1";
                list << "3";
                list << "0";
                list << comment;
                out << "a " << list.join(",") << endl;

                foreach(const CWpt::image_t& img, wpt.wpt->images)
                {
                    QString fn = img.info;
                    if(fn.isEmpty())
                    {
                        fn = QString("picture.png");
                    }
                    if(!fn.endsWith("png"))
                    {
                        fn += ".png";
                    }

                    fn = makeUniqueName(fn, dir);
                    img.pixmap.save(dir.absoluteFilePath(fn));

                    list.clear();
                    list << fn;
                    list << "1";
                    list << "8";
                    list << "0";
                    out << "a " << list.join(",") << endl;
                }
                break;
            }
        }
    }

}

void CDeviceTwoNav::writeWaypointData(QTextStream& out, CWpt * wpt, QDir& dir)
{
    QString name    = wpt->getName();
    name    = name.replace(" ","_");

    QString comment = wpt->getComment();
    if(comment.isEmpty())
    {
        comment = wpt->getDescription();
    }
    IItem::removeHtml(comment);
    comment = comment.replace("\n","%0A%0D");

    QStringList list;
    list << "W";
    list << name.replace(" ", "_");
    list << "A";
    list << (wpt->lat > 0 ? QString("%1\272N") : QString("%1\272S")).arg(wpt->lat,0,'f');
    list << (wpt->lon > 0 ? QString("%1\272E") : QString("%1\272W")).arg(wpt->lon,0,'f');
    list << QDateTime::fromTime_t(wpt->timestamp).toString("dd-MMM-yyyy");
    list << QDateTime::fromTime_t(wpt->timestamp).toString("hh:mm:ss");
    list << QString("%1").arg(wpt->ele == WPT_NOFLOAT ? 0 : wpt->ele,0,'f');

    out << list.join(" ") << " ";
    out << comment << endl;

    list.clear();
    list << iconQlGt2TwoNav(wpt->getIconString());
    list << "0"; //test position
    list << "-1.0";
    list << "0";
    list << QString("%1").arg(CResources::self().wptTextColor().value());
    list << "1";
    list << "37"; // 1 Name 2 Beschreibung 4 Symbol 8 Höhe 16 URL 32 Radius
    list << "";//wpt->link;
    list << QString("%1").arg(wpt->prx == WPT_NOFLOAT ? 0 : wpt->prx,0,'f');
    list << wpt->getKey();

    out << "w ";
    out << list.join(",");
    out << endl;

    foreach(const CWpt::image_t& img, wpt->images)
    {
        QString fn = img.info;
        if(fn.isEmpty())
        {
            fn = QString("picture.png");
        }
        if(!fn.endsWith("png"))
        {
            fn += ".png";
        }

        fn = makeUniqueName(fn, dir);
        img.pixmap.save(dir.absoluteFilePath(fn));
        out << "a " << ".\\" << fn << endl;
    }
}


void CDeviceTwoNav::readTrkFile(QDir &dir, const QString &filename, QList<CTrack *> &trks)
{
    QString line("start");
    QFile file(dir.absoluteFilePath(filename));
    file.open(QIODevice::ReadOnly);
    QTextStream in(&file);
    in.setCodec(QTextCodec::codecForName("UTF-8"));

    CTrack * track = new CTrack(&CTrackDB::self());

    while(!line.isEmpty())
    {
        line = in.readLine();
        switch(line[0].toAscii())
        {
        case 'B':
        {
            QString name        = line.mid(1).simplified();
            QTextCodec * codec  = QTextCodec::codecForName(name.toAscii());
            if(codec)
            {
                in.setCodec(codec);
            }
            break;
        }
        case 'G':
        {
            QString name  = line.mid(1).simplified();
            if(name != "WGS 84")
            {
                QMessageBox::information(0,tr("Error..."), tr("Only support lon/lat WGS 84 format."),QMessageBox::Abort,QMessageBox::Abort);
                return;
            }
            break;
        }
        case 'U':
        {
            QString name  = line.mid(1).simplified();
            if(name != "1")
            {
                QMessageBox::information(0,tr("Error..."), tr("Only support lon/lat WGS 84 format."),QMessageBox::Abort,QMessageBox::Abort);
                return;
            }
            break;
        }
        case 'C':
        {
            QStringList values = line.split(' ', QString::SkipEmptyParts);
            QColor c(values[1].toInt(),values[2].toInt(),values[3].toInt());
            track->setColor(c);
            break;
        }
        case 'T':
        {
            CTrack::pt_t pt;
            QStringList values = line.split(' ', QString::SkipEmptyParts);

            GPS_Math_Str_To_Deg(values[2].replace("\272","") + " " + values[3].replace("\272",""), pt.lon, pt.lat);
            QDateTime time = QDateTime::fromString(values[4] + " " + values[5], "dd-MMM-yy hh:mm:ss.zzz");
            time.setTimeSpec(Qt::UTC);
            time = time.addYears(100);
            pt.timestamp        = time.toTime_t();
            pt.timestamp_msec   = time.time().msec();
            pt.ele = values[7].toFloat();


            pt._lon = pt.lon;
            pt._lat = pt.lat;
            pt._ele = pt.ele;

            *track << pt;
            break;
        }
        case 's':
        {
            QString name  = line.mid(1).simplified();
            name = name.replace("_", " ");
            track->setName(name);
            break;
        }
        case 'k':
        {
            QString name  = line.mid(1).simplified();
            track->setKey(name);
            break;
        }
        }
    }

    track->rebuild(true);
    trks << track;
}

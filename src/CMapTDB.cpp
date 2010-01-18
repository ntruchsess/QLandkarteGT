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
#include "CMapTDB.h"
#include "CMapDB.h"
#include "CMapDEM.h"
#include "Garmin.h"
#include "CGarminTile.h"
#include "CGarminIndex.h"
#include "GeoMath.h"
#include "CResources.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "IUnit.h"
#include "Platform.h"
#include "CMapSelectionGarmin.h"
#include "CDlgMapTDBConfig.h"
#include "CGarminTyp.h"
#include "CGarminTypNT.h"

#include <QtGui>
#include <algorithm>
#include <QSqlDatabase>

#define MAX_IDX_ZOOM 35
#define MIN_IDX_ZOOM 0
#undef DEBUG_SHOW_SECTION_BORDERS
#define DEBUG_SHOW_MAPLEVELS

#define TEXTWIDTH   300

CMapTDB::scale_t CMapTDB::scales[] =
{
    {                            //0
        QString("7000 km"), 70000.0, 8
    }
    ,                            //1
    {
        QString("5000 km"), 50000.0, 8
    }
    ,                            //2
    {
        QString("3000 km"), 30000.0, 9
    }
    ,                            //3
    {
        QString("2000 km"), 20000.0, 9
    }
    ,                            //4
    {
        QString("1500 km"), 15000.0, 10
    }
    ,                            //5
    {
        QString("1000 km"), 10000.0, 10
    }
    ,                            //6
    {
        QString("700 km"), 7000.0, 11
    }
    ,                            //7
    {
        QString("500 km"), 5000.0, 11
    }
    ,                            //8
    {
        QString("300 km"), 3000.0, 13
    }
    ,                            //9
    {
        QString("200 km"), 2000.0, 13
    }
    ,                            //10
    {
        QString("150 km"), 1500.0, 13
    }
    ,                            //11
    {
        QString("100 km"), 1000.0, 14
    }
    ,                            //12
    {
        QString("70 km"), 700.0, 15
    }
    ,                            //13
    {
        QString("50 km"), 500.0, 16
    }
    ,                            //14
    {
        QString("30 km"), 300.0, 16
    }
    ,                            //15
    {
        QString("20 km"), 200.0, 17
    }
    ,                            //16
    {
        QString("15 km"), 150.0, 17
    }
    ,                            //17
    {
        QString("10 km"), 100.0, 18
    }
    ,                            //18
    {
        QString("7 km"), 70.0, 18
    }
    ,                            //19
    {
        QString("5 km"), 50.0, 19
    }
    ,                            //20
    {
        QString("3 km"), 30.0, 19
    }
    ,                            //21
    {
        QString("2 km"), 20.0, 20
    }
    ,                            //22
    {
        QString("1.5 km"), 15.0, 22
    }
    ,                            //23
    {
        QString("1 km"), 10.0, 24
    }
    ,                            //24
    {
        QString("700 m"), 7.0, 24
    }
    ,                            //25
    {
        QString("500 m"), 5.0, 24
    }
    ,                            //26
    {
        QString("300 m"), 3.0, 24
    }
    ,                            //27
    {
        QString("200 m"), 2.0, 24
    }
    ,                            //28
    {
        QString("150 m"), 1.5, 24
    }
    ,                            //29
    {
        QString("100 m"), 1.0, 24
    }
    ,                            //30
    {
        QString("70 m"), 0.7, 24
    }
    ,                            //31
    {
        QString("50 m"), 0.5, 24
    }
    ,                            //32
    {
        QString("30 m"), 0.3, 24
    }
    ,                            //33
    {
        QString("20 m"), 0.2, 24
    }
    ,                            //34
    {
        QString("15 m"), 0.15, 24
    }
    ,                            //35
    {
        QString("10 m"), 0.10, 24
    }
};

CMapTDB::CMapTDB(const QString& key, const QString& filename, CCanvas * parent)
: IMap(eGarmin, key, parent)
, filename(filename)
, north(-90.0 * DEG_TO_RAD)
, east(-180.0 * DEG_TO_RAD)
, south(90.0 * DEG_TO_RAD)
, west(180.0 * DEG_TO_RAD)
, baseimg(0)
, isTransparent(false)
, zoomFactor(0)
, fm(CResources::self().getMapFont())
, detailsFineTune(0)
, mouseOverDecDetail(false)
, mouseOverIncDetail(false)
, lon_factor(+1.0)
, lat_factor(-1.0)
, useTyp(true)
, textAboveLine(true)
, poiLabels(true)
, checkPoiLabels(0)
, nightView(false)
, checkNightView(0)
{
    readTDB(filename);
    //     QString str = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs +towgs84=0,0,0";
    QString str = QString("+proj=merc +lat_ts=%1 +ellps=WGS84").arg(int((south + (north - south) / 2) * RAD_TO_DEG));
    pjsrc       = pj_init_plus(str.toLatin1());

    char * ptr;
    ptr = pj_get_def(pjsrc,0);
    qDebug() << "pjsrc:\t" << ptr;
//     free(ptr);
    ptr = pj_get_def(pjtar,0);
    qDebug() << "pjtar:\t" << ptr;
//     free(ptr);

    processPrimaryMapData();

    QSettings cfg;
    cfg.beginGroup("garmin/maps");
    cfg.beginGroup(name);
    QString pos     = cfg.value("topleft","").toString();
    zoomidx         = cfg.value("zoomidx",11).toInt();
    detailsFineTune = cfg.value("details",0).toInt();
    textAboveLine   = cfg.value("textAboveLine",textAboveLine).toBool();
    useTyp          = cfg.value("useTyp",useTyp).toBool();
    poiLabels       = cfg.value("poiLabels",poiLabels).toBool();
    nightView       = cfg.value("nightView",nightView).toBool();
    cfg.endGroup();
    cfg.endGroup();

    setup();

    if(pos.isEmpty()) {
        topLeft.u = west;
        topLeft.v = north;
    }
    else {
        float u = 0;
        float v = 0;
        GPS_Math_Str_To_Deg(pos, u, v);
        topLeft.u = u * DEG_TO_RAD;
        topLeft.v = v * DEG_TO_RAD;
    }
    zoom(zoomidx);
    resize(parent->size());

    info = new QTextDocument(this);
    info->setTextWidth(TEXTWIDTH);
    info->setHtml(infotext);

    parent->installEventFilter(this);

    index = new CGarminIndex(this);
    index->setDBName(name);

    checkPoiLabels = new QCheckBox(theMainWindow->getCanvas());
    checkPoiLabels->setText(tr("POI labels"));
    theMainWindow->statusBar()->insertPermanentWidget(0,checkPoiLabels);
    checkPoiLabels->setChecked(poiLabels);
    connect(checkPoiLabels, SIGNAL(toggled(bool)), this, SLOT(slotPoiLabels(bool)));

    checkNightView = new QCheckBox(theMainWindow->getCanvas());
    checkNightView->setText(tr("Night"));
    theMainWindow->statusBar()->insertPermanentWidget(0,checkNightView);
    checkNightView->setChecked(nightView);
    connect(checkNightView, SIGNAL(toggled(bool)), this, SLOT(slotNightView(bool)));


    qDebug() << "CMapTDB::CMapTDB()";
}


CMapTDB::CMapTDB(const QString& key, const QString& filename)
: IMap(eGarmin, key, 0)
, filename(filename)
, north(-90.0)
, east(-180.0)
, south(90.0)
, west(180.0)
, baseimg(0)
, isTransparent(false)
, zoomFactor(0)
, fm(CResources::self().getMapFont())
, detailsFineTune(0)
, mouseOverDecDetail(false)
, mouseOverIncDetail(false)
, fid(0x0001)
, pid(0x0320)
, lon_factor(+1.0)
, lat_factor(-1.0)
, useTyp(true)
, textAboveLine(true)
, poiLabels(true)
, checkPoiLabels(0)
, nightView(false)
, checkNightView(0)
{
    char * ptr = CMapDB::self().getMap().getProjection();

    if(QString(ptr).contains("longlat")) {
        lon_factor =   PI / pow(2.0, 24);
        lat_factor = - PI / pow(2.0, 24);
        qDebug() << "set correction factor to" << lon_factor << lat_factor;
    }

    pjsrc = pj_init_plus(ptr);

    qDebug() << "TDB:" << ptr;
//     if(ptr) free(ptr);

    readTDB(filename);
    processPrimaryMapData();

    QSettings cfg;
    cfg.beginGroup("garmin/maps");
    cfg.beginGroup(name);
    detailsFineTune = cfg.value("details",0).toInt();
    textAboveLine   = cfg.value("textAboveLine",textAboveLine).toBool();
    useTyp          = cfg.value("useTyp",useTyp).toBool();
    poiLabels       = cfg.value("poiLabels",poiLabels).toBool();
    cfg.endGroup();
    cfg.endGroup();

    setup();
    info          = 0;
    isTransparent = true;

    index = new CGarminIndex(this);
    index->setDBName(name);
    qDebug() << "CMapTDB::CMapTDB()";
}


CMapTDB::~CMapTDB()
{
    QString pos;
    QSettings cfg;
    cfg.beginGroup("garmin/maps");
    cfg.beginGroup(name);
    GPS_Math_Deg_To_Str(topLeft.u * RAD_TO_DEG, topLeft.v * RAD_TO_DEG, pos);
    pos = pos.replace("\260","");
    cfg.setValue("topleft",pos);
    cfg.setValue("zoomidx",zoomidx);
    cfg.setValue("details", detailsFineTune);
    cfg.setValue("textAboveLine",textAboveLine);
    cfg.setValue("useTyp",useTyp);
    cfg.setValue("poiLabels",poiLabels);
    cfg.setValue("nightView",nightView);

    cfg.endGroup();
    cfg.endGroup();

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);

    if(pjsrc) pj_free(pjsrc);

    if(checkPoiLabels) {
        delete checkPoiLabels;
    }

    if(checkNightView) {
        delete checkNightView;
    }

    qDebug() << "CMapTDB::~CMapTDB()";
}

void CMapTDB::slotPoiLabels(bool checked)
{
    poiLabels   = checked;
    needsRedraw = true;
    emit sigChanged();
}

void CMapTDB::slotNightView(bool checked)
{
    nightView   = checked;
    needsRedraw = true;
    emit sigChanged();
}

void CMapTDB::setup()
{
    polyline_typestr.clear();
    polyline_typestr << /*0x00,*/   tr("");
    polyline_typestr << /*0x01,*/   tr("Major highway");
    polyline_typestr << /*0x02,*/   tr("Principal highway");
    polyline_typestr << /*0x03,*/   tr("Other highway");
    polyline_typestr << /*0x04,*/   tr("Arterial road");
    polyline_typestr << /*0x05,*/   tr("Collector road");
    polyline_typestr << /*0x06,*/   tr("Residential street");
    polyline_typestr << /*0x07,*/   tr("Alley/Private road");
    polyline_typestr << /*0x08,*/   tr("Highway ramp, low speed");
    polyline_typestr << /*0x09,*/   tr("Highway ramp, high speed");
    polyline_typestr << /*0x0a,*/   tr("Unpaved road");
    polyline_typestr << /*0x0b,*/   tr("Major highway connector");
    polyline_typestr << /*0x0c,*/   tr("Roundabout");
    polyline_typestr << /*0x0d,*/   tr("");
    polyline_typestr << /*0x0e,*/   tr("");
    polyline_typestr << /*0x0f,*/   tr("");
    polyline_typestr << /*0x10,*/   tr("");
    polyline_typestr << /*0x11,*/   tr("");
    polyline_typestr << /*0x12,*/   tr("");
    polyline_typestr << /*0x13,*/   tr("");
    polyline_typestr << /*0x14,*/   tr("Railroad");
    polyline_typestr << /*0x15,*/   tr("Shoreline");
    polyline_typestr << /*0x16,*/   tr("Trail");
    polyline_typestr << /*0x17,*/   tr("");
    polyline_typestr << /*0x18,*/   tr("Stream");
    polyline_typestr << /*0x19,*/   tr("Time zone");
    polyline_typestr << /*0x1a,*/   tr("Ferry");
    polyline_typestr << /*0x1b,*/   tr("Ferry");
    polyline_typestr << /*0x1c,*/   tr("State/province border");
    polyline_typestr << /*0x1d,*/   tr("County/parish border");
    polyline_typestr << /*0x1e,*/   tr("International border");
    polyline_typestr << /*0x1f,*/   tr("River");
    polyline_typestr << /*0x20,*/   tr("Minor land contour");
    polyline_typestr << /*0x21,*/   tr("Intermediate land contour");
    polyline_typestr << /*0x22,*/   tr("Major land contour");
    polyline_typestr << /*0x23,*/   tr("Minor deph contour");
    polyline_typestr << /*0x24,*/   tr("Intermediate depth contour");
    polyline_typestr << /*0x25,*/   tr("Major depth contour");
    polyline_typestr << /*0x26,*/   tr("Intermittent stream");
    polyline_typestr << /*0x27,*/   tr("Airport runway");
    polyline_typestr << /*0x28,*/   tr("Pipeline");
    polyline_typestr << /*0x29,*/   tr("Powerline");
    polyline_typestr << /*0x2a,*/   tr("Marine boundary");
    polyline_typestr << /*0x2b,*/   tr("Hazard boundary");
    polyline_typestr << /*0x2c,*/   tr("");
    polyline_typestr << /*0x2d,*/   tr("");
    polyline_typestr << /*0x2e,*/   tr("");
    polyline_typestr << /*0x2f,*/   tr("");
    polyline_typestr << /*0x30,*/   tr("");
    polyline_typestr << /*0x31,*/   tr("");
    polyline_typestr << /*0x32,*/   tr("");
    polyline_typestr << /*0x33,*/   tr("");
    polyline_typestr << /*0x34,*/   tr("");
    polyline_typestr << /*0x35,*/   tr("");
    polyline_typestr << /*0x36,*/   tr("");
    polyline_typestr << /*0x37,*/   tr("");
    polyline_typestr << /*0x38,*/   tr("");
    polyline_typestr << /*0x39,*/   tr("");
    polyline_typestr << /*0x3a,*/   tr("");
    polyline_typestr << /*0x3b,*/   tr("");
    polyline_typestr << /*0x3c,*/   tr("");
    polyline_typestr << /*0x3d,*/   tr("");
    polyline_typestr << /*0x3e,*/   tr("");
    polyline_typestr << /*0x3f,*/   tr("");

    polylineProperties.clear();
    polylineProperties[0x01] = IGarminTyp::polyline_property(0x01, "#c46442",   4, Qt::SolidLine );
//     polylineProperties[0x01].pixmap = majorHighway(Qt::blue);
    polylineProperties[0x02] = IGarminTyp::polyline_property(0x02, "#dc7c5a",   3, Qt::SolidLine );
//     polylineProperties[0x02].pixmap = principalHighway("#cc9900");
    polylineProperties[0x03] = IGarminTyp::polyline_property(0x03, "#D86C00",   3, Qt::SolidLine );
//     polylineProperties[0x03].pixmap = otherHighway(Qt::yellow);
    polylineProperties[0x04] = IGarminTyp::polyline_property(0x04, "#ffff99",   3, Qt::SolidLine );
//     polylineProperties[0x04].pixmap = arterialRoad("#ffff00");
    polylineProperties[0x05] = IGarminTyp::polyline_property(0x05, "#dc7c5a",   2, Qt::SolidLine );
    polylineProperties[0x06] = IGarminTyp::polyline_property(0x06, "#000000",   2, Qt::SolidLine );
//     polylineProperties[0x06].pixmap = residentialStreet(Qt::white);
    polylineProperties[0x07] = IGarminTyp::polyline_property(0x07, "#c46442",   1, Qt::SolidLine );
    polylineProperties[0x08] = IGarminTyp::polyline_property(0x08, "#e88866",   2, Qt::SolidLine );
    polylineProperties[0x09] = IGarminTyp::polyline_property(0x09, "#e88866",   2, Qt::SolidLine );
    polylineProperties[0x0A] = IGarminTyp::polyline_property(0x0A, "#808080",   2, Qt::SolidLine );
    polylineProperties[0x0B] = IGarminTyp::polyline_property(0x0B, "#c46442",   2, Qt::SolidLine );
    polylineProperties[0x0C] = IGarminTyp::polyline_property(0x0C, "#FFFFFF",   2, Qt::SolidLine );
    polylineProperties[0x14] = IGarminTyp::polyline_property(0x14, "#FFFFFF",   2, Qt::DotLine   );
//     polylineProperties[0x14].pixmap = QImage(":/typ/railroad.xpm");
    polylineProperties[0x15] = IGarminTyp::polyline_property(0x15, "#000080",   2, Qt::SolidLine );
    polylineProperties[0x16] = IGarminTyp::polyline_property(0x16, "#E0E0E0",   1, Qt::SolidLine );
    polylineProperties[0x18] = IGarminTyp::polyline_property(0x18, "#0000ff",   2, Qt::SolidLine );
    polylineProperties[0x19] = IGarminTyp::polyline_property(0x19, "#00ff00",   2, Qt::SolidLine );
    polylineProperties[0x1A] = IGarminTyp::polyline_property(0x1A, "#000000",   2, Qt::SolidLine );
    polylineProperties[0x1B] = IGarminTyp::polyline_property(0x1B, "#000000",   2, Qt::SolidLine );
    polylineProperties[0x1C] = IGarminTyp::polyline_property(0x1C, "#00c864",   2, Qt::DotLine   );
    polylineProperties[0x1D] = IGarminTyp::polyline_property(0x1D, "#00c864",   2, Qt::DotLine   );
    polylineProperties[0x1E] = IGarminTyp::polyline_property(0x1E, "#00c864",   2, Qt::DotLine   );
    polylineProperties[0x1F] = IGarminTyp::polyline_property(0x1F, "#0000ff",   2, Qt::SolidLine );
    polylineProperties[0x20] = IGarminTyp::polyline_property(0x20, "#b67824",   1, Qt::SolidLine );
    polylineProperties[0x21] = IGarminTyp::polyline_property(0x21, "#b67824",   2, Qt::SolidLine );
    polylineProperties[0x22] = IGarminTyp::polyline_property(0x22, "#b67824",   3, Qt::SolidLine );
    polylineProperties[0x23] = IGarminTyp::polyline_property(0x23, "#b67824",   1, Qt::SolidLine );
    polylineProperties[0x24] = IGarminTyp::polyline_property(0x24, "#b67824",   2, Qt::SolidLine );
    polylineProperties[0x25] = IGarminTyp::polyline_property(0x25, "#b67824",   3, Qt::SolidLine );
    polylineProperties[0x26] = IGarminTyp::polyline_property(0x26, "#0000ff",   2, Qt::DotLine   );
    polylineProperties[0x27] = IGarminTyp::polyline_property(0x27, "#c46442",   4, Qt::SolidLine );
    polylineProperties[0x28] = IGarminTyp::polyline_property(0x28, "#aa0000",   2, Qt::SolidLine );
    polylineProperties[0x29] = IGarminTyp::polyline_property(0x29, "#ff0000",   2, Qt::SolidLine );
    polylineProperties[0x2A] = IGarminTyp::polyline_property(0x2A, "#000000",   2, Qt::SolidLine );
    polylineProperties[0x2B] = IGarminTyp::polyline_property(0x2B, "#000000",   2, Qt::SolidLine );

    polygonProperties.clear();
    polygonProperties[0x01] = IGarminTyp::polygon_property(0x01, Qt::NoPen,     "#d2c0c0", Qt::SolidPattern);
    polygonProperties[0x02] = IGarminTyp::polygon_property(0x02, Qt::NoPen,     "#fbeab7", Qt::SolidPattern);
    polygonProperties[0x03] = IGarminTyp::polygon_property(0x03, Qt::NoPen,     "#a4b094", Qt::SolidPattern);
    polygonProperties[0x04] = IGarminTyp::polygon_property(0x04, Qt::NoPen,     "#808080", Qt::SolidPattern);
    polygonProperties[0x05] = IGarminTyp::polygon_property(0x05, Qt::NoPen,     "#f0f0f0", Qt::SolidPattern);
    polygonProperties[0x06] = IGarminTyp::polygon_property(0x06, Qt::NoPen,     "#cacaca", Qt::SolidPattern);
    polygonProperties[0x07] = IGarminTyp::polygon_property(0x07, Qt::NoPen,     "#feebcf", Qt::SolidPattern);
    polygonProperties[0x08] = IGarminTyp::polygon_property(0x08, Qt::NoPen,     "#fde8d5", Qt::SolidPattern);
    polygonProperties[0x09] = IGarminTyp::polygon_property(0x09, Qt::NoPen,     "#fee8b8", Qt::SolidPattern);
    polygonProperties[0x0a] = IGarminTyp::polygon_property(0x0a, Qt::NoPen,     "#fdeac6", Qt::SolidPattern);
    polygonProperties[0x0b] = IGarminTyp::polygon_property(0x0b, Qt::NoPen,     "#fddfbd", Qt::SolidPattern);
    polygonProperties[0x0c] = IGarminTyp::polygon_property(0x0c, Qt::NoPen,     "#ebeada", Qt::SolidPattern);
    polygonProperties[0x0d] = IGarminTyp::polygon_property(0x0d, Qt::NoPen,     "#f8e3be", Qt::SolidPattern);
    polygonProperties[0x0e] = IGarminTyp::polygon_property(0x0e, Qt::NoPen,     "#e0e0e0", Qt::SolidPattern);
    polygonProperties[0x13] = IGarminTyp::polygon_property(0x13, Qt::NoPen,     "#cc9900", Qt::SolidPattern);
    polygonProperties[0x14] = IGarminTyp::polygon_property(0x14, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x15] = IGarminTyp::polygon_property(0x15, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x16] = IGarminTyp::polygon_property(0x16, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x17] = IGarminTyp::polygon_property(0x17, Qt::NoPen,     "#90be00", Qt::SolidPattern);
    polygonProperties[0x18] = IGarminTyp::polygon_property(0x18, Qt::NoPen,     "#00ff00", Qt::SolidPattern);
    polygonProperties[0x19] = IGarminTyp::polygon_property(0x19, Qt::NoPen,     "#f8e3be", Qt::SolidPattern);
    polygonProperties[0x1a] = IGarminTyp::polygon_property(0x1a, Qt::NoPen,     "#d3f5a5", Qt::SolidPattern);
    polygonProperties[0x1e] = IGarminTyp::polygon_property(0x1e, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x1f] = IGarminTyp::polygon_property(0x1f, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x20] = IGarminTyp::polygon_property(0x20, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x28] = IGarminTyp::polygon_property(0x28, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x29] = IGarminTyp::polygon_property(0x29, Qt::NoPen,     "#0000ff", Qt::SolidPattern);
    polygonProperties[0x32] = IGarminTyp::polygon_property(0x32, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x3b] = IGarminTyp::polygon_property(0x3b, Qt::NoPen,     "#0000ff", Qt::SolidPattern);
    polygonProperties[0x3c] = IGarminTyp::polygon_property(0x3c, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x3d] = IGarminTyp::polygon_property(0x3d, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x3e] = IGarminTyp::polygon_property(0x3e, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x3f] = IGarminTyp::polygon_property(0x3f, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x40] = IGarminTyp::polygon_property(0x40, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x41] = IGarminTyp::polygon_property(0x41, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x42] = IGarminTyp::polygon_property(0x42, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x43] = IGarminTyp::polygon_property(0x43, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x44] = IGarminTyp::polygon_property(0x44, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x45] = IGarminTyp::polygon_property(0x45, Qt::NoPen,     "#0000ff", Qt::SolidPattern);
    polygonProperties[0x46] = IGarminTyp::polygon_property(0x46, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x47] = IGarminTyp::polygon_property(0x47, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x48] = IGarminTyp::polygon_property(0x48, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x49] = IGarminTyp::polygon_property(0x49, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x4a] = IGarminTyp::polygon_property(0x4a, "#000000",     Qt::NoBrush, Qt::NoBrush);
    polygonProperties[0x4b] = IGarminTyp::polygon_property(0x4b, "#000000",     Qt::NoBrush, Qt::NoBrush);
    polygonProperties[0x4c] = IGarminTyp::polygon_property(0x4c, Qt::NoPen,     "#f0e68c", Qt::SolidPattern);
    polygonProperties[0x4d] = IGarminTyp::polygon_property(0x4d, Qt::NoPen,     "#00ffff", Qt::SolidPattern);
    polygonProperties[0x4e] = IGarminTyp::polygon_property(0x4e, Qt::NoPen,     "#d3f5a5", Qt::SolidPattern);
    polygonProperties[0x4f] = IGarminTyp::polygon_property(0x4f, Qt::NoPen,     "#d3f5a5", Qt::SolidPattern);
    polygonProperties[0x50] = IGarminTyp::polygon_property(0x50, Qt::NoPen,     "#b7e999", Qt::SolidPattern);
    polygonProperties[0x51] = IGarminTyp::polygon_property(0x51, Qt::NoPen,     "#ffffff", Qt::SolidPattern);
    polygonProperties[0x52] = IGarminTyp::polygon_property(0x52, Qt::NoPen,     "#4aca4a", Qt::SolidPattern);
    polygonProperties[0x53] = IGarminTyp::polygon_property(0x53, Qt::NoPen,     "#bcedfa", Qt::SolidPattern);
    polygonProperties[0x54] = IGarminTyp::polygon_property(0x54, Qt::NoPen,     "#fde8d5", Qt::SolidPattern);
    polygonProperties[0x59] = IGarminTyp::polygon_property(0x59, Qt::NoPen,     "#0080ff", Qt::SolidPattern);
    polygonProperties[0x69] = IGarminTyp::polygon_property(0x69, Qt::NoPen,     "#0080ff", Qt::SolidPattern);

    polygonDrawOrder.clear();
    for(int i = 0; i < 0x80; i++) {
        polygonDrawOrder << (0x7F - i);
    }

    pointProperties.clear();

    if(useTyp) {
        readTYP();
    }
}


void CMapTDB::registerDEM(CMapDEM& dem)
{
    if(pjsrc == 0) {
        dem.deleteLater();
        throw tr("No basemap projection. That shouldn't happen.");
    }

    char * ptr = dem.getProjection();
    qDebug() << "Reproject map to:" << ptr;

    if(QString(ptr).contains("longlat")) {
        lon_factor =   PI / pow(2.0, 23);
        lat_factor = - PI / pow(2.0, 23);
        qDebug() << "set correction factor to" << lon_factor << lat_factor;
    }

    if(strlen(ptr) == 0) {
        return;
    }
    pj_free(pjsrc);
    pjsrc = pj_init_plus(ptr);
//     if(ptr) free(ptr);
}


void CMapTDB::resize(const QSize& s)
{
    int yshift = 0;
    if (QApplication::desktop()->height() < 650) yshift = 60 ;
    IMap::resize(s);
    topLeftInfo     = QPoint(size.width() - TEXTWIDTH - 10 , 10);

    rectDecDetail   = QRect(20, size.height() - 55 - yshift, 32, 32);
    rectDetail      = QRect(52, size.height() - 55 - yshift, 128, 32);
    rectIncDetail   = QRect(180, size.height() - 55 - yshift, 32, 32);

    setFastDraw();
}


bool CMapTDB::eventFilter(QObject * watched, QEvent * event)
{

    if(parent() == watched && event->type() == QEvent::MouseMove && !doFastDraw) {
        QMouseEvent * e = (QMouseEvent*)event;

        pointFocus = e->pos();

        if(rectDecDetail.contains(pointFocus) && !mouseOverDecDetail) {
            mouseOverDecDetail = true;
        }
        else if(!rectDecDetail.contains(pointFocus) && mouseOverDecDetail) {
            mouseOverDecDetail = false;
        }

        if(rectIncDetail.contains(pointFocus) && !mouseOverIncDetail) {
            mouseOverIncDetail = true;
        }
        else if(!rectIncDetail.contains(pointFocus) && mouseOverIncDetail) {
            mouseOverIncDetail = false;
        }

        QMultiMap<QString, QString> dict;
        getInfoPoints(pointFocus, dict);
        getInfoPois(pointFocus, dict);
        getInfoPolygons(pointFocus, dict);
        getInfoPolylines(pointFocus, dict);

        QString key;
        QStringList keys = dict.uniqueKeys();
        qSort(keys);

        infotext.clear();
        foreach(key,keys) {
            infotext += "<b>" + key + ":</b><br/>";
            const QStringList& values = dict.values(key).toSet().toList();
            infotext += values.join("<br/>");
            infotext += "<br/>";
        }

        if(!doFastDraw) emit sigChanged();

    }
    else if(parent() == watched && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent * e = (QMouseEvent*)event;
        if(rectDecDetail.contains(e->pos()) && e->button() == Qt::LeftButton) {
            detailsFineTune -= 1;
            if(detailsFineTune < -5) detailsFineTune = -5;
            needsRedraw = true;
            emit sigChanged();
            qDebug() << "detailsFineTune" << detailsFineTune;
        }
        else if(rectIncDetail.contains(e->pos()) && e->button() == Qt::LeftButton) {
            detailsFineTune += 1;
            if(detailsFineTune >  5) detailsFineTune =  5;
            needsRedraw = true;
            emit sigChanged();
            qDebug() << "detailsFineTune" << detailsFineTune;
        }
    }

    return IMap::eventFilter(watched, event);
}


void CMapTDB::readTDB(const QString& filename)
{
    QByteArray  data;
    QFile       file(filename);
    QFileInfo   finfo(filename);

    file.open(QIODevice::ReadOnly);
    data = file.readAll();
    file.close();

    quint8 * const pRawData = (quint8*)data.data();
    tdb_hdr_t * pRecord     = (tdb_hdr_t*)pRawData;

    quint32 basemapId   = 0;
    bool    tainted     = false;

    while((quint8*)pRecord < pRawData + data.size()) {
        pRecord->size = gar_load(uint16_t,pRecord->size);
        switch(pRecord->type) {
            case 0x50:           // product name
            {
                tdb_product_t * p = (tdb_product_t*)pRecord;
                name = (char*)p->name;
            }
            break;

            case 0x42:           // basemap
            {
                tdb_map_t * p   = (tdb_map_t*)pRecord;

                basemapId       = gar_load(uint32_t,p->id);
                double n        = GARMIN_RAD((gar_load(int32_t,p->north) >> 8) & 0x00FFFFFF);
                double e        = GARMIN_RAD((gar_load(int32_t,p->east) >> 8)  & 0x00FFFFFF);
                double s        = GARMIN_RAD((gar_load(int32_t,p->south) >> 8) & 0x00FFFFFF);
                double w        = GARMIN_RAD((gar_load(int32_t,p->west) >> 8)  & 0x00FFFFFF);

                if(north < n) north = n;
                if(east  < e) east  = e;
                if(south > s) south = s;
                if(west  > w) west  = w;

                QSettings cfg;
                cfg.beginGroup("garmin/maps");
                cfg.beginGroup(name);
                basemap = cfg.value("basemap","").toString();
                mapkey  = cfg.value("key","").toString();
                cfg.endGroup();
                cfg.endGroup();

                QFileInfo basemapFileInfo(basemap);
                if(!basemapFileInfo.isFile()) {

                    qApp->changeOverrideCursor(Qt::ArrowCursor);
                    QString filename = QFileDialog::getOpenFileName( 0, tr("Select Base Map for ") + name
                        ,finfo.dir().path()
                        ,"Map File (*.img)"
                        ,0
                        , QFileDialog::DontUseNativeDialog
                        );
                    qApp->restoreOverrideCursor();

                    if(filename.isEmpty()) {
                        deleteLater();
                        return;
                    }

                    cfg.beginGroup("garmin/maps");
                    cfg.beginGroup(name);
                    cfg.setValue("basemap",filename);
                    cfg.endGroup();
                    cfg.endGroup();

                    basemap = filename;
                }

                area = QRectF(QPointF(west, north), QPointF(east, south));

            }
            break;

            case 0x4C:           // map tiles
            {
                tdb_map_t * p = (tdb_map_t*)pRecord;
                p->id = gar_load(uint32_t,p->id);
                if(p->id == basemapId) break;

                QString tilename = QString::fromLatin1(p->name);
                // produce a unique key form the tile name and it's ID. Some tiles
                // might have the same name but never the same ID
                //QString key = QString("%1 %2").arg(tilename).arg(p->id,8,10,QChar('0'));
                QString key = QString("%1").arg(p->id,8,10,QChar('0'));

                tile_t& tile    = tiles[key];
                tile.id         = p->id;
                tile.key        = key;
                tile.name       = tilename;
                tile.file.sprintf("%08i.img",p->id);
                tile.file = finfo.dir().filePath(tile.file);

                //                 qDebug() << tile.file;

                tile.north  = GARMIN_RAD((gar_load(int32_t,p->north) >> 8) & 0x00FFFFFF);
                tile.east   = GARMIN_RAD((gar_load(int32_t,p->east) >> 8)  & 0x00FFFFFF);
                tile.south  = GARMIN_RAD((gar_load(int32_t,p->south) >> 8) & 0x00FFFFFF);
                tile.west   = GARMIN_RAD((gar_load(int32_t,p->west) >> 8)  & 0x00FFFFFF);
                tile.area   = QRectF(QPointF(tile.west, tile.north), QPointF(tile.east, tile.south));

                tile.defAreaU << tile.west << tile.east << tile.east << tile.west;
                tile.defAreaV << tile.north << tile.north << tile.south << tile.south;
                tile.defArea  << QPointF(tile.west, tile.north) << QPointF(tile.east, tile.north) << QPointF(tile.east, tile.south) << QPointF(tile.west, tile.south);

                tile.memSize = 0;
                tdb_map_size_t * s = (tdb_map_size_t*)(p->name + tilename.size() + 1);

                for(quint16 i=0; i < s->count; ++i) {
                    tile.memSize += gar_load(uint32_t,s->sizes[i]);
                }

                try
                {
                    tile.img = new CGarminTile(this);
                    tile.img->readBasics(tile.file);
                }
                catch(CGarminTile::exce_t e) {

                    if(e.err == CGarminTile::errLock) {
                        if(!mapkey.isEmpty()) break;

                        QMessageBox::warning(0,tr("Error"),e.msg,QMessageBox::Abort,QMessageBox::Abort);
                        // help is on the way!!!
                        mapkey = QInputDialog::getText(0,tr("However ...")
                            ,tr("<p><b>However ...</b></p>"
                            "<p>as I can read the basemap, and the information from the *tdb file,<br/>"
                            "I am able to let you select the map tiles for upload. To do this I<br/>"
                            "need the unlock key (25 digits) for this map, as it has to be uploaded<br/>"
                            "to the unit together with the map.</p>"
                            ));
                        // no money, no brother, no sister - no key
                        if(mapkey.isEmpty()) {
                            deleteLater();
                            return;
                        }

                        QSettings cfg;
                        cfg.beginGroup("garmin/maps");
                        cfg.beginGroup(name);
                        cfg.setValue("key",mapkey);
                        cfg.endGroup();
                        cfg.endGroup();

                    }
                    else {
                        if(!tainted) {
                            QMessageBox::warning(0,tr("Error"),e.msg,QMessageBox::Ok,QMessageBox::Ok);
                            tainted = true;
                        }
                        delete tile.img;
                        tile.img = 0;
                    }
                }
            }
            break;

            case 0x44:
            {
                QString str;
                QTextStream out(&str,QIODevice::WriteOnly);

                out << "<h1>" << name << "</h1>" << endl;

                tdb_copyrights_t * p = (tdb_copyrights_t*)pRecord;
                tdb_copyright_t  * c = &p->entry;
                while((void*)c < (void*)((quint8*)p + gar_load(uint16_t,p->size) + 3)) {

                    if(c->type != 0x07) {
                        out << c->str << "<br/>" << endl;
                    }
                    c = (tdb_copyright_t*)((quint8*)c + 4 + strlen(c->str) + 1);
                }

                copyright += str;

                //                 qDebug() << "copyright:" << copyright;
            }
            break;
        }

        pRecord = (tdb_hdr_t*)((quint8*)pRecord + pRecord->size + sizeof(tdb_hdr_t));
    }
}


bool CMapTDB::processPrimaryMapData()
{
    try
    {
        baseimg = new CGarminTile(this);
        baseimg->readBasics(basemap);
    }
    catch(CGarminTile::exce_t e) {
        // no basemap? bad luck!
        QMessageBox::warning(0,tr("Error"),e.msg,QMessageBox::Ok,QMessageBox::NoButton);
        deleteLater();
        return false;
    }

    qDebug() << "name:\t\t" << name;
    qDebug() << "basemap:\t" << basemap;
    qDebug() << "dimensions:\t" << "N" << north << "E" << east << "S" << south << "W" << west;

    const QMap<QString,CGarminTile::subfile_desc_t>& subfiles            = baseimg->getSubFiles();
    QMap<QString,CGarminTile::subfile_desc_t>::const_iterator subfile    = subfiles.begin();
    quint8 fewest_map_bits = 0xFF;

    /* Put here so the submap check doesn't do the basemap again. */
    QMap<QString,CGarminTile::subfile_desc_t>::const_iterator basemap_subfile;

    /* Find best candidate for basemap. */
    while (subfile != subfiles.end()) {
        QVector<CGarminTile::maplevel_t>::const_iterator maplevel = subfile->maplevels.begin();
        /* Skip any upper levels that contain no real data. */
        while (!maplevel->inherited) {
            ++maplevel;
        }
        /* Check for the least detailed map. */
        if (maplevel->bits < fewest_map_bits) {
            fewest_map_bits = maplevel->bits;
            basemap_subfile = subfile;
        }
        ++subfile;
    }

    /* Add all basemap levels to the list. */
    quint8 largestBitsBasemap = 0;
    QVector<CGarminTile::maplevel_t>::const_iterator maplevel = basemap_subfile->maplevels.begin();
    while(maplevel != basemap_subfile->maplevels.end()) {
        if (!maplevel->inherited) {
            map_level_t ml;
            ml.bits  = maplevel->bits;
            ml.level = maplevel->level;
            ml.useBaseMap = true;
            maplevels << ml;

            if(ml.bits > largestBitsBasemap) largestBitsBasemap = ml.bits;
        }
        ++maplevel;
    }

    if(!tiles.isEmpty()) {
        CGarminTile * img = 0;
        QMap<QString,tile_t>::iterator tile = tiles.begin();
        while(tile != tiles.end()) {
            img = tile->img;
            if(img) break;
            ++tile;
        }
        if(img) {
            const QMap<QString,CGarminTile::subfile_desc_t>& subfiles = img->getSubFiles();
            QMap<QString,CGarminTile::subfile_desc_t>::const_iterator subfile = subfiles.begin();
            /*
             * Query all subfiles for possible maplevels.
             * Exclude basemap to avoid polution.
             */
            while (subfile != subfiles.end()) {
                QVector<CGarminTile::maplevel_t>::const_iterator maplevel = subfile->maplevels.begin();

                /* Skip basemap. */
                if (subfile == basemap_subfile) {
                    ++subfile;
                    continue;
                }
                while (maplevel != subfile->maplevels.end()) {
                    if (!maplevel->inherited && (maplevel->bits > largestBitsBasemap)) {
                        map_level_t ml;
                        ml.bits  = maplevel->bits;
                        ml.level = maplevel->level;
                        ml.useBaseMap = false;
                        maplevels << ml;
                    }
                    ++maplevel;
                }
                ++subfile;
            }

            //             isTransparent = img->isTransparent();
        }
        /* Sort all entries, note that stable sort should insure that basemap is preferred when available. */
        qStableSort(maplevels.begin(), maplevels.end(), map_level_t::GreaterThan);
        /* Delete any duplicates for obvious performance reasons. */
        QVector<map_level_t>::iterator where;
        where = std::unique(maplevels.begin(), maplevels.end());
        maplevels.erase(where, maplevels.end());
    }

#ifdef DEBUG_SHOW_MAPLEVELS
    for(int i=0; i < maplevels.count(); ++i) {
        map_level_t& ml = maplevels[i];
        qDebug() << ml.bits << ml.level << ml.useBaseMap;
    }
#endif

    // read basemap for tile boundary polygons
    // Search all basemap levels for tile boundaries.
    // More detailed polygons will overwrite least detailed ones
    polygons.clear();
    for(int i=0; i < maplevels.count(); ++i) {
        if(!maplevels[i].useBaseMap) break;

        baseimg->loadPolygonsOfType(polygons, 0x4a, maplevels[i].level);

        polytype_t::iterator item = polygons.begin();
        while (item != polygons.end()) {
            if((item->labels.size() > 1) && tiles.contains(item->labels[1])) {
                tile_t& tile = tiles[item->labels[1]];

                double * u = item->u.data();
                double * v = item->v.data();
                int N      = item->u.size();

                tile.defArea.clear();

                for(int n = 0; n < N; ++n, ++u, ++v) {
                    tile.defArea << QPointF(*u, *v);
                }

                tile.defAreaU = item->u;
                tile.defAreaV = item->v;

            }
            ++item;
        }
    }

    return true;
}


void CMapTDB::convertPt2M(double& u, double& v)
{
    XY pt = topLeft;
    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    u = pt.u + u * zoomFactor * lon_factor;
    v = pt.v + v * zoomFactor * lat_factor;
}


void CMapTDB::convertM2Pt(double& u, double& v)
{
    XY pt = topLeft;
    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    u = (u - pt.u) / (zoomFactor * lon_factor);
    v = (v - pt.v) / (zoomFactor * lat_factor);
}


void CMapTDB::convertM2Pt(double* u, double* v, int n)
{
    XY pt = topLeft;
    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    for(int i = 0; i < n; ++i, ++u, ++v) {
        *u = (*u - pt.u) / (zoomFactor * lon_factor);
        *v = (*v - pt.v) / (zoomFactor * lat_factor);
    }
};

void CMapTDB::convertRad2Pt(double* u, double* v, int n)
{
    if(pjsrc == 0) {
        return;
    }

    pj_transform(pjtar,pjsrc,n,0,u,v,0);
    convertM2Pt(u,v,n);
}


void CMapTDB::move(const QPoint& old, const QPoint& next)
{
    XY p2 = topLeft;
    IMap::convertRad2Pt(p2.u, p2.v);

    // move top left point by difference
    p2.u += old.x() - next.x();
    p2.v += old.y() - next.y();

    // convert back to new top left geo coordinate
    convertPt2Rad(p2.u, p2.v);
    topLeft = p2;

    needsRedraw = true;
    setFastDraw();
    emit sigChanged();
}


void CMapTDB::zoom(bool zoomIn, const QPoint& p0)
{
    XY p1;

    needsRedraw = true;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    zoomidx += zoomIn ? +1 : -1;
    // sigChanged will be sent at the end of this function
    blockSignals(true);
    zoom(zoomidx);

    // convert geo. coordinates back to point
    IMap::convertRad2Pt(p1.u, p1.v);

    XY p2 = topLeft;
    IMap::convertRad2Pt(p2.u, p2.v);

    // move top left point by difference point befor and after zoom
    p2.u += p1.u - p0.x();
    p2.v += p1.v - p0.y();

    // convert back to new top left geo coordinate
    convertPt2Rad(p2.u, p2.v);
    topLeft = p2;

    blockSignals(false);
    emit sigChanged();

    //     qDebug() << "maplevel" /*<< mapLevelMap << "(" << mapLevelOvl << ")"*/ << "bits" << scales[zoomidx].bits;
}


void CMapTDB::zoom(double lon1, double lat1, double lon2, double lat2)
{
    double u[3];
    double v[3];
    double dU, dV;

    needsRedraw = true;

    u[0] = lon1;
    v[0] = lat1;
    u[1] = lon2;
    v[1] = lat1;
    u[2] = lon1;
    v[2] = lat2;

    pj_transform(pjtar, pjsrc,3,0,u,v,0);
    dU = u[1] - u[0];
    dV = v[2] - v[0];

    for(int i = MAX_IDX_ZOOM; i >= MIN_IDX_ZOOM; --i) {

        double z    = scales[i].scale;
        double pxU  = dU / (+1.0 * z);
        double pxV  = dV / (-1.0 * z);

        if((pxU < size.width()) && (pxV < size.height())) {
            zoomFactor  = z;
            zoomidx     = i;
            double u = lon1 + (lon2 - lon1)/2;
            double v = lat1 + (lat2 - lat1)/2;
            IMap::convertRad2Pt(u,v);
            move(QPoint(u,v), rect.center());
            return;
        }
    }
}


void CMapTDB::zoom(qint32& level)
{
    needsRedraw = true;

    zoomidx = level;
    if(zoomidx < MIN_IDX_ZOOM) zoomidx = MIN_IDX_ZOOM;
    if(zoomidx > MAX_IDX_ZOOM) zoomidx = MAX_IDX_ZOOM;
    zoomFactor = scales[zoomidx].scale;

    qDebug() << zoomidx << zoomFactor << scales[zoomidx].bits << scales[zoomidx].scale << scales[zoomidx].label;

    setFastDraw();

    emit sigChanged();
}


void CMapTDB::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = west;
    lat1 = north;
    lon2 = east;
    lat2 = south;
}


void CMapTDB::getArea_n_Scaling(XY& p1, XY& p2, float& my_xscale, float& my_yscale)
{
    p1 = topLeft;
    p2 = bottomRight;

    XY p3;
    p3.u = p1.u;
    p3.v = p2.v;

    my_xscale = zoomFactor * lon_factor;
    my_yscale = zoomFactor * lat_factor;

}


void CMapTDB::draw(QPainter& p)
{
    bottomRight.u = size.width();
    bottomRight.v = size.height();
    convertPt2Rad(bottomRight.u, bottomRight.v);

    if((bottomRight.u < 0) && (bottomRight.u < topLeft.u)) {
        bottomRight.u = 180 * DEG_TO_RAD + (180 * DEG_TO_RAD + bottomRight.u );
    }

    if((topLeft.u > 0) && (topLeft.u > bottomRight.u)) {
        topLeft.u = -180 * DEG_TO_RAD - (180 * DEG_TO_RAD - topLeft.u);
    }

    // render map if necessary
    if(needsRedraw) {
        draw();
    }
    // copy internal buffer to paint device
    p.drawImage(0,0,buffer);

    // render overlay
    if(!ovlMap.isNull() && !doFastDraw) {
        ovlMap->draw(size, needsRedraw, p);
    }

    needsRedraw = false;

    if(!infotext.isEmpty() && info) {
        QFont f = p.font();
        f.setBold(false);
        f.setItalic(false);
        if(f.pointSize() < 8) {
            f.setPointSize(8);
        }
        info->setDefaultFont(f);
        info->setHtml(infotext);

        p.save();
        p.translate(topLeftInfo);

        QRectF rectInfo(QPointF(0,0), info->size());
        rectInfo.adjust(0,0,4,4);
        rectInfo.moveTopLeft(QPointF(-2,-2));

        p.setPen(Qt::black);
        p.setBrush(QColor(0xff, 0xff, 0xcc, 0xE0));
        p.drawRect(rectInfo);
        info->drawContents(&p);

        p.restore();
    }

    p.drawPixmap(pointFocus - QPoint(5,5), QPixmap(":/icons/small_bullet_yellow.png"));

    // draw detail scaling
    p.setPen(Qt::darkBlue);
    p.setBrush(QColor(255,255,255,100));
    if(mouseOverDecDetail) {
        p.drawPixmap(rectDecDetail.topLeft(), QPixmap(":/icons/iconMinus32x32.png").scaled(40,40));
    }
    else {
        p.drawPixmap(rectDecDetail.topLeft(), QPixmap(":/icons/iconMinus32x32.png"));
    }

    if(mouseOverIncDetail) {
        p.drawPixmap(rectIncDetail.topLeft(), QPixmap(":/icons/iconAdd32x32.png").scaled(40,40));
    }
    else {
        p.drawPixmap(rectIncDetail, QPixmap(":/icons/iconAdd32x32.png"));
    }

    QString str;
    if(detailsFineTune < 0) {
        str = tr("details %1").arg(detailsFineTune);
    }
    else if(detailsFineTune > 0) {
        str = tr("details +%1").arg(detailsFineTune);
    }
    else {
        str = tr("details %1").arg(detailsFineTune);
    }
    QFont font = p.font();

    p.save();

    font.setPixelSize(20);

    p.setPen(Qt::white);
    p.setFont(font);

    p.drawText(rectDetail.translated(-1,-1 + 5), Qt::AlignCenter, str);
    p.drawText(rectDetail.translated( 0,-1 + 5), Qt::AlignCenter, str);
    p.drawText(rectDetail.translated(+1,-1 + 5), Qt::AlignCenter, str);

    p.drawText(rectDetail.translated(-1, 0 + 5), Qt::AlignCenter, str);
    p.drawText(rectDetail.translated(+1, 0 + 5), Qt::AlignCenter, str);

    p.drawText(rectDetail.translated(-1,+1 + 5), Qt::AlignCenter, str);
    p.drawText(rectDetail.translated( 0,+1 + 5), Qt::AlignCenter, str);
    p.drawText(rectDetail.translated(+1,+1 + 5), Qt::AlignCenter, str);

    p.setPen(Qt::darkBlue);
    p.drawText(rectDetail.translated( 0, 0 + 5), Qt::AlignCenter, str);

    p.restore();

    if(doFastDraw) setFastDraw();
}


void CMapTDB::draw(const QSize& s, bool needsRedraw, QPainter& p)
{
    int i;

    if(s != size) {
        resize(s);
        needsRedraw = true;
        doFastDraw  = false;
    }

    if(needsRedraw) {
        float sx, sy;
        getArea_n_Scaling_fromBase(topLeft, bottomRight, sx, sy);

        zoomFactor = sx/lon_factor;
        lat_factor = sy/sx * lon_factor;

        for(i=0; i < MAX_IDX_ZOOM; ++i) {
            if(scales[i].scale <= sx) break;
        }

        zoomidx     = i;

        draw();

        // make map semi transparent
        quint32 * ptr  = (quint32*)buffer.bits();
        for(int i = 0; i < (buffer.numBytes()>>2); ++i) {
            if(*ptr & 0xFF000000) {
                *ptr = (*ptr & 0x00FFFFFF) | 0xB0000000;
            }
            ++ptr;
        }
    }

    p.drawImage(0,0,buffer);

    if(ovlMap) ovlMap->draw(s, needsRedraw, p);
}


void CMapTDB::draw()
{
    buffer.fill(Qt::transparent);
    QPainter p(&buffer);

    QFont f = CResources::self().getMapFont();
    fm = QFontMetrics(f);

    p.setFont(f);
    p.setPen(Qt::black);
    p.setBrush(Qt::NoBrush);

    quint8 bits = scales[zoomidx].bits + detailsFineTune;
    QVector<map_level_t>::const_iterator maplevel = maplevels.end();
    do {
        --maplevel;
        if(bits >= maplevel->bits) break;
    } while(maplevel != maplevels.begin());

    QRectF viewport(QPointF(topLeft.u, topLeft.v), QPointF(bottomRight.u, bottomRight.v));
    polygons.clear();
    polylines.clear();
    pois.clear();
    points.clear();
    labels.clear();

    if(maplevel->useBaseMap) {
        baseimg->loadVisibleData(doFastDraw, polygons, polylines, points, pois, maplevel->level, viewport);
    }
    else {
        QMap<QString,tile_t>::const_iterator tile = tiles.begin();
        while(tile != tiles.end()) {
            if(tile->img && tile->area.intersects(viewport)) {
                tile->img->loadVisibleData(doFastDraw, polygons, polylines, points, pois, maplevel->level, viewport);
            }
            ++tile;
        }
    }

    p.setRenderHint(QPainter::Antialiasing,!doFastDraw);

    if(/*!doFastDraw && */!isTransparent) {
        drawPolygons(p, polygons);
    }

    // needs to be removed
    QPen pen(Qt::red, 30);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    polytype_t::iterator item = query1.begin();
    while(item != query1.end()) {
        QVector<double> lon = item->u;
        QVector<double> lat = item->v;
        double * u      = lon.data();
        double * v      = lat.data();
        const int size  = lon.size();

        convertRad2Pt(u,v,size);
        QPolygonF line(size);

        for(int i = 0; i < size; ++i) {
            line[i].setX(*u++);
            line[i].setY(*v++);
        }

        p.drawPolyline(line);

        ++item;
    }
    ////////////////////////


    if(!doFastDraw) {
        drawPolylines(p, polylines);
        // needs to be removed
        p.setPen(QColor("#FFB000"));
        p.setBrush(QColor("#FFB000"));
        pointtype_t::iterator item = query2.begin();
        while(item != query2.end()) {
            double u = item->lon;
            double v = item->lat;
            IMap::convertRad2Pt(u,v);
            p.drawEllipse(u - 16, v - 16, 30, 30);
            ++item;
        }
        ////////////////////////

        drawPoints(p, points);
        drawPois(p, pois);
        drawText(p);
        drawLabels(p, labels);

    }

    p.setRenderHint(QPainter::Antialiasing,false);
}

#ifdef NEW_TYP_PARSER
#define STREETNAME_THRESHOLD 5.0

void CMapTDB::drawLine(QPainter& p, CGarminPolygon& l, IGarminTyp::polyline_property& property, QFontMetricsF& metrics, QFont& font)
{
    QPolygonF line;
    double * u          = l.u.data();
    double * v          = l.v.data();
    const int size      = l.u.size();
    const int lineWidth = p.pen().width();

    if(size < 2){
        return;
    }

    convertRad2Pt(u,v,size);

    line.clear();
    line.resize(size);

    for(int i = 0; i < size; ++i) {
        line[i].setX(*u++);
        line[i].setY(*v++);
    }

    if (zoomFactor < STREETNAME_THRESHOLD && property.labelType != IGarminTyp::eNone) {
        collectText(l, line, font, metrics, lineWidth);
    }

    p.drawPolyline(line);
}

void CMapTDB::drawLine(QPainter& p, CGarminPolygon& l)
{
    QPolygonF line;
    double * u          = l.u.data();
    double * v          = l.v.data();
    const int size      = l.u.size();

    if(size < 2){
        return;
    }

    line.clear();
    line.resize(size);

    for(int i = 0; i < size; ++i) {
        line[i].setX(*u++);
        line[i].setY(*v++);
    }

    p.drawPolyline(line);
}

void CMapTDB::drawPolylines(QPainter& p, polytype_t& lines)
{
    textpaths.clear();
    QFont font = CResources::self().getMapFont();

    int fontsize = 6 + 3.0/zoomFactor;
    font.setPixelSize(fontsize);
    font.setBold(false);
    QFontMetricsF metrics(font);

    QList<quint32>keys = polylineProperties.keys();
    qSort(keys);
    quint32 type;

    foreach(type, keys) {
        IGarminTyp::polyline_property& property = polylineProperties[type];

        if(property.hasPixmap){
            QImage pixmap = nightView ? property.imgNight : property.imgDay;
            const double w          = pixmap.width();
            const double h          = pixmap.height();

            polytype_t::iterator item = lines.begin();
            while(item != lines.end()) {
                if(item->type == type) {
                    QPolygonF line;

                    double * u      = item->u.data();
                    double * v      = item->v.data();
                    const int size  = item->u.size();

                    if(size < 2){
                        ++item;
                        continue;
                    }

                    convertRad2Pt(u,v,size);

                    line.clear();
                    line.resize(size);

                    QVector<double> lengths;
                    double u1, u2, v1, v2;
                    QPainterPath path;
                    double segLength, totalLength = 0;
                    int i;

                    u1 = u[0];
                    v1 = v[0];
                    line[0].setX(u1);
                    line[0].setY(v1);

                    for(i = 1; i < size; ++i) {
                        u2 = u[i];
                        v2 = v[i];

                        line[i].setX(u2);
                        line[i].setY(v2);

                        segLength    = sqrt((u2 - u1) * (u2 - u1) + (v2 - v1) * (v2 - v1));
                        totalLength += segLength;
                        lengths << segLength;

                        u1 = u2;
                        v1 = v2;
                    }

                    if (zoomFactor < STREETNAME_THRESHOLD && property.labelType != IGarminTyp::eNone) {
                        collectText((*item), line, font, metrics, h);
                    }


                    path.addPolygon(line);
                    const int nLength = lengths.count();

                    double curLength = 0;
                    for(int i = 0; i < nLength; ++i) {
                        segLength = lengths[i];

                        //                         qDebug() << curLength << totalLength << curLength / totalLength;

                        QPointF p1      = path.pointAtPercent(curLength / totalLength);
                        QPointF p2      = path.pointAtPercent((curLength + segLength) / totalLength);
                        double angle    = atan((p2.y() - p1.y()) / (p2.x() - p1.x())) * 180 / PI;

                        double l = 0;
                        QRectF r = pixmap.rect();

                        if(p2.x() - p1.x() < 0) {
                            angle += 180;
                        }

                        p.save();
                        p.translate(p1);
                        p.rotate(angle);
                        p.translate(0,-h/2);

                        while(l < segLength) {
                            if((segLength - l) < w) {
                                r.setRight(segLength - l);
                                p.setClipRect(r, Qt::ReplaceClip);
                            }
                            p.drawImage(0,0,pixmap);
                            p.translate(w,0);
                            l += w;
                        }
                        p.restore();
                        curLength += segLength;
                    }
                }
                ++item;
            }
        }
        else{
            if(property.hasBorder){
                // draw background line 1st
                p.setPen(nightView ? property.penBorderNight : property.penBorderDay);
                polytype_t::iterator item = lines.begin();
                while(item != lines.end()) {
                    if(item->type == type) {
                        drawLine(p, *item, property, metrics, font);
                    }
                    ++item;
                }
                // draw foreground line 2nd
                p.setPen(nightView ? property.penLineNight : property.penLineDay);
                item = lines.begin();
                while(item != lines.end()) {
                    if(item->type == type) {
                        drawLine(p, *item);
                    }
                    ++item;
                }
            }
            else{
                p.setPen(nightView ? property.penLineNight : property.penLineDay);
                polytype_t::iterator item = lines.begin();
                while(item != lines.end()) {
                    if(item->type == type) {
                        drawLine(p, *item, property, metrics, font);
                    }
                    ++item;
                }
            }
        }

    }
}
#else
static quint16 polylineDrawOrder[]  =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    ,0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    ,0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F
    ,0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

#define DYN_WIDTH_THRESHOLD 15.0
#define STREETNAME_THRESHOLD 5.0
#define MAX_STREET_WIDTH 20
void CMapTDB::drawPolylines(QPainter& p, polytype_t& lines)
{
    // clear text list
    QPolygonF line;
    textpaths.clear();
    QFont font = CResources::self().getMapFont();

    int fontsize = 6 + 3.0/zoomFactor;
    font.setPixelSize(fontsize);
    font.setBold(false);
    QFontMetricsF metrics(font);

    QList<quint32>keys = polylineProperties.keys();
    qSort(keys);
    quint32 type;

    foreach(type, keys) {
        IGarminTyp::polyline_property& property = polylineProperties[type];

        //         qDebug() << hex << type << property.pen1.color() << (property.pen1 == Qt::NoPen);

        QPen pen = property.penLineDay;
        p.setPen(pen);

        QImage& pixmap          = property.imgDay;
        const bool usePixmap    = !pixmap.isNull();
        const double w          = pixmap.width();
        const double h          = pixmap.height();
        qint32 lineWidth        = p.pen().width();
        if(!property.imgDay.isNull()) {
            lineWidth = property.imgDay.height();
        }

        polytype_t::iterator item = lines.begin();
        while(item != lines.end()) {
            if(item->type == type) {

                double * u      = item->u.data();
                double * v      = item->v.data();
                const int size  = item->u.size();

                if(size < 2){
                    ++item;
                    continue;
                }

                convertRad2Pt(u,v,size);

                line.clear();
                line.resize(size);

                if(!usePixmap) {
                    for(int i = 0; i < size; ++i) {
                        line[i].setX(*u++);
                        line[i].setY(*v++);
                    }

                    if (zoomFactor < STREETNAME_THRESHOLD && property.labelType != IGarminTyp::eNone) {
                        collectText((*item), line, font, metrics, lineWidth);
                    }

                    p.drawPolyline(line);
                }
                else {
                    if(doFastDraw) {
                        ++item;
                        continue;
                    }

                    QVector<double> lengths;
                    double u1, u2, v1, v2;
                    QPainterPath path;
                    double segLength, totalLength = 0;
                    int i;

                    u1 = u[0];
                    v1 = v[0];
                    line[0].setX(u1);
                    line[0].setY(v1);

                    for(i = 1; i < size; ++i) {
                        u2 = u[i];
                        v2 = v[i];

                        line[i].setX(u2);
                        line[i].setY(v2);

                        segLength    = sqrt((u2 - u1) * (u2 - u1) + (v2 - v1) * (v2 - v1));
                        totalLength += segLength;
                        lengths << segLength;

                        u1 = u2;
                        v1 = v2;
                    }

                    if (zoomFactor < STREETNAME_THRESHOLD && property.showText) {
                        collectText((*item), line, font, metrics, lineWidth);
                    }


                    path.addPolygon(line);
                    const int nLength = lengths.count();

                    double curLength = 0;
                    for(int i = 0; i < nLength; ++i) {
                        segLength = lengths[i];

                        //                         qDebug() << curLength << totalLength << curLength / totalLength;

                        QPointF p1      = path.pointAtPercent(curLength / totalLength);
                        QPointF p2      = path.pointAtPercent((curLength + segLength) / totalLength);
                        double angle    = atan((p2.y() - p1.y()) / (p2.x() - p1.x())) * 180 / PI;

                        double l = 0;
                        QRectF r = pixmap.rect();

                        if(p2.x() - p1.x() < 0) {
                            angle += 180;
                        }

                        p.save();
                        p.translate(p1);
                        p.rotate(angle);
                        p.translate(0,-h/2);

                        while(l < segLength) {
                            if((segLength - l) < w) {
                                r.setRight(segLength - l);
                                p.setClipRect(r, Qt::ReplaceClip);
                            }
                            p.drawImage(0,0,pixmap);
                            p.translate(w,0);
                            l += w;
                        }
                        p.restore();
                        curLength += segLength;
                    }
                }
            }

            ++item;
        }
    }

}
#endif

void CMapTDB::collectText(CGarminPolygon& item, QPolygonF& line,  QFont& font, QFontMetricsF metrics, qint32 lineWidth)
{

    QString str;
    if (!item.labels.isEmpty()) {

        switch(item.type) {
            case 0x23:
            case 0x20:
            case 0x24:
            case 0x21:
            case 0x25:
            case 0x22:
            {
                QString unit;
                QString val = item.labels[0];
                IUnit::self().meter2elevation(val.toFloat() / 3.28084f, val, unit);
                str = QString("%1 %2").arg(val).arg(unit);
            }
            break;

            default:
                str = item.labels.join(" ").simplified();
        }
    }

    if(str.isEmpty()) return;

    textpath_t tp;
    tp.path.addPolygon(line);
    tp.font = font;
    tp.text = str;
    tp.lineWidth = lineWidth;

    QPointF p1 = line[0];
    const int size = line.size();
    for(int i = 1; i < size; ++i) {
        QPointF p2  = line[i];
        qreal dx    = p2.x() - p1.x();
        qreal dy    = p2.y() - p1.y();
        tp.lengths << sqrt(dx * dx + dy * dy);
        p1 = p2;
    }

    textpaths << tp;
}


void CMapTDB::drawText(QPainter& p)
{
    p.setPen(Qt::black);

    QVector<textpath_t>::iterator textpath = textpaths.begin();
    while(textpath != textpaths.end()) {
        QFont& font         = textpath->font;
        QFontMetricsF fm(font);
        QPainterPath& path  = textpath->path;

        // get path length and string length
        qreal length        = fabs(path.length());
        qreal width         = fm.width(textpath->text);

        // adjust font size until string fits into polyline
        while(width > (length * 0.7)) {
            font.setPixelSize(font.pixelSize() - 1);
            fm      = QFontMetricsF(font);
            width   = fm.width(textpath->text);

            if((font.pixelSize() < 8)) break;
        }

        // no way to draw a readable string - skip
        if((font.pixelSize() < 8)) {
            ++textpath;
            continue;
        }

        // adjust exact offset to first half of segment
        const QVector<qreal>& lengths = textpath->lengths;
        const qreal ref = (length - width) / 2;
        qreal offset    = 0;

        for(int i = 0; i < lengths.size(); ++i) {
            const qreal d = lengths[i];

            if((offset + d/2) >= ref) {
                offset = ref;
                break;
            }
            if((offset + d) >= ref) {
                offset += d/2;
                break;
            }
            offset += d;
        }

        // get starting angle of first two letters
        QString& text   = textpath->text;
        qreal percent1  =  offset / length;
        qreal percent2  = (offset + fm.width(text.left(2))) / length;

        QPointF point1  = path.pointAtPercent(percent1);
        QPointF point2  = path.pointAtPercent(percent2);

        qreal angle     = atan((point2.y() - point1.y()) / (point2.x() - point1.x())) * 180 / PI;

        // flip path if string start is E->W direction
        // this helps, sometimes, in 50 % of the cases :)
        if(point2.x() - point1.x() < 0) {
            path    = path.toReversed();
        }

        p.setFont(textpath->font);

        // draw string letter by letter and adjust angle
        const int size = text.size();
        for(int i = 0; i < size; ++i) {

            percent1  =  offset / length;
            percent2  = (offset + fm.width(text[i])) / length;

            point1  = path.pointAtPercent(percent1);
            point2  = path.pointAtPercent(percent2);

            angle   = atan((point2.y() - point1.y()) / (point2.x() - point1.x())) * 180 / PI;

            if(point2.x() - point1.x() < 0) {
                angle += 180;
            }

            p.save();
            p.translate(point1);
            p.rotate(angle);
            if(textAboveLine) {
                p.translate(0, -(textpath->lineWidth + 2));
            }
            else {
                p.translate(0, fm.descent());
            }
            p.drawText(0,0,text.mid(i,1));
            p.restore();

            offset += fm.width(text[i]);
        }

        ++textpath;
    }

}

#ifdef NEW_TYP_PARSER
void CMapTDB::drawPolygons(QPainter& p, polytype_t& lines)
{
    quint32 type;
    int n;
    const int N = polygonDrawOrder.size();
    for(n = 0; n < N; ++n) {
        type = polygonDrawOrder[(N-1) - n];

        p.setPen(polygonProperties[type].pen);
        p.setBrush(nightView ? polygonProperties[type].brushNight : polygonProperties[type].brushDay);

        polytype_t::iterator item = lines.begin();
        while (item != lines.end()) {

            if(item->type != type) {
                ++item;
                continue;
            }

            double * u      = item->u.data();
            double * v      = item->v.data();
            const int size  = item->u.size();

            convertRad2Pt(u,v,size);

            QPolygonF line(size);
            for(int i = 0; i < size; ++i) {
                line[i].setX(*u++);
                line[i].setY(*v++);
            }

            p.drawPolygon(line);

            if(!polygonProperties[type].known) qDebug() << "unknown polygon" << hex << type;
            ++item;
        }
    }

}
#else
void CMapTDB::drawPolygons(QPainter& p, polytype_t& lines)
{

    int n;
    const int N = polygonDrawOrder.size();
    QSet<quint32> extendedTypes;
    quint32 type;

    // do old types 1st
    for(n = 0; n < N; ++n) {
        type = polygonDrawOrder[(N-1) - n];

        p.setPen(polygonProperties[type].pen);
        p.setBrush( nightView ? polygonProperties[type].brushNight : polygonProperties[type].brushDay);

        polytype_t::iterator item = lines.begin();
        while (item != lines.end()) {
            /// @todo do extended types in this loop once new typ file parsing works
            // collect extended types, wise to do it like that?
            if(item->type & 0x10000) {
                extendedTypes << item->type;
                ++item;
                continue;
            }
            if((item->type & 0x7F) == type) {
                double * u      = item->u.data();
                double * v      = item->v.data();
                const int size  = item->u.size();

                convertRad2Pt(u,v,size);

                QPolygonF line(size);
                for(int i = 0; i < size; ++i) {
                    line[i].setX(*u++);
                    line[i].setY(*v++);
                }

                p.drawPolygon(line);
                if(!polygonProperties[type].known) qDebug() << "unknown polygon" << hex << type;
            }
            ++item;
        }
    }

    // do extended types
    foreach(type, extendedTypes) {
        p.setPen(polygonProperties[type].pen);
        p.setBrush(polygonProperties[type].brushDay);

        polytype_t::iterator item = lines.begin();
        while (item != lines.end()) {
            if(item->type == type) {
                double * u      = item->u.data();
                double * v      = item->v.data();
                const int size  = item->u.size();

                convertRad2Pt(u,v,size);

                QPolygonF line(size);
                for(int i = 0; i < size; ++i) {
                    line[i].setX(*u++);
                    line[i].setY(*v++);
                }

                p.save();
                IGarminTyp::polygon_property& property = polygonProperties[item->type];
                p.setPen(property.pen);
                p.setBrush(property.brushDay);
                p.drawPolygon(line);
                p.restore();
                if(!property.known) qDebug() << "unknown polygon" << hex << item->type;

            }
            ++item;
        }
    }
}
#endif

void CMapTDB::drawPoints(QPainter& p, pointtype_t& pts)
{

    pointtype_t::iterator pt = pts.begin();
    while(pt != pts.end()) {

        if((pt->type > 0x1600) && (zoomFactor > 5.0)) {
            ++pt;
            continue;
        };

        IMap::convertRad2Pt(pt->lon, pt->lat);

        if(pointProperties.contains(pt->type)) {
            p.drawImage(pt->lon - 4, pt->lat - 4, pointProperties[pt->type]);
        }
        else {
            p.drawPixmap(pt->lon - 4, pt->lat - 4, QPixmap(":/icons/small_bullet_blue.png"));
        }

        if(!pt->labels.isEmpty() && ((zoomFactor < 2) || (pt->type < 0x1600))  && poiLabels) {

            // calculate bounding rectangle with a border of 2 px
            QRect rect = fm.boundingRect(pt->labels.join(" "));
            rect.adjust(0,0,4,4);
            rect.moveCenter(QPoint(pt->lon, pt->lat));

            // test rectangle for intersection with existng labels
            QVector<strlbl_t>::const_iterator label = labels.begin();
            while(label != labels.end()) {
                if(label->rect.intersects(rect)) break;
                ++label;
            }

            // if no intersection was found, add label to list
            if(label == labels.end()) {
                labels.push_back(strlbl_t());
                strlbl_t& strlbl = labels.last();
                strlbl.pt   = QPoint(pt->lon, pt->lat);
                strlbl.str  = pt->labels.join(" ");
                strlbl.rect = rect;
            }
        }
        ++pt;
    }
}


void CMapTDB::drawPois(QPainter& p, pointtype_t& pts)
{
    pointtype_t::iterator pt = pts.begin();
    while(pt != pts.end()) {
        IMap::convertRad2Pt(pt->lon, pt->lat);

        if(pointProperties.contains(pt->type)) {
            p.drawImage(pt->lon - 4, pt->lat - 4, pointProperties[pt->type]);
        }
        else {
            p.drawPixmap(pt->lon - 4, pt->lat - 4, QPixmap(":/icons/small_bullet_red.png"));
        }

        if(!pt->labels.isEmpty() && (zoomFactor < 2) && poiLabels) {

            // calculate bounding rectangle with a border of 2 px
            QRect rect = fm.boundingRect(pt->labels.join(" "));
            rect.adjust(0,0,4,4);
            rect.moveCenter(QPoint(pt->lon, pt->lat));

            // test rectangle for intersection with existng labels
            QVector<strlbl_t>::const_iterator label = labels.begin();
            while(label != labels.end()) {
                if(label->rect.intersects(rect)) break;
                ++label;
            }

            // if no intersection was found, add label to list
            if(label == labels.end()) {
                labels.push_back(strlbl_t());
                strlbl_t& strlbl = labels.last();
                strlbl.pt   = QPoint(pt->lon, pt->lat);
                strlbl.str  = pt->labels.join(" ");
                strlbl.rect = rect;
            }
        }
        ++pt;
    }
}


void CMapTDB::drawLabels(QPainter& p, QVector<strlbl_t> lbls)
{
    QVector<strlbl_t>::const_iterator lbl = lbls.begin();
    while(lbl != lbls.end()) {
        CCanvas::drawText(lbl->str, p, lbl->pt, Qt::black);
        ++lbl;
    }
}


void CMapTDB::getInfoPoints(const QPoint& pt, QMultiMap<QString, QString>& dict)
{
    pointtype_t::const_iterator point = points.begin();
    while(point != points.end()) {
        QPoint x = pt - QPoint(point->lon, point->lat);
        if(x.manhattanLength() < 10) {
            dict.insert(tr("Point of Interest"),point->labels.join( ", " ) + QString(" (%1)").arg(point->type,2,16,QChar('0')));
        }
        ++point;
    }

}


void CMapTDB::getInfoPois(const QPoint& pt, QMultiMap<QString, QString>& dict)
{
    pointtype_t::const_iterator point = pois.begin();
    while(point != pois.end()) {
        QPoint x = pt - QPoint(point->lon, point->lat);
        if(x.manhattanLength() < 10) {
            dict.insert(tr("Point of Interest"),point->labels.join( ", " ) + QString(" (%1)").arg(point->type,2,16,QChar('0')));
        }
        ++point;
    }
}


void CMapTDB::getInfoPolylines(QPoint& pt, QMultiMap<QString, QString>& dict)
{
    int i = 0;                   // index into poly line
    int len;                     // number of points in line
    XY p1, p2;                   // the two points of the polyline close to pt
    double dx,dy;                // delta x and y defined by p1 and p2
    double d_p1_p2;              // distance between p1 and p2
    double u;                    // ratio u the tangent point will divide d_p1_p2
    double x,y;                  // coord. (x,y) of the point on line defined by [p1,p2] close to pt
    double distance;             // the distance to the polyline
    double shortest;             // shortest distance sofar

    QPointF resPt = pt;
    QString key, value;
    quint32 type = 0;

    shortest = 50;

    polytype_t::const_iterator line = polylines.begin();
    while(line != polylines.end()) {
        len = line->u.count();
        // need at least 2 points
        if(len < 2) {
            ++line;
            continue;
        }

        // see http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
        for(i=1; i<len; ++i) {
            p1.u = line->u[i-1];
            p1.v = line->v[i-1];
            p2.u = line->u[i];
            p2.v = line->v[i];

            dx = p2.u - p1.u;
            dy = p2.v - p1.v;

            d_p1_p2 = sqrt(dx * dx + dy * dy);

            u = ((pt.x() - p1.u) * dx + (pt.y() - p1.v) * dy) / (d_p1_p2 * d_p1_p2);

            if(u < 0.0 || u > 1.0) continue;

            x = p1.u + u * dx;
            y = p1.v + u * dy;

            distance = sqrt((x - pt.x())*(x - pt.x()) + (y - pt.y())*(y - pt.y()));

            if(distance < shortest) {
                type = line->type;

                if(!line->labels.isEmpty()) {
                    switch(type) {
                                 // "Minor depht contour"
                        case 0x23:
                                 // "Minor land contour"
                        case 0x20:
                                 // "Intermediate depth contour",
                        case 0x24:
                                 // "Intermediate land contour",
                        case 0x21:
                                 // "Major depth contour",
                        case 0x25:
                                 // "Major land contour",
                        case 0x22:
                        {
                            QString unit;
                            QString val = line->labels[0];
                            IUnit::self().meter2elevation(val.toFloat() / 3.28084f, val, unit);
                            value = QString("%1 %2").arg(val).arg(unit);
                        }
                        break;

                        default:
                            value = line->labels.join(" ").simplified();
                    }
                }
                else {
                    value = "-";
                }
                resPt.setX(x);
                resPt.setY(y);
                shortest = distance;
            }

        }
        ++line;
    }

    if(type < 0x40){
        key  = polyline_typestr[type];
    }
    else{
        key.clear();
    }

    if(key.isEmpty()){
        QStringList strlist = polylineProperties[type].strings.values();
        if(strlist.size()){
            key =  strlist.first().simplified();
        }
    }
    if(!key.isEmpty()) {
        dict.insert(key + QString("(%1)").arg(type,2,16,QChar('0')),value);
    }

    pt = resPt.toPoint();
}


void CMapTDB::getInfoPolygons(const QPoint& pt, QMultiMap<QString, QString>& dict)
{
    int     npol;
    int     i = 0, j = 0 ,c = 0;
    XY      p1, p2;              // the two points of the polyline close to pt
    double  x = pt.x();
    double  y = pt.y();
    QString value;

    polytype_t::const_iterator line = polygons.begin();
    while(line != polygons.end()) {

        npol = line->u.count();
        if(npol > 2) {
            c = 0;
            // see http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/
            for (i = 0, j = npol-1; i < npol; j = i++) {
                p1.u = line->u[j];
                p1.v = line->v[j];
                p2.u = line->u[i];
                p2.v = line->v[i];

                if ((((p2.v <= y) && (y < p1.v))  || ((p1.v <= y) && (y < p2.v))) &&
                (x < (p1.u - p2.u) * (y - p2.v) / (p1.v - p2.v) + p2.u)) {
                    c = !c;
                }
            }

            if(c){
                if(line->labels.isEmpty()){
                    QStringList strlist = polygonProperties[line->type].strings.values();
                    if(strlist.size()){
                        dict.insert(tr("Area"), strlist.first().simplified()  + QString(" (%1)").arg(line->type,2,16,QChar('0')));
                    }
                }
                else{
                    dict.insert(tr("Area"), line->labels.join(" ").simplified()  + QString(" (%1)").arg(line->type,2,16,QChar('0')));
                }
            }
            //             if(c) dict.insert(tr("Polygon"), QString("0x%1").arg(line->type, 0, 16, QChar('0')));

        }
        ++line;
    }
}


void CMapTDB::select(IMapSelection& ms, const QRect& rect)
{
    CMapSelectionGarmin& sel = (CMapSelectionGarmin&)ms;

    double lon1 = rect.left();
    double lat1 = rect.top();
    convertPt2Rad(lon1, lat1);

    double lon2 = rect.right();
    double lat2 = rect.bottom();
    convertPt2Rad(lon2, lat2);

    QPolygonF poly;
    poly << QPointF(lon1, lat1) << QPointF(lon2, lat1) << QPointF(lon2, lat2) << QPointF(lon1, lat2);

    if(!sel.maps.contains(key)) {
        CMapSelectionGarmin::map_t& m = sel.maps[key];
        m.unlockKey = mapkey;
        m.name      = name;
        m.typfile   = typfile;
        m.pid       = pid;
        m.fid       = fid;
    }
    QMap<QString, CMapSelectionGarmin::map_t>::iterator map = sel.maps.find(key);

    QMap<QString,tile_t>::iterator tile = tiles.begin();
    while(tile != tiles.end()) {
        QPolygonF res = poly.intersected(tile->defArea);

        if(!res.isEmpty()) {

            if(map->tiles.contains(tile->key)) {
                map->tiles.remove(tile->key);
            }
            else {
                CMapSelectionGarmin::tile_t t;
                t.id        = tile->id;
                t.name      = tile->name;
                t.filename  = tile->file;
                t.u         = tile->defAreaU;
                t.v         = tile->defAreaV;
                t.memSize   = tile->memSize;
                t.area      = tile->area;
                t.pid       = pid;
                t.fid       = fid;
                map->tiles[tile->key] = t;
            }
        }

        ++tile;
    }

    sel.maps[key] = *map;
    sel.calcArea();

    quint32 memSize = sel.getMemSize();
    sel.description += QString("\nSize: %1 MB").arg(double(memSize) / (1024 * 1024), 0,'f',2);
    sel.description += QString("\nTiles: #%1").arg(sel.tilecnt);
}


void CMapTDB::readASCIIString(QDataStream& ds, QString& str)
{
    str.clear();
    quint8 byte;
    ds >> byte;
    while(byte != 0) {
        str.append(byte);
        ds >> byte;
    }
}


void CMapTDB::readColorTableAlpha(QDataStream &in, QImage &img, int colors, int maxcolors)
{
    quint8  byte;
    quint32 bits = 0;
    quint32 reg  = 0;
    quint32 mask = 0x000000FF;

    img.setNumColors(maxcolors);
    for (int i = 0; i < maxcolors; i++) {
        if(i < colors) {
            while(bits < 28) {
                in >> byte;
                mask = 0x000000FF << bits;
                reg  = reg  & (~mask);
                reg  = reg  | (byte << bits);
                bits += 8;
            }

            img.setColor(i, qRgba((reg >> 16) & 0x0FF, (reg >> 8) & 0x0FF, reg & 0x0FF, ~((reg >> 24) & 0x0F) << 4));

            reg   = reg >> 28;
            bits -= 28;
        }
        else {
            img.setColor(i, qRgba(0,0,0,0));
        }
    }
}


void CMapTDB::readColorTable(QDataStream &in, QImage &img, int colors, int maxcolors)
{
    quint8 r,g,b;

    img.setNumColors(maxcolors);
    for (int i = 0; i < maxcolors; i++) {
        if(i < colors) {
            in >> b >> g >> r;
            img.setColor(i, qRgb(r,g,b));
        }
        else {
            img.setColor(i, qRgba(0,0,0,0));
        }
    }
}


void CMapTDB::readColorTableInv(QDataStream &in, QImage &img, int colors, int maxcolors)
{
    quint8 r,g,b;

    img.setNumColors(maxcolors);
    for (int i = 0; i < maxcolors; i++) {
        if(i < colors) {
            in >> b >> g >> r;
            img.setColor(maxcolors - i - 1, qRgb(r,g,b));
        }
        else {
            img.setColor(maxcolors - i - 1, qRgba(0,0,0,0));
        }
    }
}

#define DBG
void CMapTDB::readTYP()
{
    int i;
    QFileInfo fi(filename);
    QDir path = fi.absoluteDir();
    QStringList filters;

    filters << "*.typ" << "*.TYP";
    QStringList typfiles = path.entryList(filters, QDir::Files);

    if(typfiles.isEmpty()) return;

#ifdef DBG
    QString f;
    foreach(f, typfiles){

    typfile = path.absoluteFilePath(f);
    QFile file(path.absoluteFilePath(f));
#else
    typfile = path.absoluteFilePath(typfiles[0]);
    QFile file(path.absoluteFilePath(typfiles[0]));
#endif
    qDebug() << file.fileName();
    file.open(QIODevice::ReadOnly);

    QDataStream in(&file);
    in.setByteOrder( QDataStream::LittleEndian);

    /* Read typ file descriptor */
    quint16 descriptor;
    in >> descriptor;

#ifdef NEW_TYP_PARSER

    qDebug() << "descriptor" << hex << descriptor;
    switch(descriptor)
    {
        case 0x5B:
        {
            CGarminTyp typ(0);
            typ.decode(in, polygonProperties, polylineProperties, polygonDrawOrder, pointProperties);
            break;
        }

        case 0x6E:
        {
            CGarminTypNT typ(0);
            typ.decode(in, polygonProperties, polylineProperties, polygonDrawOrder, pointProperties);
            break;
        }

        default:
            qDebug() << "CMapTDB::readTYP() not a known typ file = " << descriptor;
            QMessageBox::warning(0, tr("Warning..."), tr("Unknown typ file format. Use http://ati.land.cz/gps/typdecomp/editor.cgi to convert file to known format."), QMessageBox::Abort, QMessageBox::Abort);
            return;
    }

#else
    if ( (descriptor != 0x5b) && (descriptor != 0x6e)  && (descriptor!=0x9c) ) {
        qDebug() << "CMapTDB::readTYP() not a known typ file = " << descriptor;
        return;
    }

    /* Check Garmin string */
    QString garmintyp;
    quint8 byte;

    for(i = 0; i < 10; ++i) {
        in >> byte;
        garmintyp.append(byte);
    }
    garmintyp.append(0);
    if(garmintyp != "GARMIN TYP") {
        qDebug() << "CMapTDB::readTYP() not a known typ file = " << descriptor;
        return;
    }

    /* reading typ creation date string */
    quint16 startDate, endDate, year;
    quint8 month, day, hour, minutes, seconds;

    in.device()->seek(0x0c);
    in >> startDate >> year >> month >> day >> hour >> minutes >> seconds >> endDate;
    month -= 1;                  /* Month are like Microsoft starting 0 ? */
    year += 1900;

    typ_section_t sectPoints;
    typ_section_t sectPolylines;
    typ_section_t sectPolygons;
    typ_section_t sectOrder;

    /* Reading points / lines / polygons struct */
    in >> sectPoints.dataOffset >> sectPoints.dataLength;
    in >> sectPolylines.dataOffset >> sectPolylines.dataLength;
    in >> sectPolygons.dataOffset >> sectPolygons.dataLength;

    in >> pid >> fid;
    qDebug() << "PID" << hex << pid << "FID" << hex << fid;

    /* Read Array datas */
    in >> sectPoints.arrayOffset >> sectPoints.arrayModulo >> sectPoints.arraySize;
    in >> sectPolylines.arrayOffset  >> sectPolylines.arrayModulo  >> sectPolylines.arraySize;
    in >> sectPolygons.arrayOffset >> sectPolygons.arrayModulo >> sectPolygons.arraySize;
    in >> sectOrder.arrayOffset >> sectOrder.arrayModulo >> sectOrder.arraySize;

    processTypDrawOrder(in, sectOrder);
    processTypPolygons(in, sectPolygons);
    processTypPolyline(in, sectPolylines);
    processTypPois(in, sectPoints);

#endif //NEW_TYP_PARSER
    file.close();

#ifdef DBG
    }
#endif
}


void CMapTDB::processTypDrawOrder(QDataStream& in, const typ_section_t& section)
{
    if(section.arrayModulo != 5) {
        return;
    }

    if(!section.arrayModulo || ((section.arraySize % section.arrayModulo) != 0)) {
        return;
    }

    in.device()->seek(section.arrayOffset);

    quint16 typ, a1;
    quint8 a2;
    int count=1;

    for (unsigned  i = 0; i < (section.arraySize / 5); i++) {
        in >> typ >> a1 >> a2;
        if (typ == 0) {
            count++;
        }
        else if(typ < 0x80) {
            //             qDebug() << QString("Type 0x%1 is priority %2").arg(typ,0,16).arg(count);
            int idx = polygonDrawOrder.indexOf(typ);
            if(idx != -1) {
                polygonDrawOrder.move(idx,0);
            }
        }
    }
}


void CMapTDB::processTypPolygons(QDataStream& in, const typ_section_t& section)
{
    bool tainted = false;

    if(!section.arrayModulo || ((section.arraySize % section.arrayModulo) != 0)) {
        return;
    }

    int nbElements = section.arraySize / section.arrayModulo;
    for (int element=0; element < nbElements; element++) {
        /* seek to position of element polyline */
        quint16 otyp, ofs;
        quint8 ofsc, x;
        quint32 wtyp, typ, subtyp;
        bool hasLocalization = false;

        in.device()->seek( section.arrayOffset + (section.arrayModulo * element ) );

        if (section.arrayModulo == 4) {
            in >> otyp >> ofs;
        }
        if (section.arrayModulo == 3) {
            in >> otyp >> ofsc;
            ofs = ofsc;
        }
        wtyp    = (otyp >> 5) | (( otyp & 0x1f) << 11);
        typ     = wtyp & 0x7f;
        subtyp  = otyp & 0x1F;

        if(otyp & 0x2000) {
            //             qDebug() << "Skiped: " <<  hex << typ << subtyp << wtyp << otyp ;
            //             continue;
            typ = 0x10000 + (typ << 8) + subtyp;
        }

        in.device()->seek(section.dataOffset + ofs);

        in >> x;
        hasLocalization = x & 0x10;

        QImage myXpm(32,32, QImage::Format_Indexed8 );
        int colorType   = x & 0x0f;
        quint8 r,g,b;

        //         qDebug() << "Changed: " << typ << subtyp << hex << typ << subtyp << colorType;

        if ( colorType == 6 ) {
            in >> b >> g >> r;
            polygonProperties[typ].brushDay    = QBrush(qRgb(r,g,b));
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
        }
        else if ( colorType == 7 ) {
            in >> b >> g >> r;
            polygonProperties[typ].brushDay    = QBrush(qRgb(r,g,b));
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
            in >> b >> g >> r;
        }
        else if ( colorType == 8 ) {
            myXpm.setNumColors(2);
            in >> b >> g >> r;
                                 // forground (day + night)
            myXpm.setColor(1, qRgb(r,g,b) );
            in >> b >> g >> r;
                                 // background (day + night)
            myXpm.setColor(0, qRgb(r,g,b) );

            decodeBitmap(in, myXpm, 32, 32, 1);
            polygonProperties[typ].brushDay.setTextureImage(myXpm);
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
        }

        else if ( colorType == 0xf ) {
            myXpm.setNumColors(2);
            in >> b >> g >> r;   // day forground
            myXpm.setColor(1, qRgb(r,g,b) );
            in >> b >> g >> r;   // night forground
                                 // background is always transparent
            myXpm.setColor(0, qRgba(255,255,255,0) );

            decodeBitmap(in, myXpm, 32, 32, 1);
            polygonProperties[typ].brushDay.setTextureImage(myXpm);
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
        }

        else if ( colorType == 9 ) {
            myXpm.setNumColors(2);
            in >> b >> g >> r;   // day background
            myXpm.setColor(1, qRgb(r,g,b) );
            in >> b >> g >> r;   // day forground
            myXpm.setColor(0, qRgb(r,g,b) );
            in >> b >> g >> r;   // night background
            in >> b >> g >> r;   // night foreground

            decodeBitmap(in, myXpm, 32, 32, 1);
            polygonProperties[typ].brushDay.setTextureImage(myXpm);
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
        }

        else if ( colorType == 0xb ) {
            myXpm.setNumColors(2);
            in >> b >> g >> r;
                                 // day forground
            myXpm.setColor(1, qRgb(r,g,b) );
            myXpm.setColor(0, qRgba(255,255,255,0) );

            in >> b >> g >> r;   // night forground
            //             myXpm.setColor(1, qRgb(r,g,b) );
            in >> b >> g >> r;   // night background
            //             myXpm.setColor(1, qRgb(r,g,b) );

            decodeBitmap(in, myXpm, 32, 32, 1);
            polygonProperties[typ].brushDay.setTextureImage(myXpm);
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
        }
        else if ( colorType == 0xe ) {
            myXpm.setNumColors(2);
            in >> b >> g >> r;
            myXpm.setColor(1, qRgb(r,g,b) );
            myXpm.setColor(0, qRgba(0,0,0,0) );

            decodeBitmap(in, myXpm, 32, 32, 1);
            polygonProperties[typ].brushDay.setTextureImage(myXpm);
            polygonProperties[typ].pen      = Qt::NoPen;
            polygonProperties[typ].known    = true;
        }
        else {
            if(!tainted) {
                QMessageBox::warning(0, tr("Warning..."), tr("This is a typ file with unknown polygon encoding. Please report!"), QMessageBox::Abort, QMessageBox::Abort);
                tainted = true;
            }
            qDebug() << "Failed polygon:" << typ << subtyp << hex << typ << subtyp << colorType;
        }
    }
}


void CMapTDB::processTypPolyline(QDataStream& in, const typ_section_t& section)
{
    bool tainted = false;

    if(!section.arrayModulo || ((section.arraySize % section.arrayModulo) != 0)) {
        return;
    }

    int nbElements = section.arraySize / section.arrayModulo;
    for (int element=0; element < nbElements; element++) {
        quint16 otyp, ofs;
        quint8 ofsc;
        int wtyp, typ, subtyp;

        in.device()->seek( section.arrayOffset + (section.arrayModulo * element ) );
        if (section.arrayModulo == 4) {
            in >> otyp >> ofs;
        }
        if (section.arrayModulo == 3) {
            in >> otyp >> ofsc;
            ofs = ofsc;
        }
        wtyp = (otyp >> 5) | (( otyp & 0x1f) << 11);
        typ = wtyp & 0xff;
        subtyp  = otyp & 0x1F;

        if(otyp & 0x2000) {
            typ = 0x10000 + (typ << 8) + subtyp;
        }

        in.device()->seek( section.dataOffset + ofs );

        quint8 data1, data2;
        in >> data1 >> data2;

        bool hasPixmap      = false;
        int colorFlag       = data1 & 0x07;
        int rows            = data1 >> 3;
        //         bool useOrientation = ( (data2 & 0x02) ? 1 :0 );
        QImage myXpmDay(32,rows ? rows : 1, QImage::Format_Indexed8 );
        QImage myXpmNight(32,rows ? rows : 1, QImage::Format_Indexed8 );

//         quint8 d1,d2,d3;
//         in >> d1 >> d2 >> d3;
//         qDebug() << "Line" << hex << typ <<  colorFlag << rows << d1 << d2 << d3;

        if ( colorFlag == 0) {
            readColorTableInv(in, myXpmDay, 2,2);
        }
        else if ( colorFlag == 1) {
                                 // day
            readColorTableInv(in, myXpmDay, 2,2);
                                 // night
            readColorTableInv(in, myXpmNight, 2,2);
        }
        else if ( colorFlag == 3) {
                                 // day
            readColorTableInv(in, myXpmDay, 2,2);
                                 // night
            readColorTableInv(in, myXpmNight, 1,2);
        }
        else if ( colorFlag == 5) {
            readColorTableInv(in, myXpmDay, 1,2);
        }
        else if ( colorFlag == 6) {
            readColorTableInv(in, myXpmDay, 1,2);
        }
        else if ( colorFlag == 7) {
                                 // day
            readColorTableInv(in, myXpmDay, 1,2);
                                 // night
            readColorTableInv(in, myXpmNight, 1,2);
        }
        else {
            if(!tainted) {
                QMessageBox::warning(0, tr("Warning..."), tr("This is a typ file with unknown polyline encoding. Please report!"), QMessageBox::Abort, QMessageBox::Abort);
                tainted = true;
            }

            qDebug() << "Failed polyline" <<  hex << ":" << typ <<  colorFlag << rows ;
            continue;
        }

        quint8 x1, x2, x3;
//         in >> x1 >> x2 >> x3;
//         qDebug() << "Line" << hex << typ <<  colorFlag << rows << x1 << x2;

        IGarminTyp::polyline_property& property = polylineProperties[typ];

        if(rows) {
            decodeBitmap(in, myXpmDay, 32, rows, 1);
            hasPixmap = true;

            property.imgDay = myXpmDay;
            //             myXpmDay.save(QString("l%1.png").arg(typ,2,16,QChar('0')));

        }
        else {
//             if(typ == 0x06){
//                 property.imgDay = residentialStreet(myXpmDay.color(1));
//             }
//             else if(typ == 0x01)
//             {
//                 property.imgDay = majorHighway(myXpmDay.color(1));
//             }
//             else if(typ == 0x02)
//             {
//                 property.imgDay = principalHighway(myXpmDay.color(1));
//             }
//             else if(typ == 0x03)
//             {
//                 property.imgDay = otherHighway(myXpmDay.color(1));
//             }
//             else if(typ == 0x04)
//             {
//                 property.imgDay = arterialRoad(myXpmDay.color(1));
//             }
//             else{
            property.penLineDay.setColor(myXpmDay.color(1));
//             }
        }

        property.known = true;
    }
}


void CMapTDB::processTypPois(QDataStream& in, const typ_section_t& section)
{
    bool tainted = false;

    if(!section.arrayModulo || ((section.arraySize % section.arrayModulo) != 0)) {
        return;
    }

    int nbElements = section.arraySize / section.arrayModulo;
    for (int element=0; element < nbElements; element++) {
        /* seek to position of element polyline */
        quint16 otyp, ofs;
        quint8 ofsc;
        quint32 wtyp, typ, subtyp;

        in.device()->seek( section.arrayOffset + (section.arrayModulo * element ) );
        if (section.arrayModulo == 4) {
            in >> otyp >> ofs;
        }
        if (section.arrayModulo == 3) {
            in >> otyp >> ofsc;
            ofs = ofsc;
        }

        wtyp = (otyp >> 5) | (( otyp & 0x1f) << 11);
        typ = wtyp & 0xff;
        subtyp  = otyp & 0x1F;

        if(otyp & 0x2000) {
            typ = 0x10000 + (typ << 8) + subtyp;
        }
        else {
            typ = (typ << 8) + subtyp;
        }

        /* Create element */
        in.device()->seek( section.dataOffset + ofs );

        quint8 a, w, h, colors, x3;
        int wBytes, bpp;
        in >> a >> w >> h >> colors >> x3;
        QImage myXpmDay(w,h, QImage::Format_Indexed8 );
        QImage myXpmNight(w,h, QImage::Format_Indexed8 );

        // openmtb bug on colors=15 colors, oe: changed back for garmin maps
        if ( colors > 15) {
            bpp = 8;
        }
        else {
            if (colors >= 3 ) {
                if ( (colors == 3) && (x3 == 0x20) ) {
                    bpp = 2;
                }
                else {
                    bpp = 4;
                }
            }
            else {
                bpp = 2;
            }
        }
        wBytes = (w * bpp) / 8;
        //         qDebug() << hex << typ << subtyp << QString(" A=0x%5 Size %1 x %2 with colors %3 and flags 0x%4 bpp=%6").arg(w).arg(h).arg(colors).arg(x3,0,16).arg(a,0,16).arg(bpp);

        int maxcolor = pow(2.0f,bpp);

        if ( ( a == 5 ) || ( a == 1 ) || ( a == 0xd ) || ( a == 0xb ) || ( a == 0x9)  || ( a == 0xf) || ( a == 0xe)) {
            if (x3 == 0x00) {
                readColorTable(in, myXpmDay, colors, maxcolor);
                // openmtb bug on bpp=4 colors
                if ( ( (bpp==4) || (bpp==8) ) && ( (colors==3) || (colors==15) ) ) {
                    bpp /=2;
                }
                //if(bpp == 4) bpp /= 2;
                decodeBitmap(in, myXpmDay, w, h, bpp);
                pointProperties[typ] = myXpmDay;
                //                 if(x3 == 0x00) myXpmDay.save(QString("poi%1%2.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));
                //                                 myXpmDay.save(QString("poi%1%2.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));
            }
            else if (x3 == 0x10) {
                readColorTable(in, myXpmDay, colors, maxcolor);
                decodeBitmap(in, myXpmDay, w, h, bpp);
                pointProperties[typ] = myXpmDay;
                //                 if(x3 == 0x00) myXpmDay.save(QString("poi%1%2.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));
                //                 myXpmDay.save(QString("poi%1%2.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));
            }
            else if (x3 == 0x20) {
                readColorTableAlpha(in, myXpmDay, colors, maxcolor);
                decodeBitmap(in, myXpmDay, w, h, bpp);
                pointProperties[typ] = myXpmDay;
                //                 if(x3 == 0x00) myXpmDay.save(QString("poi%1%2.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));
                //                                 myXpmDay.save(QString("poi%1%2.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));
            }
            else {
                if(!tainted) {
                    QMessageBox::warning(0, tr("Warning..."), tr("This is a typ file with unknown point encoding. Please report!"), QMessageBox::Abort, QMessageBox::Abort);
                    tainted = true;
                }
                qDebug() << "Failed:" << hex << typ << subtyp << QString(" A=0x%5 Size %1 x %2 with colors %3 and flags 0x%4 bpp=%6").arg(w).arg(h).arg(colors).arg(x3,0,16).arg(a,0,16).arg(bpp);
            }
        }
        else if ((a == 7)  || (a == 3)) {
            if (x3 == 0x10) {
                readColorTable(in, myXpmDay, colors, maxcolor);
            }
            else {
                readColorTableAlpha(in, myXpmDay, colors, maxcolor);
            }
            decodeBitmap(in, myXpmDay, w, h, bpp);
            pointProperties[typ] = myXpmDay;
            //             myXpmDay.save(QString("poi%1%2d.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));

            /* Get again colors and x3 flag */
            in >> colors >> x3;
            if ( colors >=16) bpp = 8;
            else if (colors >=3 ) {
                if ( (colors == 3) && (x3 == 0x20) ) {
                    bpp = 2;
                }
                else {
                    bpp = 4;
                }
            }
            else {
                bpp = 2;
            }
            wBytes = (w * bpp) / 8;

            if (x3 == 0x10) {
                readColorTable(in, myXpmNight, colors, maxcolor);
            }
            else {
                readColorTableAlpha(in, myXpmNight, colors, maxcolor);
            }
            decodeBitmap(in, myXpmNight, w, h, bpp);
            //             pointProperties[(typ << 8) | subtyp] = myXpmNight;
            //             myXpmNight.save(QString("poi%1%2n.png").arg(typ,2,16,QChar('0')).arg(subtyp,2,16,QChar('0')));

        }
        else {
            if(!tainted) {
                QMessageBox::warning(0, tr("Warning..."), tr("This is a typ file with unknown point encoding. Please report!"), QMessageBox::Abort, QMessageBox::Abort);
                tainted = true;
            }
            qDebug() << "Failed:" << hex << typ << subtyp << QString(" A=0x%5 Size %1 x %2 with colors %3 and flags 0x%4 bpp=%6").arg(w).arg(h).arg(colors).arg(x3,0,16).arg(a,0,16).arg(bpp);
        }
    }
}


void CMapTDB::decodeBitmap(QDataStream &in, QImage &img, int w, int h, int bpp)
{
    int x = 0,j = 0;
    quint8 color;
    for (int y = 0; y < h; y++) {
        while ( x < w ) {
            in >> color;

            for ( int i = 0; (i < (8 / bpp)) && (x < w) ; i++ ) {
                int value;
                if ( i > 0 ) {
                    value = (color >>= bpp);
                }
                else {
                    value = color;
                }
                if ( bpp == 4) value = value & 0xf;
                if ( bpp == 2) value = value & 0x3;
                if ( bpp == 1) value = value & 0x1;
                img.setPixel(x,y,value);
                //                 qDebug() << QString("value(%4) pixel at (%1,%2) is 0x%3 j is %5").arg(x).arg(y).arg(value,0,16).arg(color).arg(j);
                x += 1;
            }
            j += 1;
        }
        x = 0;
    }
}


void CMapTDB::createSearchIndex(QObject * receiver, const char * slot)
{
    QStringList files;
    QMap<QString,tile_t>::const_iterator tile = tiles.begin();
    while(tile != tiles.end()) {
        files << tile->file;
        ++tile;
    }
    connect(index, SIGNAL(sigProgress(const QString&, const int)), receiver, slot);
    index->create(files);
}


void CMapTDB::highlight(QVector<CGarminPolygon>& res)
{
    query1 = res;
    needsRedraw = true;
    emit sigChanged();
}


void CMapTDB::highlight(QVector<CGarminPoint>& res)
{
    query2 = res;
    needsRedraw = true;
    emit sigChanged();
}


void CMapTDB::config()
{
    CDlgMapTDBConfig dlg(this);
    dlg.exec();

    needsRedraw = true;

    if(useTyp) {
        readTYP();
    }
    else {
        setup();
    }
    emit sigChanged();
}

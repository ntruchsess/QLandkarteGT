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

#include "CMapQMAP.h"
#include "CMapLevel.h"
#include "CMapFile.h"
#include "CMapDEM.h"
#include "GeoMath.h"
#include "CCanvas.h"
#include "CMapSelectionRaster.h"
#include "CResources.h"
#include "CDlgMapQMAPConfig.h"

#include <QtGui>

#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <projects.h>
#include <math.h>
#ifdef __MINGW32__
#undef LP
#endif

CMapQMAP::CMapQMAP(const QString& key, const QString& fn, CCanvas * parent)
: IMap(eRaster, key,parent)
, pMaplevel(0)
, zoomFactor(1)
, foundMap(false)
, quadraticZoom(false)
{
    filename = fn;

    QDir path = QFileInfo(filename).absolutePath();
    // load map definition
    QSettings mapdef(filename,QSettings::IniFormat);
    int nLevels = mapdef.value("main/levels",0).toInt();

    info  = "<h1>" + mapdef.value("description/comment", "").toString() + "</h1>";

    quadraticZoom = mapdef.value("main/quadraticZoom",quadraticZoom).toBool();
    info += "<p>" + tr("Quadratic zoom %1").arg(quadraticZoom ? tr("enabled") : tr("disabled")) + "</p>";
    info += QString("<p><table><tr><th>%1</th><th>%2</th><th width='100%'>%3</th></tr>").arg(tr("Map Level")).arg(tr("Zoom Level")).arg(tr("Files"));

    // create map level list
    CMapLevel * maplevel = 0;
    for(int n=1; n <= nLevels; ++n)
    {
        mapdef.beginGroup(QString("level%1").arg(n));
        quint32 min = mapdef.value("zoomLevelMin",-1).toUInt();
        quint32 max = mapdef.value("zoomLevelMax",-1).toUInt();
        maplevel = new CMapLevel(min,max,this);

        // add GeoTiff files to map level
        QStringList files = mapdef.value("files","").toString().split("|", QString::SkipEmptyParts);
        if(files.count())
        {
            QString file;
            foreach(file,files)
            {
                maplevel->addMapFile(path.filePath(file));
            }
            maplevels << maplevel;
        }

        info += QString("<tr><td align='center'>%1</td><td align='center'>%2..%3</td><td>%4</td></tr>").arg(n).arg(min).arg(max).arg(files.join(", "));

        mapdef.endGroup();
    }
    info += "</table></p>";

    info += QString("<p><table><tr><th>%1</th><th width='100%'>%2</th></tr>").arg(tr("Parameter")).arg(tr("Value"));

    QString strTopLeft      = mapdef.value("description/topleft").toString();
    QString strBottomRight  = mapdef.value("description/bottomright").toString();

    info += QString("<tr><td>%1</td><td>%2</td></tr>").arg(tr("Top/Left")).arg(strTopLeft.replace("\260","&#176;"));
    info += QString("<tr><td>%1</td><td>%2</td></tr>").arg(tr("Bottom/Right")).arg(strBottomRight.replace("\260","&#176;"));

    if(!maplevels.isEmpty())
    {
        XY p1, p2;
        double a1,a2, width, height;
        float u1 = 0, v1 = 0, u2 = 0, v2 = 0;
        GPS_Math_Str_To_Deg(strTopLeft.replace("&#176;",""), u1, v1);
        GPS_Math_Str_To_Deg(strBottomRight.replace("&#176;",""), u2, v2);

        p1.u = u1 * DEG_TO_RAD;
        p1.v = v1 * DEG_TO_RAD;
        p2.u = u2 * DEG_TO_RAD;
        p2.v = v1 * DEG_TO_RAD;
        width   = distance(p1,p2,a1,a2)/1000;
        p1.u = u1 * DEG_TO_RAD;
        p1.v = v1 * DEG_TO_RAD;
        p2.u = u1 * DEG_TO_RAD;
        p2.v = v2 * DEG_TO_RAD;
        height  = distance(p1,p2,a1,a2)/1000;

        info += QString("<tr><td>%1</td><td>%2 km&#178; (%3 km x %4 km)</td></tr>").arg(tr("Area")).arg(width*height,0,'f',1).arg(width,0,'f',1).arg(height,0,'f',1);
        info += QString("<tr><td>%1</td><td>%2</td></tr>").arg(tr("Projection")).arg((*maplevels[0]->begin())->strProj);
    }


    info += "</table></p>";



    // If no configuration is stored read values from the map definition's "home" section
    // zoom() has to be called in either case to setup / initialize all other internal parameters
    mapdef.beginGroup(QString("home"));
    zoomidx = mapdef.value("zoom",1).toUInt();
    zoom(zoomidx);

    QString pos = mapdef.value("center","").toString();
    float u = topLeft.u;
    float v = topLeft.v;
    GPS_Math_Str_To_Deg(pos, u, v);

    topLeft.u = u * DEG_TO_RAD;
    topLeft.v = v * DEG_TO_RAD;
    mapdef.endGroup();

    QSettings cfg;
    exportPath  = cfg.value("path/export",cfg.value("path/maps","./")).toString();

    if(parent)
    {
        connect(parent, SIGNAL(sigResize(const QSize&)), this, SLOT(resize(const QSize&)));
        resize(parent->size());
    }

    qDebug() << "done";
}


CMapQMAP::~CMapQMAP()
{
    QSettings mapdef(filename,QSettings::IniFormat);
    mapdef.beginGroup(QString("home"));

    mapdef.setValue("zoom",zoomidx < 1 ? 1 : zoomidx);
    QString pos;

    GPS_Math_Deg_To_Str(topLeft.u * RAD_TO_DEG, topLeft.v * RAD_TO_DEG, pos);
    pos = pos.replace("\260","");
    mapdef.setValue("center",pos);
    mapdef.endGroup();

    QSettings cfg;
    cfg.setValue("path/export",exportPath);

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);
}


bool CMapQMAP::is32BitRgb()
{
    return pMaplevel.isNull() ? true : pMaplevel->is32BitRgb();
}


void CMapQMAP::draw(QPainter& p)
{
    if(pMaplevel.isNull() || pjsrc == 0)
    {
        IMap::draw(p);
        return;
    }

    // update internal buffer
    if(needsRedraw)
    {
        draw();
    }

    // copy internal buffer if valid
    if(!foundMap)
    {
        IMap::draw(p);
    }
    else
    {
        p.drawImage(0,0,buffer);
    }

    // render overlay
    if(!ovlMap.isNull() && !doFastDraw)
    {
        ovlMap->draw(size, needsRedraw, p);
    }

    needsRedraw = !foundMap;

    if(CResources::self().showZoomLevel())
    {
        QString str;
        if(zoomFactor < 1.0)
        {
            str = tr("Overzoom x%1").arg(1/zoomFactor,0,'f',0);
        }
        else
        {
            str = tr("Zoom level x%1").arg(zoomidx);
        }

        p.setPen(Qt::white);
        p.setFont(QFont("Sans Serif",14,QFont::Black));

        p.drawText(9  ,23, str);
        p.drawText(10 ,23, str);
        p.drawText(11 ,23, str);
        p.drawText(9  ,24, str);
        p.drawText(11 ,24, str);
        p.drawText(9  ,25, str);
        p.drawText(10 ,25, str);
        p.drawText(11 ,25, str);

        p.setPen(Qt::darkBlue);
        p.drawText(10,24,str);
    }
}


#include "CMapDB.h"
void CMapQMAP::__test()
{
    IMap& dem = CMapDB::self().getDEM();

    int c = 500, r = 500;

    QVector<float> ele1(c * r);
    float * ptr = ele1.data();
    int x, y = 0;
    for(y = 0; y < r; ++y)
    {
        for(x = 0; x < c; ++x)
        {
            double u = x;
            double v = y;
            convertPt2Rad(u,v);
            *ptr++ = dem.getElevation(u,v);
        }
    }
    printf("+++\n");

    XY p1, p2;
    p1.u = 0;
    p1.v = 0;
    p2.u = c - 1;
    p2.v = r - 1;
    convertPt2Rad(p1.u, p1.v);
    convertPt2Rad(p2.u, p2.v);

    QVector<float> ele2(c * r);
    dem.getRegion(ele2, p1, p2, c, r);

    printf("---------------------\n");

    for(y = 0 ; y < (c * r); y++)
    {
        if(ele1[y] != ele2[y])
        {
            qDebug() << "missmatch at " << y;
            break;
        }
    }
}


void CMapQMAP::draw()
{
    //     __test();

    buffer.fill(Qt::white);
    QPainter _p_(&buffer);

    foundMap = false;

    if(pjsrc == 0)
    {
        return;
    }

    const CMapFile * map = *pMaplevel->begin();

    // top left
    XY pt = topLeft;
    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    bottomRight.u = pt.u + size.width() * map->xscale * zoomFactor;
    bottomRight.v = pt.v + size.height() * map->yscale * zoomFactor;
    pj_transform(pjsrc,pjtar,1,0,&bottomRight.u,&bottomRight.v,0);

    // the viewport rectangel in [m]
    QRectF viewport(pt.u, pt.v, size.width() * map->xscale * zoomFactor,  size.height() * map->yscale * zoomFactor);
    float xscale = map->xscale, yscale = map->yscale;
    float xzoomFactor, yzoomFactor;

    // Iterate over all mapfiles within a maplevel. If a map's rectangel intersects with the
    // viewport rectangle, the part of the map within the intersecting rectangle has to be drawn.
    QVector<CMapFile*>::const_iterator mapfile = pMaplevel->begin();
    while(mapfile != pMaplevel->end())
    {

        map = *mapfile;

        QRectF maparea   = QRectF(QPointF(map->xref1, map->yref1), QPointF(map->xref2, map->yref2));
        QRectF intersect = viewport.intersected(maparea);

        if(intersect.isValid())
        {

            // x/y offset [pixel] into file matrix
            qint32 xoff = (intersect.left()   - map->xref1) / map->xscale;
            qint32 yoff = (intersect.bottom() - map->yref1) / map->yscale;

            // number of x/y pixel to read
            qint32 pxx  =   (qint32)(intersect.width()  / map->xscale);
            qint32 pxy  =  -(qint32)(intersect.height() / map->yscale);

            // all calculations should be in relation to a first map,
            // so need additional zoom factor
            xzoomFactor = xscale / (float) map->xscale;
            yzoomFactor = yscale / (float) map->yscale;

            // the final image width and height in pixel
            qint32 w    =   (qint32)(pxx / (zoomFactor * xzoomFactor)) & 0xFFFFFFFC;
            qint32 h    =   (qint32)(pxy / (zoomFactor * yzoomFactor));

            // correct pxx by truncation
            pxx         =   (qint32)(w * zoomFactor * xzoomFactor);

            if(w != 0 && h != 0)
            {

                CPLErr err = CE_Failure;

                if(map->rasterBandCount == 1)
                {

                    GDALRasterBand * pBand;
                    pBand = map->dataset->GetRasterBand(1);

                    QImage img(QSize(w,h),QImage::Format_Indexed8);
                    img.setColorTable(map->colortable);

                    CPLErr err = pBand->RasterIO(GF_Read
                        ,(int)xoff,(int)yoff
                        ,pxx,pxy
                        ,img.bits()
                        ,w,h
                        ,GDT_Byte,0,0);

                    if(!err)
                    {
                        double xx = intersect.left(), yy = intersect.bottom();
                        convertM2Pt(xx,yy);
                        _p_.drawImage(xx,yy,img);
                        foundMap = true;
                    }
                }
                else
                {
                    QImage img(w,h, QImage::Format_ARGB32);
                    QVector<quint8> buffer(w*h);

                    img.fill(qRgba(255,255,255,255));

                    for(int b = 1; b <= map->rasterBandCount; ++b)
                    {

                        GDALRasterBand * pBand;
                        pBand = map->dataset->GetRasterBand(b);

                        err = pBand->RasterIO(GF_Read, (int)xoff, (int)yoff, pxx, pxy, buffer.data(), w, h, GDT_Byte, 0, 0);

                        if(!err)
                        {

                            int offset;
                            switch(pBand->GetColorInterpretation())
                            {
                                case GCI_RedBand:   offset = 2; break;
                                case GCI_GreenBand: offset = 1; break;
                                case GCI_BlueBand:  offset = 0; break;
                                case GCI_AlphaBand: offset = 3; break;
                                default:            offset = -1;
                            }

                            if(offset >= 0 && offset <= 3)
                            {
                                quint8 * pTar   = img.bits() + offset;
                                quint8 * pSrc   = buffer.data();
                                const int size  = buffer.size();

                                for(int i = 0; i < size; ++i)
                                {
                                    *pTar = *pSrc;
                                    pTar += 4;
                                    pSrc += 1;
                                }
                            }
                        }
                    }

                    if(!err)
                    {
                        double xx = intersect.left(), yy = intersect.bottom();
                        convertM2Pt(xx,yy);
                        _p_.drawImage(xx,yy,img);
                        foundMap = true;
                    }
                }
            }
        }
        ++mapfile;
    }

    if(doFastDraw) setFastDrawTimer();
}


void CMapQMAP::convertPt2M(double& u, double& v)
{
    if(pMaplevel.isNull() || pjsrc == 0) return;

    const CMapFile * map = *pMaplevel->begin();

    XY pt = topLeft;
    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    u = pt.u + u * map->xscale * zoomFactor;
    v = pt.v + v * map->yscale * zoomFactor;
}


void CMapQMAP::convertM2Pt(double& u, double& v)
{
    if(pMaplevel.isNull() || pjsrc == 0) return;

    const CMapFile * map = *pMaplevel->begin();

    XY pt = topLeft;
    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    u = (u - pt.u) / (map->xscale * zoomFactor);
    v = (v - pt.v) / (map->yscale * zoomFactor);
}


void CMapQMAP::move(const QPoint& old, const QPoint& next)
{

    XY p2 = topLeft;
    convertRad2Pt(p2.u, p2.v);

    // move top left point by difference
    p2.u += old.x() - next.x();
    p2.v += old.y() - next.y();

    // convert back to new top left geo coordinate
    convertPt2Rad(p2.u, p2.v);
    topLeft = p2;

    needsRedraw = true;
    setFastDrawTimer();
    emit sigChanged();

    setAngleNorth();
}


void CMapQMAP::zoom(bool zoomIn, const QPoint& p0)
{
    XY p1;

    needsRedraw = true;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    if(quadraticZoom)
    {

        if(zoomidx > 1)
        {
            zoomidx = pow(2.0, ceil(log(zoomidx*1.0)/log(2.0)));
            zoomidx = zoomIn ? (zoomidx>>1) : (zoomidx<<1);
        }
        else
        {
            zoomidx += zoomIn ? -1 : 1;
        }
    }
    else
    {
        zoomidx += zoomIn ? -1 : 1;
    }

    // sigChanged will be sent at the end of this function
    blockSignals(true);
    zoom(zoomidx);

    // convert geo. coordinates back to point
    convertRad2Pt(p1.u, p1.v);

    XY p2 = topLeft;
    convertRad2Pt(p2.u, p2.v);

    // move top left point by difference point befor and after zoom
    p2.u += p1.u - p0.x();
    p2.v += p1.v - p0.y();

    // convert back to new top left geo coordinate
    convertPt2Rad(p2.u, p2.v);
    topLeft = p2;
    blockSignals(false);
    emit sigChanged();
}


qint32 CMapQMAP::getZoomLevel()
{
    if (zoomFactor < 1)
        return 1 / zoomFactor;
    else
        return zoomFactor + pMaplevel->min -1;
}


void CMapQMAP::zoom(qint32& level)
{
    needsRedraw = true;
    if(maplevels.isEmpty())
    {
        pMaplevel   = 0;
        pjsrc       = 0;
        return;
    }

    // no level less than 1
    if(level < 1)
    {
        zoomFactor  = 1.0 / - (level - 2);
        setFastDrawTimer();
        emit sigChanged();
        qDebug() << "zoom:" << zoomFactor;
        return;
    }

    QVector<CMapLevel*>::const_iterator maplevel = maplevels.begin();

    while(maplevel != maplevels.end())
    {
        if((*maplevel)->min <= level && level <= (*maplevel)->max)
        {
            break;
        }

        ++maplevel;
    }

    // no maplevel means level is larger than maximum level
    // thus the last (maximum) level is used.
    if(maplevel == maplevels.end())
    {
        --maplevel;
        level = (*maplevel)->max;
    }

    pMaplevel   = *maplevel;
    pjsrc       = (*pMaplevel->begin())->pj;
    zoomFactor  = level - (*maplevel)->min + 1;
    setFastDrawTimer();
    emit sigChanged();
    qDebug() << "zoom:" << zoomFactor;
}


void CMapQMAP::zoom(double lon1, double lat1, double lon2, double lat2)
{
    needsRedraw = true;
    if(maplevels.isEmpty() || (pjsrc == 0))
    {
        pMaplevel   = 0;
        pjsrc       = 0;
        return;
    }

    double u[3];
    double v[3];
    double dU, dV;

    qint32 level;
    QVector<CMapLevel*>::iterator maplevel = maplevels.begin();
    while(maplevel != maplevels.end())
    {
        u[0] = lon1;
        v[0] = lat1;
        u[1] = lon2;
        v[1] = lat1;
        u[2] = lon1;
        v[2] = lat2;

        pj_transform(pjtar, pjsrc,3,0,u,v,0);
        dU = u[1] - u[0];
        dV = v[2] - v[0];

        const CMapFile * map = *(*maplevel)->begin();

        for(level = (*maplevel)->min; level <= (*maplevel)->max; ++level)
        {
            int z = level - (*maplevel)->min + 1;
            double pxU = fabs(dU / (map->xscale * z));
            double pxV = fabs(dV / (map->yscale * z));

            qDebug() << pxU << size.width() << pxV << size.height();
            if((pxU < size.width()) && (pxV < size.height()))
            {
                pMaplevel   = *maplevel;
                pjsrc       = map->pj;

                zoomidx = pMaplevel->min + z - 1;
                if(quadraticZoom)
                {
                    zoomidx = pow(2.0, ceil(log(zoomidx*1.0)/log(2.0)));
                    z = zoomidx - pMaplevel->min + 1;
                }

                zoomFactor  = z;
                double u = lon1 + (lon2 - lon1)/2;
                double v = lat1 + (lat2 - lat1)/2;
                convertRad2Pt(u,v);
                move(QPoint(u,v), rect.center());
                emit sigChanged();
                return;
            }

        }
        ++maplevel;
    }
}


void CMapQMAP::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    if(pMaplevel.isNull())
    {
        lon1 = lat1 = lon2 = lat2 = 0;
        return;
    }

    pMaplevel->dimensions(lon1, lat1, lon2, lat2);
}


void CMapQMAP::getArea_n_Scaling(XY& p1, XY& p2, float& my_xscale, float& my_yscale)
{
    if(pMaplevel.isNull())
    {
        return;
    }
    const CMapFile * map = *pMaplevel->begin();

    p1          = topLeft;
    p2          = bottomRight;
    my_xscale   = map->xscale*zoomFactor;
    my_yscale   = map->yscale*zoomFactor;
}


void CMapQMAP::select(IMapSelection& ms, const QRect& rect)
{
    if(ms.type != IMapSelection::eRaster) return;

    CMapSelectionRaster& sel = (CMapSelectionRaster&)ms;

    sel.lon1 = rect.left();
    sel.lat1 = rect.top();
    convertPt2Rad(sel.lon1, sel.lat1);

    sel.lon2 = rect.right();
    sel.lat2 = rect.bottom();
    convertPt2Rad(sel.lon2, sel.lat2);

    sel.key = QString("%1%2%3").arg(sel.mapkey).arg(sel.lon1).arg(sel.lat1);

    QString pos1, pos2;
    GPS_Math_Deg_To_Str(sel.lon1 * RAD_TO_DEG, sel.lat1 * RAD_TO_DEG, pos1);
    GPS_Math_Deg_To_Str(sel.lon2 * RAD_TO_DEG, sel.lat2 * RAD_TO_DEG, pos2);

//    sel.description += "\n" + pos1 + "\n" + pos2;
}

quint32 CMapQMAP::scalePixelGrid(quint32 nPixel)
{
    CMapLevel * level       = maplevels[0];
    const CMapFile * file1  = *level->begin();
    const CMapFile * file2  = *pMaplevel->begin();
    return quint32((nPixel * file1->xscale)/(zoomFactor * file2->xscale) + 0.5);
}

void CMapQMAP::config()
{

    CDlgMapQMAPConfig * dlg = new CDlgMapQMAPConfig(this);
    dlg->show();

}


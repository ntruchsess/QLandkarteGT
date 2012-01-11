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

#include "CMapWms.h"
#include "CCanvas.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "CDiskCache.h"
#include <QtGui>
#include <QtXml>
#include <QtNetwork>


CMapWms::CMapWms(const QString &key, const QString &filename, CCanvas *parent)
: IMap(eWMS,key,parent)
, xsize_px(0)
, ysize_px(0)
, xscale(1.0)
, yscale(1.0)
, xref1(0)
, yref1(0)
, xref2(0)
, yref2(0)
, x(0)
, y(0)
, zoomFactor(1.0)
, status(0)
, quadraticZoom(0)
, needsRedrawOvl(true)
, lastTileLoaded(false)
{

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(parent, tr("Error..."), tr("Failed to open %1").arg(filename), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    QDomDocument dom;

    QString msg;
    int line, column;
    if(!dom.setContent(&file, true, &msg, &line, &column))
    {
        file.close();
        QMessageBox::critical(parent, tr("Error..."), tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    file.close();

    QDomElement gdal        =  dom.firstChildElement("GDAL_WMS");
    QDomElement service     = gdal.firstChildElement("Service");
    QDomElement datawindow  = gdal.firstChildElement("DataWindow");

    name        = service.firstChildElement("Title").text();
    urlstr      = service.firstChildElement("ServerUrl").text();
    format      = service.firstChildElement("ImageFormat").text();
    layers      = service.firstChildElement("Layers").text();
    srs         = service.firstChildElement("SRS").text();
    version     = service.firstChildElement("Version").text();
    copyright   = service.firstChildElement("Copyright").text();
    projection  = gdal.firstChildElement("Projection").text().toLower();
    blockSizeX  = gdal.firstChildElement("BlockSizeX").text().toUInt();
    blockSizeY  = gdal.firstChildElement("BlockSizeY").text().toUInt();

    if(srs.isEmpty())
    {
        srs = projection;
    }

    if(projection.isEmpty())
    {
        projection = srs;
    }


    projection = projection.toLower();
    if(projection.startsWith("epsg"))
    {
        QString str = QString("+init=%1").arg(projection);
        pjsrc = pj_init_plus(str.toLocal8Bit());
        qDebug() << "wms:" << str.toLocal8Bit();
    }
    else
    {
        pjsrc = pj_init_plus(projection.toLocal8Bit());
        qDebug() << "wms:" << projection.toLocal8Bit();
    }


    if(pjsrc == 0)
    {
        QMessageBox::critical(parent, tr("Error..."), tr("Unknown projection %1").arg(projection.toAscii().data()), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    oSRS.importFromProj4(getProjection());

    xsize_px    = datawindow.firstChildElement("SizeX").text().toInt();
    ysize_px    = datawindow.firstChildElement("SizeY").text().toInt();

    if (pj_is_latlong(pjsrc))
    {
        xref1   = datawindow.firstChildElement("UpperLeftX").text().toDouble()  * DEG_TO_RAD;
        yref1   = datawindow.firstChildElement("UpperLeftY").text().toDouble()  * DEG_TO_RAD;
        xref2   = datawindow.firstChildElement("LowerRightX").text().toDouble() * DEG_TO_RAD;
        yref2   = datawindow.firstChildElement("LowerRightY").text().toDouble() * DEG_TO_RAD;
    }
    else
    {
        xref1   = datawindow.firstChildElement("UpperLeftX").text().toDouble();
        yref1   = datawindow.firstChildElement("UpperLeftY").text().toDouble();
        xref2   = datawindow.firstChildElement("LowerRightX").text().toDouble();
        yref2   = datawindow.firstChildElement("LowerRightY").text().toDouble();
    }

    xscale      = (xref2 - xref1) / xsize_px;
    yscale      = (yref2 - yref1) / ysize_px;

    x           = xref1 + (xref2 - xref1) / 2;
    y           = yref1 + (yref2 - yref1) / 2;

    quadraticZoom = new QCheckBox(theMainWindow->getCanvas());
    quadraticZoom->setText(tr("quadratic zoom"));
    theMainWindow->statusBar()->insertPermanentWidget(0,quadraticZoom);

    status = new QLabel(theMainWindow->getCanvas());
    theMainWindow->statusBar()->insertPermanentWidget(0,status);

    QSettings cfg;
    quadraticZoom->setChecked(cfg.value("maps/quadraticZoom", false).toBool());

    accessManager = new QNetworkAccessManager(this);
    accessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));
    connect(accessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));

    diskCache = new CDiskCache(this);

    cfg.beginGroup("wms/maps");
    cfg.beginGroup(getKey());

    x = cfg.value("lon", x).toDouble();
    y = cfg.value("lat", y).toDouble();

    cfg.endGroup();
    cfg.endGroup();

    zoom(zoomidx);
}

CMapWms::~CMapWms()
{
    if(pjsrc) pj_free(pjsrc);

    if(quadraticZoom)
    {
        QSettings cfg;
        cfg.setValue("maps/quadraticZoom", quadraticZoom->isChecked());
        delete quadraticZoom;
    }

    delete status;

    QSettings cfg;
    cfg.beginGroup("wms/maps");
    cfg.beginGroup(getKey());

    cfg.setValue("lon", x);
    cfg.setValue("lat", y);

    cfg.endGroup();
    cfg.endGroup();
}

void CMapWms::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}

void CMapWms::convertM2Pt(double& u, double& v)
{
    u = (u - x) / (xscale * zoomFactor);
    v = (v - y) / (yscale * zoomFactor);
}

void CMapWms::convertPt2Pixel(double& u, double& v)
{
    convertPt2M(u,v);

    u = (u - xref1) / xscale;
    v = (v - yref1) / yscale;

    if(u < 0 || u > xsize_px)
    {
        u = -1;
        v = -1;
        return;
    }
    if(v < 0 || v > ysize_px)
    {
        u = -1;
        v = -1;
        return;
    }

}


void CMapWms::move(const QPoint& old, const QPoint& next)
{
    if(pjsrc == 0) return;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference
    xx += old.x() - next.x();
    yy += old.y() - next.y();

    convertPt2M(xx,yy);
    x = xx;
    y = yy;
    emit sigChanged();

    setAngleNorth();
}

void CMapWms::zoom(bool zoomIn, const QPoint& p0)
{
    qDebug() << "zoom" << zoomIn;

    XY p1;
    if(pjsrc == 0) return;

    needsRedraw     = true;
    needsRedrawOvl  = true;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    if(quadraticZoom->isChecked())
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

    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference point befor and after zoom
    xx += p1.u - p0.x();
    yy += p1.v - p0.y();

    // convert back to new top left geo coordinate
    convertPt2M(xx, yy);
    x = xx;
    y = yy;
    blockSignals(false);
    emit sigChanged();
}

void CMapWms::zoom(double lon1, double lat1, double lon2, double lat2)
{
    if(pjsrc == 0) return;

    needsRedraw     = true;
    needsRedrawOvl  = true;

    double u[3];
    double v[3];
    double dU, dV;

    u[0] = lon1;
    v[0] = lat1;
    u[1] = lon2;
    v[1] = lat1;
    u[2] = lon1;
    v[2] = lat2;

    pj_transform(pjtar, pjsrc,3,0,u,v,0);
    dU = (u[1] - u[0]) / xscale;
    dV = (v[0] - v[2]) / yscale;

    int z1 = fabs(dU / size.width());
    int z2 = fabs(dV / size.height());

    zoomFactor = (z1 > z2 ? z1 : z2)  + 1;
    if(quadraticZoom->isChecked())
    {
        zoomFactor = zoomidx = pow(2.0, ceil(log(zoomFactor)/log(2.0)));
    }
    else
    {
        zoomidx = zoomFactor;
    }

    double u_ = lon1 + (lon2 - lon1)/2;
    double v_ = lat1 + (lat2 - lat1)/2;
    convertRad2Pt(u_,v_);
    move(QPoint(u_,v_), rect.center());

    emit sigChanged();

    qDebug() << "zoom:" << zoomFactor;
}

void CMapWms::zoom(qint32& level)
{
    if(pjsrc == 0) return;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    // no level less than 1
    if(level < 1)
    {
        zoomFactor  = 1.0 / - (level - 2);
        qDebug() << "zoom:" << zoomFactor;
        return;
    }
    zoomFactor = level;
    emit sigChanged();
    qDebug() << "zoom:" << zoomFactor;
}

void CMapWms::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = xref1;
    lat1 = yref1;
    pj_transform(pjsrc,pjtar,1,0,&lon1,&lat1,0);

    lon2 = xref2;
    lat2 = yref2;
    pj_transform(pjsrc,pjtar,1,0,&lon2,&lat2,0);
}

void CMapWms::getArea_n_Scaling(XY& p1, XY& p2, float& my_xscale, float& my_yscale)
{

    p1.u        = 0;
    p1.v        = 0;
    p2.u        = rect.width();
    p2.v        = rect.height();

    convertPt2Rad(p1.u, p1.v);
    convertPt2Rad(p2.u, p2.v);

    my_xscale   = xscale * zoomFactor;
    my_yscale   = yscale * zoomFactor;

}

void CMapWms::draw(QPainter& p)
{
    if(pjsrc == 0) return IMap::draw(p);

    // render map if necessary
    if(needsRedraw)
    {
        draw();
    }

    p.drawPixmap(0,0,pixBuffer);

    // render overlay
    if(!ovlMap.isNull() && lastTileLoaded && !doFastDraw)
    {
        ovlMap->draw(size, needsRedrawOvl, p);
        needsRedrawOvl = false;
    }

    needsRedraw = false;

    if(CResources::self().showZoomLevel())
    {

        QString str;
        if(zoomFactor < 1.0)
        {
            str = tr("Overzoom x%1").arg(1/zoomFactor,0,'f',0);
        }
        else
        {
            str = tr("Zoom level x%1").arg(zoomFactor);
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

    p.setFont(QFont("Sans Serif",8,QFont::Black));
    if(copyright.isEmpty())
    {
        CCanvas::drawText(tr("%1 %2").arg(QChar(0x00A9)).arg(tr("Copyright notice is missing. Use <copyright> tag in <service> secton to supply a copyright notice.")), p, rect.bottomLeft() + QPoint(rect.width() / 2, -5) , QColor(Qt::darkBlue));
    }
    else
    {
        CCanvas::drawText(tr("%1 %2").arg(QChar(0x00A9)).arg(copyright), p, rect.bottomLeft() + QPoint(rect.width() / 2, -5) , QColor(Qt::darkBlue));
    }

}


void CMapWms::draw()
{
    QImage img;
    lastTileLoaded = false;

    pixBuffer.fill(Qt::white);
    QPainter p(&pixBuffer);

    double x1 = 0;
    double y1 = 0;
    convertPt2M(x1, y1);

    // convert to abs pixel in map
    x1 = (x1 - xref1) / (xscale);
    y1 = (y1 - yref1) / (yscale);

    // quantify to smalles multiple of blocksize
    x1 = floor(x1/(blockSizeX * zoomFactor)) * blockSizeX * zoomFactor;
    y1 = floor(y1/(blockSizeY * zoomFactor)) * blockSizeY * zoomFactor;

    // convert back to meter/rad
    x1 = x1*xscale + xref1;
    y1 = y1*yscale + yref1;

    // convert ref1 to point on screen
    double xx1 = x1;
    double yy1 = y1;
    convertM2Pt(xx1, yy1);

    int n = 0;
    int m = 0;

    double cx;
    double cy;

    do
    {
        do
        {
            double p1x = xx1 + n * blockSizeX;
            double p1y = yy1 + m * blockSizeY;
            double p2x = xx1 + (n + 1) * blockSizeX;
            double p2y = yy1 + (m + 1) * blockSizeY;

            cx = p2x;
            cy = p2y;

            convertPt2M(p1x, p1y);
            convertPt2M(p2x, p2y);

            QUrl url(urlstr);
            url.addQueryItem("request", "GetMap");
            url.addQueryItem("version", version);
            url.addQueryItem("layers", layers);
            url.addQueryItem("styles", "");
            url.addQueryItem("srs", srs);
            url.addQueryItem("format", format);
            url.addQueryItem("width", QString::number(blockSizeX));
            url.addQueryItem("height", QString::number(blockSizeY));

            if(pj_is_latlong(pjsrc))
            {
                url.addQueryItem("bbox", QString("%1,%2,%3,%4").arg(p1x*RAD_TO_DEG,0,'f').arg(p2y*RAD_TO_DEG,0,'f').arg(p2x*RAD_TO_DEG,0,'f').arg(p1y*RAD_TO_DEG,0,'f'));
            }
            else
            {
                url.addQueryItem("bbox", QString("%1,%2,%3,%4").arg(p1x,0,'f').arg(p2y,0,'f').arg(p2x,0,'f').arg(p1y,0,'f'));
            }

            request_t req;
            req.url         = url;
            req.lon         = p1x;
            req.lat         = p1y;
            req.zoomFactor  = zoomFactor;
            convertM2Rad(req.lon,req.lat);

            diskCache->restore(url.toString(), img);
            if(!img.isNull())
            {
                convertRad2Pt(req.lon,req.lat);
                p.drawImage(req.lon, req.lat,img);
            }
            else
            {
                addToQueue(req);
            }

            n++;
        }
        while(cx < rect.width());

        n = 0;
        m++;
    }
    while(cy < rect.height());

    checkQueue();
}

void CMapWms::addToQueue(request_t& req)
{
    newRequests.enqueue(req);
}

void CMapWms::checkQueue()
{
    if(newRequests.size() && pendRequests.size() < 6)
    {
        request_t req = newRequests.dequeue();

        if(diskCache->contains(req.url.toString()) || (req.zoomFactor != zoomFactor))
        {
            checkQueue();
            return;
        }

        QNetworkRequest request;
        request.setUrl(req.url);
        req.reply = accessManager->get(request);

        pendRequests[req.url.toString()] = req;
    }

    if(pendRequests.isEmpty() && newRequests.isEmpty())
    {
        status->setText(tr("Map loaded."));
        lastTileLoaded = true;
    }
    else
    {
        status->setText(tr("Wait for %1 tiles.").arg(pendRequests.size() + newRequests.size()));
    }


}

void CMapWms::slotRequestFinished(QNetworkReply* reply)
{
    QString _url_ = reply->url().toString();
    if(pendRequests.contains(_url_))
    {
        QImage img;
        QPainter p(&pixBuffer);

        request_t& req = pendRequests[_url_];

        // only take good responses
        if(!reply->error())
        {
            // read image data
            img.loadFromData(reply->readAll());
        }

        // always store image to cache, the cache will take care of NULL images
        diskCache->store(_url_, img);

        // only paint image if on current zoom factor
        if((req.zoomFactor == zoomFactor))
        {
            convertRad2Pt(req.lon, req.lat);
            p.drawImage(req.lon, req.lat, img);
        }

        // pending request finished
        pendRequests.remove(_url_);

    }

    // debug output any error
    if(reply->error())
    {
        qDebug() << reply->errorString();
    }

    // delete reply object
    reply->deleteLater();

    // check for more requests
    checkQueue();

    // the map did change
    emit sigChanged();
}

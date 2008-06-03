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
#include "CResources.h"
#include "CCanvas.h"
#include "CMapNoMap.h"
#include "CMapQMAP.h"
#include "CMainWindow.h"
#include "CCreateMapGeoTiff.h"

#include "CMouseMoveMap.h"
#include "CMouseSelMap.h"
#include "CMouseAddWpt.h"
#include "CMouseMoveWpt.h"
#include "CMouseEditWpt.h"
#include "CMouseRefPoint.h"
#include "CMouseCutTrack.h"

#include "CWpt.h"
#include "CTrack.h"
#include "CSearchDB.h"
#include "CWptDB.h"
#include "CMapDB.h"
#include "CTrackDB.h"
#include "CLiveLogDB.h"

#include "GeoMath.h"
#include "WptIcons.h"

#include <QtGui>

CCanvas::CCanvas(QWidget * parent)
: QWidget(parent)
, mouse(0)
, info(0)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    mouseMoveMap    = new CMouseMoveMap(this);
    mouseSelMap     = new CMouseSelMap(this);
    mouseAddWpt     = new CMouseAddWpt(this);
    mouseMoveWpt    = new CMouseMoveWpt(this);
    mouseEditWpt    = new CMouseEditWpt(this);
    mouseRefPoint   = new CMouseRefPoint(this);
    mouseCutTrack   = new CMouseCutTrack(this);
    setMouseMode(eMouseMoveArea);

}


CCanvas::~CCanvas()
{
}

void CCanvas::slotCopyPosition()
{
    IMap& map = CMapDB::self().getMap();

    double u = posMouse.x();
    double v = posMouse.y();
    map.convertPt2Rad(u,v);
    u = u * RAD_TO_DEG;
    v = v * RAD_TO_DEG;

    QString position;
    GPS_Math_Deg_To_Str(u, v, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);
}


void CCanvas::setMouseMode(mouse_mode_e mode)
{
    QApplication::restoreOverrideCursor();

    switch(mode) {

        case eMouseMoveArea:
            mouse = mouseMoveMap;
            break;

            //         case eMouseZoomArea:
            //             cursor = QCursor(cursorZoom,0,0);
            //             pfMousePressEvent   = &CCanvas::mousePressZoomArea;
            //             pfMouseMoveEvent    = &CCanvas::mouseMoveZoomArea;
            //             pfMouseReleaseEvent = &CCanvas::mouseReleaseZoomArea;
            //             break;
            //
        case eMouseAddWpt:
            mouse = mouseAddWpt;
            break;

        case eMouseEditWpt:
            mouse = mouseEditWpt;
            break;

        case eMouseMoveWpt:
            mouse = mouseMoveWpt;
            break;

        case eMouseMoveRefPoint:
            mouse = mouseRefPoint;
            break;

        case eMouseSelectArea:
            mouse = mouseSelMap;
            break;

        case eMouseCutTrack:
            mouse = mouseCutTrack;
            break;

        default:;

    }
    if(underMouse()) {
        QApplication::setOverrideCursor(*mouse);
    }
    mouseMode = mode;
    update();
}


void CCanvas::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);
    emit sigResize(e->size());
}


void CCanvas::paintEvent(QPaintEvent * e)
{
    QWidget::paintEvent(e);

    QPainter p;
    p.begin(this);
    p.fillRect(rect(),Qt::white);
//     p.setRenderHint(QPainter::Antialiasing,true);
    p.setFont(CResources::self().getMapFont());
    draw(p);

    p.end();
}


void CCanvas::mouseMoveEvent(QMouseEvent * e)
{
    posMouse = e->pos();

    mouseMoveEventCoord(e);
    mouse->mouseMoveEvent(e);
}


void CCanvas::mousePressEvent(QMouseEvent * e)
{
    posMouse = e->pos();
    mouse->mousePressEvent(e);
}


void CCanvas::mouseReleaseEvent(QMouseEvent * e)
{
    posMouse = e->pos();
    mouse->mouseReleaseEvent(e);
}


void CCanvas::enterEvent(QEvent * )
{
    QApplication::setOverrideCursor(*mouse);
}


void CCanvas::leaveEvent(QEvent * )
{
    QApplication::restoreOverrideCursor();
}

#define BORDER  20

void CCanvas::print(QPainter& p, const QSize& pagesize)
{
    bool  rotate = false;
    QSize _size_ = pagesize;
    qreal s = 0.0;

    p.save();

    if(pagesize.height() > pagesize.width()){
        _size_.setWidth(pagesize.height());
        _size_.setHeight(pagesize.width());
        rotate = true;
        p.rotate(90.0);
        p.translate(0,-pagesize.width());
    }

    qreal s1 = (qreal)(_size_.width() - 2 * BORDER) / (qreal)size().width();
    qreal s2 = (qreal)(_size_.height() - 2 * BORDER) / (qreal)size().height();

    s = (s1 > s2) ? s2 : s1;

    p.translate(BORDER,BORDER);
    p.scale(s,s);
    p.setClipRegion(rect());
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setFont(CResources::self().getMapFont());
    draw(p);
    p.restore();
}


void CCanvas::print(QPrinter& printer)
{
    QPainter p;

    p.begin(&printer);
    print(p, printer.pageRect().size());
    p.end();

}

void CCanvas::draw(QPainter& p)
{
//     QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    CMapDB::self().draw(p,rect());
    CTrackDB::self().draw(p, rect());
    CLiveLogDB::self().draw(p, rect());
    CWptDB::self().draw(p, rect());
    CSearchDB::self().draw(p, rect());
    drawRefPoints(p);
    drawScale(p);

    mouse->draw(p);
//     QApplication::restoreOverrideCursor();
}

void CCanvas::drawRefPoints(QPainter& p)
{
    CCreateMapGeoTiff * dlg = CCreateMapGeoTiff::self();
    if(dlg == 0) return;

    IMap& map = CMapDB::self().getMap();

    QMap<quint32,CCreateMapGeoTiff::refpt_t>& refpts         = dlg->getRefPoints();
    QMap<quint32,CCreateMapGeoTiff::refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end()){
        double x = refpt->x;
        double y = refpt->y;
        map.convertM2Pt(x,y);

        if(rect().contains(x,y)){
            p.drawPixmap(x - 15,y - 31,QPixmap(":/icons/iconRefPoint31x31"));
            drawText(refpt->item->text(CCreateMapGeoTiff::eLabel),p,QPoint(x, y - 35));
        }

        ++refpt;
    }
}

void CCanvas::drawScale(QPainter& p)
{

    IMap& map = CMapDB::self().getMap();
    QPoint px1(rect().bottomRight() - QPoint(100,50));

    // step I: get the approximate distance for 200px in the bottom right corner
    double u1 = px1.x();
    double v1 = px1.y();

    double u2 = px1.x() - 200;
    double v2 = v1;

    map.convertPt2M(u1,v1);
    map.convertPt2M(u2,v2);

    double d = u1 - u2;

    // step II: derive the actual scale length in [m]
    double a = (int)log10(d);
    double b = log10(d) - a;

//     qDebug() << log10(d) << d << a << b;

    if(0 <= b && b < log10(3.0f)){
        d = 1 * pow(10,a);
    }
    else if(log10(3.0f) < b && b < log10(5.0f)){
        d = 3 * pow(10,a);
    }
    else{
        d = 5 * pow(10,a);
    }

//     qDebug() << "----" << d;

    // step III: convert the scale length from [m] into [px]
    XY pt1, pt2;
    pt1.u = px1.x();
    pt1.v = px1.y();
    map.convertPt2Rad(pt1.u,pt1.v);


    if(pt1.u == px1.x() && pt1.v == px1.y()){
        return;
    }

    pt2 = GPS_Math_Wpt_Projection(pt1, d, -90 * DEG_TO_RAD);
    map.convertRad2Pt(pt2.u, pt2.v);

    // step IV: draw the scale
    QPoint px2(px1 - QPoint(px1.x() - pt2.u,0));

    p.setRenderHint(QPainter::Antialiasing,false);
    p.setPen(QPen(Qt::white, 9));
    p.drawLine(px1, px2 + QPoint(9,0));
    p.setPen(QPen(Qt::black, 7));
    p.drawLine(px1, px2 + QPoint(9,0));
    p.setPen(QPen(Qt::white, 5));
    p.drawLine(px1, px2 + QPoint(9,0));

    QVector<qreal> pattern;
    pattern << 2 << 4;
    QPen pen(Qt::black, 5, Qt::CustomDashLine);
    pen.setDashPattern(pattern);
    p.setPen(pen);
    p.drawLine(px1, px2 + QPoint(9,0));
    p.setRenderHint(QPainter::Antialiasing,true);

    QPoint px3(px2.x() + (px1.x() - px2.x())/2, px2.y());
    drawText(QString("%1 m").arg(d), p, px3, Qt::black);
}

void CCanvas::drawText(const QString& str, QPainter& p, const QPoint& center, const QColor& color)
{
    QFont           f = CResources::self().getMapFont();
    QFontMetrics    fm(f);
    QRect           r = fm.boundingRect(str);

    r.moveCenter(center);

    p.setPen(Qt::white);
    p.setFont(f);

    p.drawText(r.topLeft() - QPoint(-1,-1), str);
    p.drawText(r.topLeft() - QPoint( 0,-1), str);
    p.drawText(r.topLeft() - QPoint(+1,-1), str);

    p.drawText(r.topLeft() - QPoint(-1, 0), str);
    p.drawText(r.topLeft() - QPoint(+1, 0), str);

    p.drawText(r.topLeft() - QPoint(-1,+1), str);
    p.drawText(r.topLeft() - QPoint( 0,+1), str);
    p.drawText(r.topLeft() - QPoint(+1,+1), str);

    p.setPen(color);
    p.drawText(r.topLeft(),str);

}

void CCanvas::drawText(const QString& str, QPainter& p, const QRect& r, const QColor& color)
{

    p.setPen(Qt::white);
    p.setFont(CResources::self().getMapFont());

    p.drawText(r.adjusted(-1,-1,-1,-1),Qt::AlignCenter,str);
    p.drawText(r.adjusted( 0,-1, 0,-1),Qt::AlignCenter,str);
    p.drawText(r.adjusted(+1,-1,+1,-1),Qt::AlignCenter,str);

    p.drawText(r.adjusted(-1, 0,-1, 0),Qt::AlignCenter,str);
    p.drawText(r.adjusted(+1, 0,+1, 0),Qt::AlignCenter,str);

    p.drawText(r.adjusted(-1,+1,-1,+1),Qt::AlignCenter,str);
    p.drawText(r.adjusted( 0,+1, 0,+1),Qt::AlignCenter,str);
    p.drawText(r.adjusted(+1,+1,+1,+1),Qt::AlignCenter,str);

    p.setPen(color);
    p.drawText(r,Qt::AlignCenter,str);

}


void CCanvas::wheelEvent(QWheelEvent * e)
{
    zoom(CResources::self().flipMouseWheel() ? (e->delta() > 0) : (e->delta() < 0), e->pos());
}


void CCanvas::zoom(bool in, const QPoint& p)
{
    CMapDB::self().getMap().zoom(in, p);
    update();
}


void CCanvas::move(double lon, double lat)
{
    IMap& map = CMapDB::self().getMap();
    double u = lon * DEG_TO_RAD;
    double v = lat * DEG_TO_RAD;
    map.convertRad2Pt(u,v);
    map.move(QPoint(u,v), rect().center());
    update();
}


void CCanvas::move(move_direction_e dir)
{
    IMap& map = CMapDB::self().getMap();
    QPoint p1 = geometry().center();
    QPoint p2 = p1;

    switch(dir) {

        case eMoveLeft:
            p2.rx() += width() / 4;
            break;

        case eMoveRight:
            p2.rx() -= width() / 4;
            break;

        case eMoveUp:
            p2.ry() += height() / 4;
            break;

        case eMoveDown:
            p2.ry() -= height() / 4;
            break;

        case eMoveCenter:
        {
            double lon1 = 0, lat1 = 0, lon2 = 0, lat2 = 0;

            map.dimensions(lon1, lat1, lon2, lat2);

            lon1 += (lon2 - lon1)/2;
            lat2 += (lat1 - lat2)/2;
            map.convertRad2Pt(lon1,lat2);

            p1.rx() = lon1;
            p1.ry() = lat2;

            p2 = geometry().center();

        }
        break;
    }
    map.move(p1, p2);

    update();
}


void CCanvas::mouseMoveEventCoord(QMouseEvent * e)
{
    IMap& map = CMapDB::self().getMap();
    QString info;                // = QString("%1 %2, ").arg(e->x()).arg(e->y());

    double x = e->x();
    double y = e->y();

    map.convertPt2Rad(x,y);

    if((x == e->x()) && (y == e->y())){
        map.convertPt2M(x,y);
        info += QString(" (%1 %2)").arg(x,0,'f',0).arg(y,0,'f',0);
    }
    else {

        float ele = CMapDB::self().getDEM().getElevation(x,y);
        if(ele != WPT_NOFLOAT) {
            info += QString(" (ele: %1 m)").arg(ele,0,'f',0);
        }

        x *= RAD_TO_DEG;
        y *= RAD_TO_DEG;

        qint32 degN,degE;
        float minN,minE;

        GPS_Math_Deg_To_DegMin(y, &degN, &minN);

        GPS_Math_Deg_To_DegMin(x, &degE, &minE);

        QString str,lat,lng;
        lat = degN < 0 ? "S" : "N";
        lng = degE < 0 ? "W" : "E";
        str.sprintf(" %s%02d\260 %06.3f %s%03d\260 %06.3f ",lat.toUtf8().data(),abs(degN),minN,lng.toUtf8().data(),abs(degE),minE);

        info += str;
    }

    theMainWindow->setPositionInfo(info);

}

void CCanvas::raiseContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    menu.addAction(QIcon(":/icons/iconClipboard16x16.png"),tr("Copy Position"),this,SLOT(slotCopyPosition()));
    mouse->contextMenu(menu);

    QPoint p = mapToGlobal(pos);
    menu.exec(p);
}


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

#include "IGarminTyp.h"
#include <QtCore>

#include <stdio.h>
#include <string.h>

#define DBG

IGarminTyp::IGarminTyp(format_e format, QObject * parent)
: QObject(parent)
, format(format)
{

}

IGarminTyp::~IGarminTyp()
{

}

void IGarminTyp::decodeBitmap(QDataStream &in, QImage &img, int w, int h, int bpp)
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


bool IGarminTyp::parseHeader(QDataStream& in)
{
    int i;
    QString garmintyp;
    quint8 byte;

    for(i = 0; i < 10; ++i) {
        in >> byte;
        garmintyp.append(byte);
    }
    garmintyp.append(0);
    if(garmintyp != "GARMIN TYP") {
        qDebug() << "CMapTDB::readTYP() not a known typ file";
        return false;
    }

    /* reading typ creation date string */

    in.device()->seek(0x0c);
    in >> version >> year >> month >> day >> hour >> minutes >> seconds >> codepage;
    month -= 1;                  /* Month are like Microsoft starting 0 ? */
    year += 1900;

    /* Reading points / lines / polygons struct */
    in >> sectPoints.dataOffset >> sectPoints.dataLength;
    in >> sectPolylines.dataOffset >> sectPolylines.dataLength;
    in >> sectPolygons.dataOffset >> sectPolygons.dataLength;

    in >> pid >> fid;


    /* Read Array datas */
    in >> sectPoints.arrayOffset >> sectPoints.arrayModulo >> sectPoints.arraySize;
    in >> sectPolylines.arrayOffset  >> sectPolylines.arrayModulo  >> sectPolylines.arraySize;
    in >> sectPolygons.arrayOffset >> sectPolygons.arrayModulo >> sectPolygons.arraySize;
    in >> sectOrder.arrayOffset >> sectOrder.arrayModulo >> sectOrder.arraySize;

#ifdef DBG
    qDebug() << "Version:" << version << "Codepage:" << codepage;
    qDebug() << "PID" << hex << pid << "FID" << hex << fid;
    qDebug() << "Points     doff/dlen/aoff/amod/asize:" << hex << "\t" << sectPoints.dataOffset << "\t" << sectPoints.dataLength << "\t" << sectPoints.arrayOffset << "\t" << sectPoints.arrayModulo << "\t" << sectPoints.arrayOffset;
    qDebug() << "Polylines  doff/dlen/aoff/amod/asize:" << hex << "\t" << sectPolylines.dataOffset << "\t" << sectPolylines.dataLength << "\t" << sectPolylines.arrayOffset << "\t" << sectPolylines.arrayModulo << "\t" << sectPolylines.arrayOffset;
    qDebug() << "Polygons   doff/dlen/aoff/amod/asize:" << hex << "\t" << sectPolygons.dataOffset << "\t" << sectPolygons.dataLength << "\t" << sectPolygons.arrayOffset << "\t" << sectPolygons.arrayModulo << "\t" << sectPolygons.arrayOffset;
    qDebug() << "Order      doff/dlen/aoff/amod/asize:" << hex << "\t" << sectOrder.dataOffset << "\t" << sectOrder.dataLength << "\t" << sectOrder.arrayOffset << "\t" << sectOrder.arrayModulo << "\t" << sectOrder.arrayOffset;
#endif

    return true;
}

bool IGarminTyp::parseDrawOrder(QDataStream& in, QList<quint32>& drawOrder)
{
    if(sectOrder.arrayModulo != 5) {
        return false;
    }

    if((sectOrder.arraySize % sectOrder.arrayModulo) != 0) {
        return true;
    }

    in.device()->seek(sectOrder.arrayOffset);

    int i,n;
    quint8  typ;
    quint32 subtyp;

    int count=1;

    const int N = sectOrder.arraySize / sectOrder.arrayModulo;

    for (unsigned  i = 0; i < N; i++) {
        in >> typ >>  subtyp;
//         qDebug() << hex << typ << subtyp;
        if (typ == 0) {
            count++;
        }
        else if(subtyp == 0) {
#ifdef DBG
            qDebug() << QString("Type 0x%1 is priority %2").arg(typ,0,16).arg(count);
#endif
            int idx = drawOrder.indexOf(typ);
            if(idx != -1) {
                drawOrder.move(idx,0);
            }
        }
        else if(format == eNT){
            quint32 exttyp = 0x010000 | (typ << 8);
            quint32 mask = 0x1;

            for(n=0; n < 0x20; ++n){
                if(subtyp & mask){
                    drawOrder.push_front(exttyp|n);
#ifdef DBG
                    qDebug() << QString("Type 0x%1 is priority %2").arg(exttyp|n,0,16).arg(count);
#endif
                }
                mask = mask << 1;
            }

        }
        else if(format == eNorm){
            quint32 exttyp = 0x010000 | (typ << 8) | (subtyp >> 16);
            drawOrder.push_front(exttyp);
#ifdef DBG
            qDebug() << QString("Type 0x%1 is priority %2").arg(exttyp,0,16).arg(count);
#endif

        }
    }

#ifdef DBG
    for(unsigned i = 0; i < drawOrder.size(); ++i){
        if(i && i%16 == 0) printf(" \n");
        printf("%06X ", drawOrder[i]);
    }
    printf(" \n");
#endif

   return true;
}


bool IGarminTyp::parsePolygon(QDataStream& in, QMap<quint32, polygon_property>& polygons)
{
    bool tainted = false;

    if(!sectPolygons.arrayModulo || ((sectPolygons.arraySize % sectPolygons.arrayModulo) != 0)) {
        return true;
    }

    QTextCodec * codec = QTextCodec::codecForName(QString("CP%1").arg(codepage).toLatin1());

    const int N = sectPolygons.arraySize / sectPolygons.arrayModulo;
    for (int element = 0; element < N; element++) {
        quint16 t16_1, t16_2, subtyp;
        quint8  t8;
        quint32 typ, offset;
        bool hasLocalization = false;
        bool hasTextColor = false;
        quint8 ctyp;
        QImage xpmDay(32,32, QImage::Format_Indexed8);
        QImage xpmNight(32,32, QImage::Format_Indexed8);
        quint8 r,g,b;
        quint8 langcode;

        in.device()->seek( sectPolygons.arrayOffset + (sectPolygons.arrayModulo * element ) );

        if (sectPolygons.arrayModulo == 5) {
            in >> t16_1 >> t16_2 >> t8;
            offset = t16_2|(t8<<16);
        }
        else if (sectPolygons.arrayModulo == 4) {
            in >> t16_1 >> t16_2;
            offset = t16_2;
        }
        else if (sectPolygons.arrayModulo == 3) {
            in >> t16_1 >> t8;
            offset = t8;
        }

        t16_2   = (t16_1 >> 5) | (( t16_1 & 0x1f) << 11);
        typ     = t16_2 & 0x7F;
        subtyp  = t16_1 & 0x1F;

        if(t16_1 & 0x2000) {
            typ = 0x10000|(typ << 8)|subtyp;
        }

        in.device()->seek(sectPolygons.dataOffset + offset);
        in >> t8;
        hasLocalization = t8 & 0x10;
        hasTextColor    = t8 & 0x20;
        ctyp            = t8 & 0x0F;

#ifdef DBG
        qDebug() << "Polygon typ:" << hex << typ << "ctype:" << ctyp << "offset:" << (sectPolygons.dataOffset + offset) << "orig data:" << t16_1;
#endif

        polygon_property& property = polygons[typ];

        switch(ctyp){
            case 0x06:
            {
                // day & night single color
                in >> b >> g >> r;
                property.brushDay      = QBrush(qRgb(r,g,b));
                property.brushNight    = QBrush(qRgb(r,g,b));
                property.pen           = Qt::NoPen;
                property.known         = true;

                break;
            }
            case 0x07:
            {
                // day single color & night single color
                in >> b >> g >> r;
                property.brushDay      = QBrush(qRgb(r,g,b));
                in >> b >> g >> r;
                property.brushNight    = QBrush(qRgb(r,g,b));
                property.pen           = Qt::NoPen;
                property.known         = true;

                break;
            }
            case 0x08:
            {
                // day & night two color
                xpmDay.setNumColors(2);

                in >> b >> g >> r;
                xpmDay.setColor(1, qRgb(r,g,b) );
                in >> b >> g >> r;
                xpmDay.setColor(0, qRgb(r,g,b) );

                decodeBitmap(in, xpmDay, 32, 32, 1);
                property.brushDay.setTextureImage(xpmDay);
                property.brushNight.setTextureImage(xpmDay);
                property.pen      = Qt::NoPen;
                property.known    = true;
                break;
            }

            case 0x09:
            {
                //day two color & night two color
                xpmDay.setNumColors(2);
                xpmNight.setNumColors(2);

                in >> b >> g >> r;
                xpmDay.setColor(1, qRgb(r,g,b) );
                in >> b >> g >> r;
                xpmDay.setColor(0, qRgb(r,g,b) );
                in >> b >> g >> r;
                xpmNight.setColor(1, qRgb(r,g,b) );
                in >> b >> g >> r;
                xpmNight.setColor(0, qRgb(r,g,b) );

                decodeBitmap(in, xpmDay, 32, 32, 1);
                memcpy(xpmNight.bits(), xpmDay.bits(), (32*32));
                property.brushDay.setTextureImage(xpmDay);
                property.brushNight.setTextureImage(xpmNight);
                property.pen      = Qt::NoPen;
                property.known    = true;

                break;
            }
            case 0x0B:
            {
                // day one color, transparent & night two color
                xpmDay.setNumColors(2);
                xpmNight.setNumColors(2);
                in >> b >> g >> r;
                xpmDay.setColor(1, qRgb(r,g,b) );
                xpmDay.setColor(0, qRgba(255,255,255,0) );

                in >> b >> g >> r;
                xpmNight.setColor(1, qRgb(r,g,b) );
                in >> b >> g >> r;
                xpmNight.setColor(0, qRgb(r,g,b) );

                decodeBitmap(in, xpmDay, 32, 32, 1);
                memcpy(xpmNight.bits(), xpmDay.bits(), (32*32));
                property.brushDay.setTextureImage(xpmDay);
                property.brushNight.setTextureImage(xpmNight);
                property.pen      = Qt::NoPen;
                property.known    = true;
                break;
            }

            case 0x0D:
            {
                // day two color & night one color, transparent
                xpmDay.setNumColors(2);
                xpmNight.setNumColors(2);
                in >> b >> g >> r;
                xpmDay.setColor(1, qRgb(r,g,b) );
                in >> b >> g >> r;
                xpmDay.setColor(0, qRgb(r,g,b) );

                in >> b >> g >> r;
                xpmNight.setColor(1, qRgb(r,g,b) );
                xpmNight.setColor(0, qRgba(255,255,255,0) );

                decodeBitmap(in, xpmDay, 32, 32, 1);
                memcpy(xpmNight.bits(), xpmDay.bits(), (32*32));
                property.brushDay.setTextureImage(xpmDay);
                property.brushNight.setTextureImage(xpmNight);
                property.pen      = Qt::NoPen;
                property.known    = true;

                break;
            }
            case 0x0E:
            {
                // day & night one color, transparent
                xpmDay.setNumColors(2);
                in >> b >> g >> r;
                xpmDay.setColor(1, qRgb(r,g,b) );
                xpmDay.setColor(0, qRgba(255,255,255,0) );

                decodeBitmap(in, xpmDay, 32, 32, 1);
                property.brushDay.setTextureImage(xpmDay);
                property.brushNight.setTextureImage(xpmDay);
                property.pen      = Qt::NoPen;
                property.known    = true;

                break;
            }
            case 0x0F:
            {
                // day one color, transparent & night one color, transparent
                xpmDay.setNumColors(2);
                xpmNight.setNumColors(2);
                in >> b >> g >> r;
                xpmDay.setColor(1, qRgb(r,g,b) );
                xpmDay.setColor(0, qRgba(255,255,255,0) );

                in >> b >> g >> r;
                xpmNight.setColor(1, qRgb(r,g,b) );
                xpmNight.setColor(0, qRgba(255,255,255,0) );

                decodeBitmap(in, xpmDay, 32, 32, 1);
                memcpy(xpmNight.bits(), xpmDay.bits(), (32*32));
                property.brushDay.setTextureImage(xpmDay);
                property.brushNight.setTextureImage(xpmNight);
                property.pen      = Qt::NoPen;
                property.known    = true;

                break;
            }

            default:
                if(!tainted) {
                    QMessageBox::warning(0, tr("Warning..."), tr("This is a typ file with unknown polygon encoding. Please report!"), QMessageBox::Abort, QMessageBox::Abort);
                    tainted = true;
                }
                qDebug() << "Failed polygon:" << typ << subtyp << hex << typ << subtyp << ctyp;
        }

        if(hasLocalization){
            qint16 len;
            quint8 n = 1;

            in >> t8;
            len = t8;


            if(!(t8 & 0x01)){
                n = 2;
                in >> t8;
                len |= t8 << 8;
            }

            len -= n;
            while(len > 0){
                QByteArray str;
                in >> langcode;
                len -= 2*n;
                while(len > 0){

                    in >> t8;
                    len -= 2*n;

                    if(t8 == 0) break;

                    str += t8;

                }
                property.strings[langcode] = codec->toUnicode(str);
#ifdef DBG
                qDebug() << len << langcode << property.strings[langcode];
#endif
            }
        }

        if(hasTextColor){
            in >> t8;
            property.labelType = (label_type_e)(t8 & 0x07);

            if(t8 & 0x08){
                in >> r >> g >> b;
                property.colorLabelDay = qRgb(r,g,b);
            }

            if(t8 & 0x10){
                in >> r >> g >> b;
                property.colorLabelNight = qRgb(r,g,b);
            }
#ifdef DBG
            qDebug() << "ext. label: type" << property.labelType << "day" << property.colorLabelDay << "night" << property.colorLabelNight;
#endif
        }
    }

    return true;
}

bool IGarminTyp::parsePolyline(QDataStream& in, QMap<quint32, polyline_property>& polylines)
{
    bool tainted = false;

    if(!sectPolylines.arrayModulo || ((sectPolylines.arraySize % sectPolylines.arrayModulo) != 0)) {
        return true;
    }

    QTextCodec * codec = QTextCodec::codecForName(QString("CP%1").arg(codepage).toLatin1());

    const int N = sectPolylines.arraySize / sectPolylines.arrayModulo;
    for (int element = 0; element < N; element++) {
        quint16 t16_1, t16_2, subtyp;
        quint8  t8_1, t8_2;
        quint32 typ, offset;
        bool hasLocalization = false;
        bool hasTextColor = false;
        bool renderMode = false;
        quint8 ctyp, rows;
        quint8 r,g,b;
        quint8 langcode;

        in.device()->seek( sectPolylines.arrayOffset + (sectPolylines.arrayModulo * element ) );

        if (sectPolylines.arrayModulo == 5) {
            in >> t16_1 >> t16_2 >> t8_1;
            offset = t16_2|(t8_1<<16);
        }
        else if (sectPolylines.arrayModulo == 4) {
            in >> t16_1 >> t16_2;
            offset = t16_2;
        }
        else if (sectPolylines.arrayModulo == 3) {
            in >> t16_1 >> t8_1;
            offset = t8_1;
        }

        t16_2   = (t16_1 >> 5) | (( t16_1 & 0x1f) << 11);
        typ     = t16_2 & 0x7F;
        subtyp  = t16_1 & 0x1F;

        if(t16_1 & 0x2000) {
            typ = 0x10000|(typ << 8)|subtyp;
        }


        in.device()->seek(sectPolylines.dataOffset + offset);
        in >> t8_1 >> t8_2;
        ctyp = t8_1 & 0x07;
        rows = t8_1 >> 3;

        hasLocalization = t8_2 & 0x01;
        renderMode      = t8_2 & 0x02;
        hasTextColor    = t8_2 & 0x04;


#ifdef DBG
        qDebug() << "Polyline typ:" << hex << typ << "ctyp:" << ctyp << "offset:" << (sectPolylines.dataOffset + offset) << "orig data:" << t16_1;
#endif

        polyline_property& property = polylines[typ];
#ifdef DBG
        qDebug() << "rows" << rows << "t8_2" << hex << t8_2;
#endif

        switch(ctyp){
            case 0x00:
            {
                if(rows){
                    QImage xpm(32, rows, QImage::Format_Indexed8 );
                    in >> b >> g >> r;
                    xpm.setColor(1, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm.setColor(0, qRgb(r,g,b) );
                    decodeBitmap(in, xpm, 32, rows, 1);
                    property.imgDay     = xpm;
                    property.imgNight   = xpm;
                    property.hasPixmap  = true;
                    property.known      = true;
                }
                else{
                    quint8 w1, w2;
                    in >> b >> g >> r;
                    property.penLineDay     = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penLineNight   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penBorderDay   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderNight = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> w1 >> w2;
                    property.penLineDay.setWidth(w1);
                    property.penLineNight.setWidth(w1);
                    property.penBorderDay.setWidth(w2);
                    property.penBorderNight.setWidth(w2);
                    property.hasBorder  = w2 > w1;
                    property.hasPixmap  = false;
                    property.known      = true;
                }

                break;
            }
            case 0x01:
            {
                if(rows){
                    QImage xpm1(32, rows, QImage::Format_Indexed8 );
                    QImage xpm2(32, rows, QImage::Format_Indexed8 );
                    in >> b >> g >> r;
                    xpm1.setColor(1, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm1.setColor(0, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm2.setColor(1, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm2.setColor(0, qRgb(r,g,b) );
                    decodeBitmap(in, xpm1, 32, rows, 1);
                    memcpy(xpm2.bits(), xpm1.bits(), (32*rows));
                    property.imgDay     = xpm1;
                    property.imgNight   = xpm2;
                    property.hasPixmap  = true;
                    property.known      = true;
                }
                else{
                    quint8 w1, w2;
                    in >> b >> g >> r;
                    property.penLineDay     = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penBorderDay   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penLineNight   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penBorderNight = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> w1 >> w2;
                    property.penLineDay.setWidth(w1);
                    property.penLineNight.setWidth(w1);
                    property.penBorderDay.setWidth(w2);
                    property.penBorderNight.setWidth(w2);
                    property.hasBorder  = w2 > w1;
                    property.hasPixmap  = false;
                    property.known      = true;
                }
                break;
            }
            case 0x03:
            {
                if(rows){
                    QImage xpm1(32, rows, QImage::Format_Indexed8 );
                    QImage xpm2(32, rows, QImage::Format_Indexed8 );
                    in >> b >> g >> r;
                    xpm1.setColor(1, qRgb(r,g,b) );
                    xpm1.setColor(0, qRgba(255,255,255,0) );
                    in >> b >> g >> r;
                    xpm2.setColor(1, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm2.setColor(0, qRgb(r,g,b) );
                    decodeBitmap(in, xpm1, 32, rows, 1);
                    memcpy(xpm2.bits(), xpm1.bits(), (32*rows));
                    property.imgDay     = xpm1;
                    property.imgNight   = xpm2;
                    property.hasPixmap  = true;
                    property.known      = true;
                }
                else{
                    quint8 w1, w2;
                    in >> b >> g >> r;
                    property.penLineDay     = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderDay   = QPen(Qt::NoPen);
                    in >> b >> g >> r;
                    property.penLineNight   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penBorderNight = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> w1 >> w2;
                    property.penLineDay.setWidth(w1);
                    property.penLineNight.setWidth(w1);
                    property.penBorderDay.setWidth(w2);
                    property.penBorderNight.setWidth(w2);
                    property.hasBorder  = w2 > w1;
                    property.hasPixmap  = false;
                    property.known      = true;
                }

                break;
            }
            case 0x05:
            {
                if(rows){
                    QImage xpm1(32, rows, QImage::Format_Indexed8 );
                    QImage xpm2(32, rows, QImage::Format_Indexed8 );
                    in >> b >> g >> r;
                    xpm1.setColor(1, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm1.setColor(0, qRgb(r,g,b) );
                    in >> b >> g >> r;
                    xpm2.setColor(1, qRgb(r,g,b) );
                    xpm2.setColor(0, qRgba(255,255,255,0) );
                    decodeBitmap(in, xpm1, 32, rows, 1);
                    memcpy(xpm2.bits(), xpm1.bits(), (32*rows));
                    property.imgDay     = xpm1;
                    property.imgNight   = xpm2;
                    property.hasPixmap  = true;
                    property.known      = true;
                }
                else{
                    quint8 w1;
                    in >> b >> g >> r;
                    property.penLineDay     = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penBorderDay   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    in >> b >> g >> r;
                    property.penLineNight   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderNight = QPen(Qt::NoPen);
                    in >> w1;
                    property.penLineDay.setWidth(w1);
                    property.penLineNight.setWidth(w1);
                    property.hasBorder  = false;
                    property.hasPixmap  = false;
                    property.known      = true;
                }
                break;
            }
            case 0x06:
            {
                if(rows){
                    QImage xpm(32, rows, QImage::Format_Indexed8 );
                    in >> b >> g >> r;
                    xpm.setColor(1, qRgb(r,g,b) );
                    xpm.setColor(0, qRgba(255,255,255,0) );
                    decodeBitmap(in, xpm, 32, rows, 1);
                    property.imgDay     = xpm;
                    property.imgNight   = xpm;
                    property.hasPixmap  = true;
                    property.known      = true;
                }
                else{
                    quint8 w1;
                    in >> b >> g >> r;
                    property.penLineDay     = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderDay   = QPen(Qt::NoPen);
                    property.penLineNight   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderNight = QPen(Qt::NoPen);
                    in >> w1;
                    property.penLineDay.setWidth(w1);
                    property.penLineNight.setWidth(w1);
                    property.hasBorder  = false;
                    property.hasPixmap  = false;
                    property.known      = true;
                }
                break;
            }
            case 0x07:
            {
                if(rows){
                    QImage xpm1(32, rows, QImage::Format_Indexed8 );
                    QImage xpm2(32, rows, QImage::Format_Indexed8 );
                    in >> b >> g >> r;
                    xpm1.setColor(1, qRgb(r,g,b) );
                    xpm1.setColor(0, qRgba(255,255,255,0) );
                    in >> b >> g >> r;
                    xpm2.setColor(1, qRgb(r,g,b) );
                    xpm2.setColor(0, qRgba(255,255,255,0) );
                    decodeBitmap(in, xpm1, 32, rows, 1);
                    memcpy(xpm2.bits(), xpm1.bits(), (32*rows));
                    property.imgDay     = xpm1;
                    property.imgNight   = xpm2;
                    property.hasPixmap  = true;
                    property.known      = true;
                }
                else{
                    quint8 w1;
                    in >> b >> g >> r;
                    property.penLineDay     = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderDay   = QPen(Qt::NoPen);
                    in >> b >> g >> r;
                    property.penLineNight   = QPen(QBrush(qRgb(r,g,b)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    property.penBorderNight = QPen(Qt::NoPen);
                    in >> w1;
                    property.penLineDay.setWidth(w1);
                    property.penLineNight.setWidth(w1);
                    property.hasBorder  = false;
                    property.hasPixmap  = false;
                    property.known      = true;
                }
                break;
            }

            default:
                if(!tainted) {
                    QMessageBox::warning(0, tr("Warning..."), tr("This is a typ file with unknown polyline encoding. Please report!"), QMessageBox::Abort, QMessageBox::Abort);
                    tainted = true;
                }

                qDebug() << "Failed polyline" <<  hex << ":" << typ <<  ctyp << rows ;
                continue;
        }
        if(hasLocalization){
            qint16 len;
            quint8 n = 1;

            in >> t8_1;
            len = t8_1;


            if(!(t8_1 & 0x01)){
                n = 2;
                in >> t8_1;
                len |= t8_1 << 8;
            }

            len -= n;
            while(len > 0){
                QByteArray str;
                in >> langcode;
                len -= 2*n;
                while(len > 0){

                    in >> t8_1;
                    len -= 2*n;

                    if(t8_1 == 0) break;

                    str += t8_1;

                }
                property.strings[langcode] = codec->toUnicode(str);
#ifdef DBG
                qDebug() << len << langcode << property.strings[langcode];
#endif
            }
        }

        if(hasTextColor){
            in >> t8_1;
            property.labelType = (label_type_e)(t8_1 & 0x07);

            if(t8_1 & 0x08){
                in >> r >> g >> b;
                property.colorLabelDay = qRgb(r,g,b);
            }

            if(t8_1 & 0x10){
                in >> r >> g >> b;
                property.colorLabelNight = qRgb(r,g,b);
            }
#ifdef DBG
            qDebug() << "ext. label: type" << property.labelType << "day" << property.colorLabelDay << "night" << property.colorLabelNight;
#endif
        }
    }
    return true;
}

bool IGarminTyp::parsePoint(QDataStream& in, QMap<quint32, QImage>& points)
{
    bool tainted = false;

    if(!sectPoints.arrayModulo || ((sectPoints.arraySize % sectPoints.arrayModulo) != 0)) {
        return true;
    }

    const int N = sectPoints.arraySize / sectPoints.arrayModulo;
    for (int element=0; element < N; element++) {
        quint16 t16_1, t16_2, subtyp;
        quint8  t8_1, t8_2;
        quint32 typ, offset;
        bool hasLocalization = false;
        bool hasTextColor = false;
//         quint8 ctyp, rows;
//         quint8 r,g,b;
        quint8 langcode;

        in.device()->seek( sectPoints.arrayOffset + (sectPoints.arrayModulo * element ) );

        if (sectPoints.arrayModulo == 5) {
            in >> t16_1 >> t16_2 >> t8_1;
            offset = t16_2|(t8_1<<16);
        }
        else if (sectPoints.arrayModulo == 4) {
            in >> t16_1 >> t16_2;
            offset = t16_2;
        }
        else if (sectPoints.arrayModulo == 3) {
            in >> t16_1 >> t8_1;
            offset = t8_1;
        }

        t16_2   = (t16_1 >> 5) | (( t16_1 & 0x1f) << 11);
        typ     = t16_2 & 0x7FF;
        subtyp  = t16_1 & 0x01F;

        if(t16_1 & 0x2000) {
            typ = 0x10000|(typ << 8)|subtyp;
        }
        else {
            typ = (typ << 8) + subtyp;
        }

        in.device()->seek( sectPoints.dataOffset + offset );

        quint8  w, h, colors, ctyp;
        in >> t8_1 >> w >> h >> colors >> ctyp;

        hasLocalization = t8_1 & 0x04;
        hasTextColor    = t8_1 & 0x08;
        t8_1            = t8_1 & 0x03;

#ifdef DBG
        qDebug() << "Point typ:" << hex << typ << "ctyp:" << ctyp << dec << "w" << w << "h" << h << "colors" << colors << hex <<"offset:" << (sectPoints.dataOffset + offset) << "orig data:" << t16_1;
#endif

    }

    return true;
}

// sub decode {
//     my ($self, $content_data) = @_;
//
//     $self->{l18n} = undef;
//     $self->{unknown_flags} = undef;
//     $self->{width} = undef;
//     $self->{height} = undef;
//     $self->{morecolors_info} = undef;
//
//
//     $self->{colors} = undef;
//     $self->{bitmap} = undef;
//     $self->{colors2} = undef;
//     $self->{bitmap2} = undef;
//     $self->{morecolors} = undef;
//
//     $self->{strings} = undef;
//
//     my $tmp = $content_data;
//     if (length($tmp) == 0) {
//
//         return;
//     };
//
//     my ($a, $w, $h, $colors, $x3) = unpack("CCCCC", substr($tmp, 0, 5, ''));
//
//     $self->{l18n} = ($a & 0x04) ? 1 : 0;
//     $a &= ~0x04;
//     $self->{unknown_flags} = ($a & 0xF8);
//     $a &= ~0xF8;
//
//     $self->{width} = $w;
//     $self->height($h);
//     $self->{x3} = $x3;
//
//     my ($bpp, $w_bytes) = $self->bpp_and_width_in_bytes($colors, $w, $x3);
//
//     $self->{bpp} = $bpp;
//
//     if ($x3 == 0x20) {
//
//         if (($colors == 0) && ($bpp >= 16)) {
//             $colors = $w*$h;
//         };
//         $self->{colors} = [$self->get_rgb_triplets(\$tmp, $colors, 1)];
//     } elsif ($x3 == 0x10) {
//
//         $self->{colors} = [$self->get_rgb_triplets(\$tmp, $colors)];
//     } elsif ($x3 == 0) {
//         if (($colors == 0) && ($bpp >= 16)) {
//             $colors = $w*$h;
//         };
//         $self->{colors} = [$self->get_rgb_triplets(\$tmp, $colors)];
//         if ($a == 0) {
//
//         };
//     } else {
//         die "unknown x3: $x3 (a=$a)";
//     };
//
//     my $bitmapa;
//     if ($bpp >= 16) {
//
//         $bitmapa = '';
//         for (my $i=0; $i<$colors; $i++) {
//             $bitmapa .= pack('S', $i);
//         };
//     } else {
//         my $x_zbyva = length($tmp);
//         my $x_mam_nacist = $h*$w_bytes;
//
//         if (($x_zbyva-$x_mam_nacist) < 0) {
//
//         };
//         $bitmapa = substr($tmp, 0, $x_mam_nacist, '');
//     };
//     $self->bitmap($bitmapa);
//
//
//
//     if ($a == 0x01) {
//
//     } elsif ($a == 0x00) {
//
//     } elsif ($a == 0x03) {
//
//         my ($colors2) = unpack("C", (substr($tmp, 0, 1, '')));
//         my ($x3b) = unpack("C", (substr($tmp, 0, 1, '')));
//
//         $self->{x3b} = $x3b;
//
//         $self->{colors2} = [$self->get_rgb_triplets(\$tmp, $colors2, ($x3b == 0x20))];
//         my ($bpp2, $w_bytes2) = $self->bpp_and_width_in_bytes($colors2, $w, $x3b);
//         $self->{bpp2} = $bpp2;
//         my $x_zbyva = length($tmp);
//         my $x_mam_nacist = $h*$w_bytes2;
//         if (($x_zbyva-$x_mam_nacist) < 0) {
//
//         };
//
//         my $bitmapa2 = substr($tmp, 0, $x_mam_nacist, '');
//         $self->{bitmap2} = $bitmapa2;
//
//     } elsif ($a == 0x02) {
//
//         my ($colors2) = unpack("C", (substr($tmp, 0, 1, '')));
//         my ($x3b) = unpack("C", (substr($tmp, 0, 1, '')));
//
//         $self->{x3b} = $x3b;
//
//         $self->{colors2} = [$self->get_rgb_triplets(\$tmp, $colors2)];
//         my ($bpp2, $w_bytes2) = $self->bpp_and_width_in_bytes($colors2, $w, $x3b);
//         $self->{bpp2} = $bpp2;
//         $self->{bitmap2} = undef;
//
//     } else {
//         die $self->id . "neznamy priznak a=$a, zbyva:\n".$self->smart_dump($tmp);
//     };
//
//     if (length($tmp) > 0) {
//
//     };
//     $self->{strings} = [$self->dekoduj_stringy(\$tmp)] if $self->{l18n};
//
//     if ($self->{unknown_flags} & 0x08) {
//         $self->{unknown_flags} &= ~0x08;
//         $self->{morecolors_info} = unpack('C', substr($tmp, 0, 1, ''));
//         $self->{morecolors} = $self->get_morecolors(\$tmp, $self->{morecolors_info});
//     };
//
//     $self->{_neznamy_konec} = $tmp;
// };






// /*sub bpp_and_width_in_bytes {
//     my ($self, $numcolors, $w_pixels, $x3_flag) = @_;
//
//     my $bpp = undef;
//     if ($x3_flag == 0x00) {
//
//         $bpp = {
//
//             0    => 16,    # 16 znamena ze se nepouziva paleta, ale primo barvy
//
//             1    => 1,
//             2    => 2,
//             3    => 2,
//             4    => 4,
//             5    => 4,
//
//             (map {$_ => 4} (6 .. 15)),
//             (map {$_ => 8} (16 .. 31)),
//
//             32    => 8,
//
//             (map {$_ => 8} (33 .. 255))
//         }->{$numcolors};
//     } elsif ($x3_flag == 0x10) {
//
//         $bpp = {
//
//             0    => 1,    # dulezite, i takove bitmapy (h=0, w=0, bpp=0, bez bitmapy) se tu vyskytuji
//             (map {$_ => 2} (1 .. 2)),
//
//             3    => 4,
//             4    => 4,
//             5    => 4,
//             6    => 4,
//             7    => 4,
//             8    => 4,
//             9    => 4,
//             10    => 4,
//             11    => 4,
//             12    => 4,
//             13    => 4,
//             14    => 4,
//
//             15    => 8,
//             16    => 8,
//             17    => 8,
//
//             (map {$_ => 8} (18 .. 255))
//         }->{$numcolors};
//     } elsif ($x3_flag == 0x20) {
//
//         $bpp = {
//             0    => 16,    # 16 znamena ze se nepouziva paleta, ale primo barvy
//             1    => 1,
//             (map {$_ => 2} (2 .. 3)),
//             (map {$_ => 4} (4 .. 15)),
//             (map {$_ => 8} (16 .. 255))
//         }->{$numcolors};
//     } else {
//         die {message => "unknown image flag: $x3_flag"};
//     };
//     if (!defined $bpp) {
//         die $self->id . "unknown bpp: flag=".sprintf('0x%02x', $x3_flag).", colors=$numcolors";
//
//     };
//
//     my $w_pixels_bytes = ($w_pixels * $bpp) / 8;
//     if ($w_pixels_bytes > int($w_pixels_bytes)) {
//
//         $w_pixels_bytes = int($w_pixels_bytes) + 1;
//     };
//     if ($bpp < 0) {
//         $w_pixels_bytes = 0;
//     };
//
//     return ($bpp, $w_pixels_bytes);
// };
// */

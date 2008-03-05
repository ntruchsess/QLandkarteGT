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

#include "GeoMath.h"
#include <stdlib.h>
#include <QtGui>

#if WIN32
#include <float.h>
#define isnan _isnan
#define FP_NAN NAN
#endif

//Copyright (C) 1999 Alan Bleasby
void GPS_Math_Deg_To_DegMin(float v, qint32 *d, float *m)
{
    qint32 sign;

    if(v<(float)0.) {
        v *= (float)-1.;
        sign = 1;
    }
    else {
        sign = 0;
    }

    *d = (qint32)v;
    *m = (v-(float)*d) * (float)60.0;
    if(*m>(float)59.999) {
        ++*d;
        *m = (float)0.0;
    }

    if(sign) {
        *d = -*d;
    }

    return;
}


void GPS_Math_DegMin_To_Deg(const qint32 d, const float m, float& deg)
{

    deg = ((float)abs(d)) + m / (float)60.0;
    if(d<0) {
        deg = -deg;
    }

    return;
}


bool GPS_Math_Str_To_Deg(const QString& str, float& lon, float& lat)
{
    QRegExp re("^\\s*([N|S]){1}\\W*([0-9]+)\\W*([0-9]+\\.[0-9]+)\\W([E|W]){1}\\W*([0-9]+)\\W*([0-9]+\\.[0-9]+)\\s*$");
    if(!re.exactMatch(str)) {
        QMessageBox::warning(0,QObject::tr("Error"),QObject::tr("Bad position format. Must be: [N|S] ddd mm.sss [W|E] ddd mm.sss"),QMessageBox::Ok,QMessageBox::NoButton);
        return false;
    }

    bool signLat    = re.cap(1) == "S";
    int degLat      = re.cap(2).toInt();
    float minLat   = re.cap(3).toDouble();

    GPS_Math_DegMin_To_Deg((signLat ? -degLat : degLat), minLat, lat);

    bool signLon    = re.cap(4) == "W";
    int degLon      = re.cap(5).toInt();
    float minLon   = re.cap(6).toDouble();

    GPS_Math_DegMin_To_Deg((signLon ? -degLon : degLon), minLon, lon);

    return true;
}


/// calc if 2 line segments intersect
/**
    The 1st line is defined by (x11,y11) - (x12,y12)
    The 2nd line is defined by (x21,y21) - (x22,y22)

*/
bool testLineSegForIntersect(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22)
{
    /*
            float denom = ((other_line.end_.y_ - other_line.begin_.y_)*(end_.x_ - begin_.x_)) -
                          ((other_line.end_.x_ - other_line.begin_.x_)*(end_.y_ - begin_.y_));

            float nume_a = ((other_line.end_.x_ - other_line.begin_.x_)*(begin_.y_ - other_line.begin_.y_)) -
                           ((other_line.end_.y_ - other_line.begin_.y_)*(begin_.x_ - other_line.begin_.x_));

            float nume_b = ((end_.x_ - begin_.x_)*(begin_.y_ - other_line.begin_.y_)) -
                           ((end_.y_ - begin_.y_)*(begin_.x_ - other_line.begin_.x_));
    */
    float denom  = ((y22 - y21) * (x12 - x11)) - ((x22 - x21) * (y12 - y11));
    float nume_a = ((x22 - x21) * (y11 - y21)) - ((y22 - y21) * (x11 - x21));
    float nume_b = ((x12 - x11) * (y11 - y21)) - ((y12 - y11) * (x11 - x21));

    if(denom == 0.0f) return false;

    float ua = nume_a / denom;
    float ub = nume_b / denom;

    if(ua >= 0.0f && ua <= 1.0f && ub >= 0.0f && ub <= 1.0f) {
        return true;
    }
    return false;
}


bool testPointInPolygon(const XY& pt, const QVector<XY>& poly1)
{

    bool    c = false;
    int     npol;
    int     i = 0, j = 0;
    XY      p1, p2;              // the two points of the polyline close to pt
    float  x = pt.u;
    float  y = pt.v;

    npol = poly1.count();
    if(npol > 2) {

        // see http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/
        for (i = 0, j = npol-1; i < npol; j = i++) {
            p1 = poly1[j];
            p2 = poly1[i];

            if ((((p2.v <= y) && (y < p1.v))  || ((p1.v <= y) && (y < p2.v))) &&
            (x < (p1.u - p2.u) * (y - p2.v) / (p1.v - p2.v) + p2.u)) {
                c = !c;
            }
        }
    }
    return c;
}


bool testPolygonsForIntersect(const QVector<XY>& poly1, const QVector<XY>& poly2)
{

    int n;
    int npol1 = poly1.count();
    int npol2 = poly2.count();

    if(npol1 < 2 || npol2 < 2) return false;

    // test if points of poly1 are within poly2.
    for(n = 0; n < npol1; ++n) {
        if(testPointInPolygon(poly1[n],poly2)) return true;
    }

    // test if lines of poly1 intersect with lines from poly2
    int i1 = 0, j1 = 0;
    int i2 = 0, j2 = 0;

    XY  p1, p2, p3, p4;

    for (i1 = 0, j1 = npol1-1; i1 < npol1; j1 = i1++) {
        p1 = poly1[j1];
        p2 = poly1[i1];
        for (i2 = 0, j2 = npol2-1; i2 < npol2; j2 = i2++) {
            p3 = poly2[j2];
            p4 = poly2[i2];
            if(testLineSegForIntersect(p1.u,p1.v,p2.u,p2.v,p3.u,p3.v,p4.u,p4.v)) {
                return true;
            }
        }
    }
    return false;
}


// from http://www.movable-type.co.uk/scripts/LatLongVincenty.html
double distance(const XY& p1, const XY& p2, double& a1, double& a2)
{
    double cosSigma = 0.0;
    double sigma = 0.0;
    double sinAlpha = 0.0;
    double cosSqAlpha = 0.0;
    double cos2SigmaM = 0.0;
    double sinSigma = 0.0;
    double sinLambda = 0.0;
    double cosLambda = 0.0;

                                 // WGS-84 ellipsiod
    double a = 6378137.0f, b = 6356752.3142f,  f = 1.0f/298.257223563f;
    double L = p2.u - p1.u;

    double U1 = atan((1-f) * tan(p1.v));
    double U2 = atan((1-f) * tan(p2.v));
    double sinU1 = sin(U1), cosU1 = cos(U1);
    double sinU2 = sin(U2), cosU2 = cos(U2);
    double lambda = L, lambdaP = (double)(2*PI);
    unsigned iterLimit = 20;

    while ((fabs(lambda - lambdaP) > 1e-12) && (--iterLimit > 0)) {
        sinLambda = sin(lambda);
        cosLambda = cos(lambda);
        sinSigma = sqrt((cosU2*sinLambda) * (cosU2*sinLambda) + (cosU1*sinU2-sinU1*cosU2*cosLambda) * (cosU1*sinU2-sinU1*cosU2*cosLambda));

        if (sinSigma==0) {
            return 0;            // co-incident points
        }

        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
        sigma = atan2(sinSigma, cosSigma);
        sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
        cosSqAlpha = 1 - sinAlpha * sinAlpha;
        cos2SigmaM = cosSigma - 2 * sinU1 * sinU2 / cosSqAlpha;

        if (isnan(cos2SigmaM)) {
            cos2SigmaM = 0;      // equatorial line: cosSqAlpha=0 (6)
        }

        double C = f/16 * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));
        lambdaP = lambda;
        lambda = L + (1-C) * f * sinAlpha * (sigma + C*sinSigma*(cos2SigmaM + C * cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM)));
    }
    if (iterLimit==0) return 0;  // formula failed to converge

    double uSq = cosSqAlpha * (a*a - b*b) / (b*b);
    double A = 1 + uSq/16384*(4096+uSq*(-768+uSq*(320-175*uSq)));
    double B = uSq/1024 * (256+uSq*(-128+uSq*(74-47*uSq)));
    double deltaSigma = B*sinSigma*(cos2SigmaM+B/4*(cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)-B/6*cos2SigmaM*(-3+4*sinSigma*sinSigma)*(-3+4*cos2SigmaM*cos2SigmaM)));
    double s = b*A*(sigma-deltaSigma);

    a1 = atan2(cosU2 * sinLambda, cosU1 * sinU2 - sinU1 * cosU2 * cosLambda) * 360 / TWOPI;
    a2 = atan2(cosU1 * sinLambda, -sinU1 * cosU2 + cosU1 * sinU2 * cosLambda) * 360 / TWOPI;
    return s;
}


void GPS_Math_Deg_To_Str(const float& x, const float& y, QString& str)
{
    qint32 degN,degE;
    float minN,minE;

    GPS_Math_Deg_To_DegMin(y, &degN, &minN);

    GPS_Math_Deg_To_DegMin(x, &degE, &minE);

    QString lat,lng;
    lat = degN < 0 ? "S" : "N";
    lng = degE < 0 ? "W" : "E";
    str.sprintf(" %s%02d\260 %06.3f %s%03d\260 %06.3f ",lat.toUtf8().data(),abs(degN),minN,lng.toUtf8().data(),abs(degE),minE);
}

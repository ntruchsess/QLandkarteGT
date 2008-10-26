/**********************************************************************************************
    Copyright (C) 2006, 2007 Oliver Eichler oliver.eichler@gmx.de

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

  Garmin and MapSource are registered trademarks or trademarks of Garmin Ltd.
  or one of its subsidiaries.

  This source is based on John Mechalas documentation "Garmin IMG File Format" found
  at sourceforge. The missing bits and error were rectified by the source code of
  Konstantin Galichsky (kg@geopainting.com), http://www.geopainting.com

**********************************************************************************************/
#include "Platform.h"
#include "CGarminPolygon.h"
#include "Garmin.h"

#undef DEBUG_SHOW_POLY_DATA
#undef DEBUG_SHOW_POLY_PTS

struct sign_info_t
{
    sign_info_t();

    quint32 sign_info_bits;
    bool x_has_sign;
    bool nx;
    bool y_has_sign;
    bool ny;

};
sign_info_t::sign_info_t()
: sign_info_bits(2)
, x_has_sign(true)
, nx(false)
, y_has_sign(true)
, ny(false)
{
}


quint32 CGarminPolygon::cnt = 0;

CGarminPolygon::CGarminPolygon()
: type(0)
, direction(false)
, lbl_info(0)
, lbl_in_NET(false)
, dLng(0)
, dLat(0)
, id(0)

{

}


CGarminPolygon::~CGarminPolygon()
{

}


quint32 CGarminPolygon::decode(qint32 iCenterLon, qint32 iCenterLat, quint32 shift, bool line, const quint8 * pData)
{
    quint32 bytes_total = 10;
    // bitstream has a two byte length
    bool two_byte_len;
    // coordinates use extra bit - ??? have never seen it
    bool extra_bit;
    // bitstream length
    quint16 bs_len = 0;
    // base bit size info for coordinates
    quint8 bs_info;
    // bits per x coord.
    quint32 bx;
    // bits per y coord.
    quint32 by;

    /* poly_type

        for polylines:
        bit 0..5    type
        bit 6       direction
        for polygons:
        bit 0..6    type

        bit 7       bitstream_len is two bytes (true)
    */
    type = *pData++;
    two_byte_len = type & 0x80;
    if(line) {
        direction = (type & 0x40);
        type &= 0x3F;
    }
    else {
        type &= 0x7F;
    }

    /* label info
        bit 0..21   off set into LBL section
        bit 22      use extra bit for coordinates
        bit 23      use label data of NET section
    */
    lbl_info    = gar_ptr_load(uint24_t, pData);
    lbl_in_NET  = lbl_info & 0x800000;
    extra_bit   = lbl_info & 0x400000;
    lbl_info    = lbl_info & 0x3FFFFF;

    pData += 3;

//     qDebug() << hex << lbl_in_NET << extra_bit << lbl_info;

    // delta longitude and latitude
    dLng = gar_ptr_load(uint16_t, pData); pData += 2;
    dLat = gar_ptr_load(uint16_t, pData); pData += 2;

    // bitstream length
    if(two_byte_len) {
        bs_len = gar_ptr_load(uint16_t, pData); pData += 2;
        bytes_total += bs_len + 1;
    }
    else {
#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
        (quint8&)bs_len = *pData++;
#else
        bs_len = *pData++;
#endif
        bytes_total += bs_len;
    }

    /* bitstream info
        bit 0..3    base bits longitude
        bit 4..7    base bits latitude
    */
    bs_info = *pData++;;

    //if(extra_bit) qWarning("extrabit");

#ifdef DEBUG_SHOW_POLY_DATA
    qDebug() << "type:      " << type;
    qDebug() << "two byte:  " << two_byte_len;
    qDebug() << "extra bit: " << extra_bit;
    qDebug() << "dLng:      " << dLng;
    qDebug() << "dLat:      " << dLat;
    qDebug() << "len:       " << bs_len;
    qDebug() << "info:      " << hex << bs_info;
    qDebug() << "1st byte:  " << hex << *pData;
    qDebug() << "bytes total" << bytes_total;
#endif                       // DEBUG_SHOW_POLY_DATA

    sign_info_t signinfo;
    bits_per_coord(bs_info,*pData,bx,by,signinfo);

    CShiftReg sr(pData,bs_len,bx,by,extra_bit,signinfo);
    qint32 x1,y1,x = 0,y = 0;
    XY xy;

    bool isNegative = (iCenterLon >= 0x800000);
    // first point
    x1 = ((qint32)dLng << shift) + iCenterLon;
    y1 = ((qint32)dLat << shift) + iCenterLat;

    if(x1 >= 0x800000 && !isNegative) x1 = 0x7fffff;

    xy.u = GARMIN_RAD(x1);
    xy.v = GARMIN_RAD(y1);
//     xy = pj_fwd(xy,*gpProj);
#ifdef DEBUG_SHOW_POLY_PTS
    qDebug() << xy.u << xy.v << hex << x1 << y1 << DEG(x1) << DEG(y1);
#endif
//     points.append(xy);

    u << xy.u;
    v << xy.v;

    // next points
    while(sr.get(x,y)) {
        x1 += (x << shift);
        y1 += (y << shift);

        if(x1 >= 0x800000 && !isNegative) x1 = 0x7fffff;

        xy.u = GARMIN_RAD(x1);
        xy.v = GARMIN_RAD(y1);
//         xy = pj_fwd(xy,*gpProj);
#ifdef DEBUG_SHOW_POLY_PTS
        qDebug() << xy.u << xy.v << hex << x1 << y1 << DEG(x1) << DEG(y1);
#endif
        u << xy.u;
        v << xy.v;
    }

    u.squeeze();
    v.squeeze();
    id = cnt++;
//         qDebug() << "<<<" << id;
    return bytes_total;
}


void CGarminPolygon::bits_per_coord(quint8 base, quint8 bfirst, quint32& bx, quint32& by, sign_info_t& signinfo)
{
    bool x_sign_same, y_sign_same;

    x_sign_same = bfirst & 0x1;

    if(x_sign_same) {
        signinfo.x_has_sign = false;
        signinfo.nx         = bfirst & 0x2;
        ++signinfo.sign_info_bits;
    }
    else {
        signinfo.x_has_sign = true;
    }
    bx = bits_per_coord(base & 0x0F, signinfo.x_has_sign);

    y_sign_same = x_sign_same ? (bfirst & 0x04) : (bfirst & 0x02);

    if(y_sign_same) {
        signinfo.y_has_sign = false;
        signinfo.ny         = x_sign_same ? bfirst & 0x08 : bfirst & 0x04;
        ++signinfo.sign_info_bits;
    }
    else {
        signinfo.y_has_sign = true;
    }

    by = bits_per_coord((base>>4) & 0x0F, signinfo.y_has_sign);

}


// extract bits per coordinate
int CGarminPolygon::bits_per_coord(quint8 base, bool is_signed)
{
    int n= 2;

    if ( base <= 9 ) n+= base;
    else n+= (2*base-9);

    if ( is_signed ) ++n;
    return n;
}


CShiftReg::CShiftReg(const quint8* pData, quint32 n, quint32 bx, quint32 by, bool extra_bit, sign_info_t& si)
: reg(0)
, pData(pData)
, bytes(n)
, xmask(0xFFFFFFFF)
, ymask(0xFFFFFFFF)
, xsign(1)
, ysign(1)
, xsign2(2)
, ysign2(2)
, bits(0)
, bits_per_x(bx)
, bits_per_y(by)
, bits_per_coord(bx + by + (extra_bit ? 1 : 0))
, sinfo(si)
, extraBit(extra_bit)
{
    // create bit masks
    xmask = (xmask << (32-bx)) >> (32-bx);
    ymask = (ymask << (32-by)) >> (32-by);

    xsign   <<= (bits_per_x - 1);
    ysign   <<= (bits_per_y - 1);
    xsign2  = xsign<<1;
    ysign2  = ysign<<1;

    // add sufficient bytes for the first coord. pair
    fill(bits_per_coord + si.sign_info_bits);

    // get rid of sign setup bytes
    reg >>= si.sign_info_bits;
    bits -= si.sign_info_bits;
}


bool CShiftReg::get(qint32& x, qint32& y)
{
    x = y = 0;
    if(bits < (bits_per_coord)) return false;

    // don't know what to do with it -> skip extra bit
    if(extraBit) {
        reg >>= 1;
        bits -= 1;
    }

    if(sinfo.x_has_sign) {
        qint32 tmp = 0;
        while(1) {
            tmp = reg & xmask;
            if(tmp != xsign) {
                break;
            }
            x += tmp - 1;
            reg >>= bits_per_x;
            bits -= bits_per_x;
            fill(bits_per_y + bits_per_x);
        }
        if(tmp < xsign) {
            x += tmp;
        }
        else {
            x = tmp - (xsign2) - x;
        }

    }
    else {
        x = reg & xmask;
        if(sinfo.nx) {
            x = -x;
        }
    }
    reg >>= bits_per_x;
    bits -= bits_per_x;

    // take y coord., add sign if neccessary, shift register by bits per y coord.
    if(sinfo.y_has_sign) {
        qint32 tmp = 0;
        while(1) {
            tmp = reg & ymask;
            if(tmp != ysign) {
                break;
            }
            y += tmp - 1;
            reg >>= bits_per_y;
            bits -= bits_per_y;
            fill(bits_per_y);
        }
        if(tmp < ysign) {
            y += tmp;
        }
        else {
            y = tmp - (ysign2) - y;
        }
    }
    else {
        y = reg & ymask;
        if(sinfo.ny) {
            y = -y;
        }
    }
    reg >>= bits_per_y;
    bits -= bits_per_y;

    // fill register until it has enought bits for one coord. pair again
    fill(bits_per_coord);
    return true;
}


void CShiftReg::fill(quint32 b)
{
    quint64 tmp = 0;
    while((bits < b) && bytes) {
#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
        (quint8&)tmp = *pData++;
#else
        tmp = *pData++;
#endif
        --bytes;

        reg |= tmp << bits;
        bits += 8;
    }
}

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

**********************************************************************************************/
#ifndef CGPX_H
#define CGPX_H

#include <QObject>
#include <QtXml/QDomDocument>

/// handle geo data from GPX files
class CGpx : public QObject, public QDomDocument
{
    Q_OBJECT;
    public:
        CGpx(QObject * parent);
        virtual ~CGpx();

        void load(const QString& filename);
        void save(const QString& filename);

    protected:
        void writeMetadata();

};
#endif                           //CGPX_H

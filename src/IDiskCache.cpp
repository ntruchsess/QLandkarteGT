/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#include "IDiskCache.h"
#ifndef STANDALONE
#include "CResources.h"
#endif                           //!STANDALONE

#include <QtGui>

#ifdef STANDALONE
IDiskCache::IDiskCache(const QString &path, QObject *parent)
#else
IDiskCache::IDiskCache(QObject *parent)
#endif                           //STANDALONE
: QObject(parent)
{
}


IDiskCache::~IDiskCache()
{
}


void IDiskCache::store(const QString& key, QImage& img)
{
}


void IDiskCache::restore(const QString& key, QImage& img)
{
    img = QImage();
}


bool IDiskCache::contains(const QString& key)
{
    return false;
}

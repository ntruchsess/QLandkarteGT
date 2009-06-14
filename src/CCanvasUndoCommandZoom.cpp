//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- Copyright (c) 2009 Marc Feld
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

#include "CCanvasUndoCommandZoom.h"
#include <QMap>
#include <QString>
#include <QDebug>
#include "CMapDB.h"

CCanvasUndoCommandZoom::CCanvasUndoCommandZoom(bool in, const QPoint &p)
: in(in) , p(p)
{
    if(in)
        setText(QObject::tr("Zoom in"));
    else
        setText(QObject::tr("Zoom out"));
}


CCanvasUndoCommandZoom::~CCanvasUndoCommandZoom()
{
}


void CCanvasUndoCommandZoom::redo()
{
    CMapDB::self().getMap().zoom(in, p);
}


void CCanvasUndoCommandZoom::undo()
{
    CMapDB::self().getMap().zoom(!in, p);
}
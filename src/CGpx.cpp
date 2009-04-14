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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

**********************************************************************************************/
#include "CGpx.h"

#include <QtCore>
#include <QMessageBox>

CGpx::CGpx(QObject * parent)
: QObject(parent)
, QDomDocument()
{
    writeMetadata();
}


CGpx::~CGpx()
{

}


void CGpx::writeMetadata()
{
    QDomElement root = createElement("gpx");
    appendChild(root);
    root.setAttribute("version","1.1");
    root.setAttribute("creator","QLandkarteGT");
    root.setAttribute("xmlns","http://www.topografix.com/GPX/1/1");
    root.setAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
    root.setAttribute("xmlns:rmc","http://www.qlandkarte.org/");
    root.setAttribute("xsi:schemaLocation","http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");

    QDomElement metadata = createElement("metadata");
    root.appendChild(metadata);

    QDomElement link = createElement("link");
    metadata.appendChild(link);
    link.setAttribute("href","http://www.qlandkarte.org/");
    QDomElement text = createElement("text");
    link.appendChild(text);
    QDomText _text_ = createTextNode("QLandkarteGT");
    text.appendChild(_text_);

    QDomElement time = createElement("time");
    metadata.appendChild(time);
    QDomText _time_ = createTextNode(QDateTime::currentDateTime().toUTC().toString("yyyy-MM-dd'T'hh:mm:ss'Z'"));
    time.appendChild(_time_);

    //     QDomElement bounds = createElement("bounds");
    //     metadata.appendChild(bounds);
    //     bounds.setAttribute("minlat",0.0);
    //     bounds.setAttribute("minlon",0.0);
    //     bounds.setAttribute("maxlat",0.0);
    //     bounds.setAttribute("maxlon",0.0);
}


void CGpx::save(const QString& filename)
{
    QFile file(filename);

    if(file.exists()) {
        CGpx gpx(0);
        try
        {
            gpx.load(filename);
            const  QDomElement& docElem = gpx.documentElement();
            const QDomNamedNodeMap& attr = docElem.attributes();
            if(attr.namedItem("creator").nodeValue() != "QLandkarteGT") {
                throw tr("bad application");
            }
        }
        catch(const QString& msg) {
            int res = QMessageBox::warning(0,tr("File exists ...")
                ,tr("The file exists and it has not been created by QLandkarte GT. "
                "If you press 'yes' all data in this file will be lost. "
                "Even if this file contains GPX data, QLandkarte GT might not "
                "load and store all elements of this file. Those elements "
                "will be lost. I recommend to use another file. "
                "<b>Do you really want to overwrite the file?</b>")
                ,QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
            if(res == QMessageBox::No) {
                return;
            }
        }
    }

    if(!file.open(QIODevice::WriteOnly)) {
        throw tr("Failed to open: ") + filename;
    }
    QTextStream out(&file);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>" << endl;;
    out << toString();
    file.close();
}


void CGpx::load(const QString& filename)
{
    clear();
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        throw tr("Failed to open: ") + filename;
    }

    QString msg;
    int line;
    int column;
    if(!setContent(&file, &msg, &line, &column)) {
        file.close();
        throw tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg);
    }
    file.close();

    const  QDomElement& docElem = documentElement();
    if(docElem.tagName() != "gpx") {
        throw tr("Not a GPX file: ") + filename;
    }

}

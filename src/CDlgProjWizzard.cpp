/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDlgProjWizzard.h"
#include "mitab.h"
#include <proj_api.h>

#include <QtGui>
#include <QMessageBox>

struct mitab_entry_t {QString name; int idx;};

static bool mitabLessThan(const mitab_entry_t &s1, const mitab_entry_t &s2)
{
    return s1.name < s2.name;
}


CDlgProjWizzard::CDlgProjWizzard(QLineEdit& line, QWidget * parent)
: QDialog(parent)
, line(line)
{
    setupUi(this);

    mitab_entry_t           entry;
    QList<mitab_entry_t>    list;
    int idx                 = 0;
    const MapInfoDatumInfo * di   = asDatumInfoListQL;

    while(di->nMapInfoDatumID != -1)
    {
        entry.name  = di->pszOGCDatumName;
        entry.idx   = idx;
        list << entry;
        ++di;++idx;
    }
    qSort(list.begin(), list.end(), mitabLessThan);

    foreach(entry, list)
    {
        comboDatum->addItem(entry.name, entry.idx);
    }

    comboHemisphere->addItem(tr("north"), "");
    comboHemisphere->addItem(tr("south"), "+south");

    connect(radioLonLat, SIGNAL(clicked()), this, SLOT(slotChange()));
    connect(radioMercator, SIGNAL(clicked()), this, SLOT(slotChange()));
    connect(radioWorldMercator, SIGNAL(clicked()), this, SLOT(slotChange()));
    connect(radioUTM, SIGNAL(clicked()), this, SLOT(slotChange()));
    connect(radioUserDef, SIGNAL(clicked()), this, SLOT(slotChange()));
    connect(comboDatum, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChange()));
    connect(comboHemisphere, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChange()));
    connect(lineUserDef, SIGNAL(textChanged(const QString&)), this, SLOT(slotChange()));
    connect(spinUTMZone, SIGNAL(valueChanged(int)), this, SLOT(slotChange()));

    QString projstr = line.text();
    QRegExp re1("\\+proj=longlat\\s(.*)");
    QRegExp re2("\\+proj=merc \\+a=6378137 \\+b=6378137 \\+lat_ts=0.0 \\+lon_0=0.0 \\+x_0=0.0 \\+y_0=0 \\+k=1.0 \\+units=m \\+nadgrids=@null \\+no_defs");
    QRegExp re3("\\+proj=merc\\s(.*)");
    QRegExp re4("\\+proj=utm \\+zone=([0-9]+)\\s(.*)");

    if(re1.exactMatch(projstr))
    {
        radioLonLat->setChecked(true);
        findDatum(re1.cap(1));
    }
    else if(re2.exactMatch(projstr))
    {
        radioWorldMercator->setChecked(true);
    }
    else if(re3.exactMatch(projstr))
    {
        radioMercator->setChecked(true);
        findDatum(re3.cap(1));
    }
    else if(re4.exactMatch(projstr))
    {
        radioUTM->setChecked(true);
        spinUTMZone->setValue(re4.cap(1).toInt());

        QString datum = re4.cap(2);
        if(datum.startsWith("+south "))
        {
            datum = datum.mid(7);
            comboHemisphere->setCurrentIndex(1);
        }

        findDatum(datum);
    }

    slotChange();
}


CDlgProjWizzard::~CDlgProjWizzard()
{

}


void CDlgProjWizzard::findDatum(const QString& str)
{
    QString cmp;
    int idx = 0;
    const MapInfoDatumInfo * di   = asDatumInfoListQL;

    while(di->nMapInfoDatumID != -1)
    {

        cmp.clear();
        if(di->pszOGCDatumName != QString(""))
        {
            const MapInfoSpheroidInfo * si = asSpheroidInfoList;
            while(si->nMapInfoId != -1)
            {
                if(si->nMapInfoId == di->nEllipsoid) break;
                ++si;
            }

            cmp += QString("+a=%1 +b=%2 ").arg(si->dfA,0,'f',4).arg(si->dfA * (1.0 - (1.0/si->dfInvFlattening)),0,'f',4);
            cmp += QString("+towgs84=%1,%2,%3,%4,%5,%6,%7,%8 ").arg(di->dfShiftX).arg(di->dfShiftY).arg(di->dfShiftZ).arg(di->dfDatumParm0).arg(di->dfDatumParm1).arg(di->dfDatumParm2).arg(di->dfDatumParm3).arg(di->dfDatumParm4);
            cmp += "+units=m  +no_defs";
        }

        if(cmp == str)
        {
            comboDatum->setCurrentIndex(comboDatum->findText(di->pszOGCDatumName));
            break;
        }

        ++di;++idx;
    }

}


void CDlgProjWizzard::slotChange()
{
    QString str;
    if(radioLonLat->isChecked())
    {
        str += "+proj=longlat ";
    }
    else if(radioMercator->isChecked())
    {
        str += "+proj=merc ";
    }
    else if(radioWorldMercator->isChecked())
    {
        str += "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs";
        labelResult->setText(str);
        return;
    }
    else if(radioUTM->isChecked())
    {
        str += QString("+proj=utm +zone=%1 %2 ").arg(spinUTMZone->value()).arg(comboHemisphere->itemData(comboHemisphere->currentIndex()).toString());

    }
    else if(radioUserDef->isChecked())
    {
        str += lineUserDef->text() + " ";
    }

    int idx = comboDatum->itemData(comboDatum->currentIndex()).toInt();
    const MapInfoDatumInfo di = asDatumInfoListQL[idx];
    if(di.pszOGCDatumName != QString(""))
    {
        const MapInfoSpheroidInfo * si = asSpheroidInfoList;
        while(si->nMapInfoId != -1)
        {
            if(si->nMapInfoId == di.nEllipsoid) break;
            ++si;
        }

        str += QString("+a=%1 +b=%2 ").arg(si->dfA,0,'f',4).arg(si->dfA * (1.0 - (1.0/si->dfInvFlattening)),0,'f',4);
        str += QString("+towgs84=%1,%2,%3,%4,%5,%6,%7,%8 ").arg(di.dfShiftX).arg(di.dfShiftY).arg(di.dfShiftZ).arg(di.dfDatumParm0).arg(di.dfDatumParm1).arg(di.dfDatumParm2).arg(di.dfDatumParm3).arg(di.dfDatumParm4);
        str += "+units=m  +no_defs";
    }

    labelResult->setText(str);
}


void CDlgProjWizzard::accept()
{
    if (CDlgProjWizzard::validProjStr(labelResult->text()))
    {
        line.setText(labelResult->text());
        line.setCursorPosition(0);
        QDialog::accept();
    }
}


bool CDlgProjWizzard::validProjStr(const QString projStr)
{
    projPJ projCheck = pj_init_plus(projStr.toUtf8().data());

    if (!projCheck)
    {
        QMessageBox::warning(0, tr("Error..."),tr("The value\n'%1'\nis not a valid coordinate system definition:\n%2").arg(projStr).arg(pj_strerrno(pj_errno)),QMessageBox::Abort,QMessageBox::Abort);
        return false;
    }
    else
    {
        pj_free(projCheck);
        return true;
    }
}

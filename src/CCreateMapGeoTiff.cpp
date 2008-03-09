/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

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

#include "CCreateMapGeoTiff.h"
#include "CMainWindow.h"
#include "CMapDB.h"
#include "GeoMath.h"

#include <projects.h>

#include <QtGui>

CCreateMapGeoTiff * CCreateMapGeoTiff::m_self = 0;

CCreateMapGeoTiff::CCreateMapGeoTiff(QWidget * parent)
: QWidget(parent)
, refcnt(0)
, state(eNone)
{
    m_self = this;
    setupUi(this);
    labelStep1->setPixmap(QPixmap(":/pics/Step1"));
    labelStep2->setPixmap(QPixmap(":/pics/Step2"));
    labelStep3->setPixmap(QPixmap(":/pics/Step3"));

    connect(pushOpenFile, SIGNAL(clicked()), this, SLOT(slotOpenFile()));
    connect(pushAddRef, SIGNAL(clicked()), this, SLOT(slotAddRef()));
    connect(pushDelRef, SIGNAL(clicked()), this, SLOT(slotDelRef()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(slotItemDoubleClicked(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(slotItemClicked(QTreeWidgetItem*, int)));
    connect(pushGoOn, SIGNAL(clicked()), this, SLOT(slotGoOn()));
    connect(&cmd, SIGNAL(readyReadStandardError()), this, SLOT(slotStderr()));
    connect(&cmd, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdout()));
    connect(&cmd, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));

    QSettings cfg;
    lineProjection->setText(cfg.value("create/def.proj","").toString());


    theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveRefPoint);
}


CCreateMapGeoTiff::~CCreateMapGeoTiff()
{
    theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveArea);
    m_self = 0;
}

void CCreateMapGeoTiff::enableStep2()
{
    labelStep2->setEnabled(true);
    treeWidget->setEnabled(true);
    labelDoc2->setEnabled(true);
    pushAddRef->setEnabled(true);
}

void CCreateMapGeoTiff::enableStep3()
{
    labelStep3->setEnabled(true);
    textBrowser->setEnabled(true);
    labelDoc3->setEnabled(true);
}

void CCreateMapGeoTiff::slotOpenFile()
{
    QString filename = QFileDialog::getOpenFileName(0, tr("Open map file..."),"./");
    if(filename.isEmpty()) return;

    CMapDB::self().openMap(filename, *theMainWindow->getCanvas());
    labelInputFile->setText(filename);

    QFileInfo fi(filename);
    QString path = fi.absolutePath();
    QString name = fi.baseName();

    labelOutputFile->setText(path + "/" + name + "_ref.tif");

    enableStep2();
}

void CCreateMapGeoTiff::slotAddRef()
{
    refpt_t& pt     = refpts[++refcnt];
    pt.item         = new QTreeWidgetItem(treeWidget);

    pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    pt.item->setText(eNum,tr("%1").arg(refcnt));
    pt.item->setData(eNum,Qt::UserRole,refcnt);
    pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
    pt.item->setText(eLonLat,tr(""));


    QPoint center   = theMainWindow->getCanvas()->rect().center();
    IMap& map       = CMapDB::self().getMap();
    pt.x            = center.x();
    pt.y            = center.y();
    map.convertPt2M(pt.x,pt.y);

    pt.item->setText(eX,QString::number(pt.x));
    pt.item->setText(eY,QString::number(pt.y));

    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
    for(int i=0; i < eMaxColumn - 1; ++i) {
        treeWidget->resizeColumnToContents(i);
    }


    pushGoOn->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();
}

void CCreateMapGeoTiff::slotDelRef()
{
    QTreeWidgetItem * item = treeWidget->currentItem();
    refpts.remove(item->data(eNum,Qt::UserRole).toUInt());
    delete item;
}

void CCreateMapGeoTiff::slotSelectionChanged()
{
    pushDelRef->setEnabled(treeWidget->currentItem());
}

void CCreateMapGeoTiff::slotItemDoubleClicked(QTreeWidgetItem * item)
{
    IMap& map   = CMapDB::self().getMap();
    refpt_t& pt = refpts[item->data(eNum,Qt::UserRole).toUInt()];
    double x = pt.x;
    double y = pt.y;
    map.convertM2Pt(x,y);

    map.move(QPoint(x,y), theMainWindow->getCanvas()->rect().center());
    theMainWindow->getCanvas()->update();
}

void CCreateMapGeoTiff::slotItemClicked(QTreeWidgetItem * item, int column)
{
    if(column == eLonLat){
        treeWidget->editItem(item,column);
    }
}

void CCreateMapGeoTiff::slotGoOn()
{
    QStringList args;
    QRegExp re("^\\s*([0-9]+)\\.{0,1}[0-9]*\\s+([0-9]+)\\.{0,1}[0-9]*\\s*$");

    QString projection = lineProjection->text();
    if(projection.isEmpty()){
        projection = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs";
    }

    QSettings cfg;
    cfg.setValue("create/def.proj",projection);

    PJ * pjWGS84    = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    PJ * pjTar      = pj_init_plus(projection.toLatin1());

    if(pjTar == 0){
        QMessageBox::warning(0,tr("Error ..."), tr("Failed to setup projection. Bad syntax?"), QMessageBox::Abort,QMessageBox::Abort);
        return;
    }

    bool islonlat = projection.contains("longlat");

    args << "-a_srs" << projection;

    QMap<quint32,refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end()){
        float lon = 0, lat = 0;
        double u = 0, v = 0;
        args << "-gcp";
        args << QString::number(refpt->x);
        args << QString::number(refpt->y);
        if(islonlat){
            if(!GPS_Math_Str_To_Deg(refpt->item->text(eLonLat), lon, lat)){
                return;
            }
            u = lon;
            v = lat;
        }
        else{

            if(GPS_Math_Str_To_Deg(refpt->item->text(eLonLat), lon, lat,true)){
                u = lon * DEG_TO_RAD;
                v = lat * DEG_TO_RAD;
                pj_transform(pjWGS84,pjTar,1,0,&u,&v,0);
            }
            else{
                if(!re.exactMatch(refpt->item->text(eLonLat))){
                    treeWidget->setCurrentItem(refpt->item);
                    QMessageBox::warning(0,tr("Error ..."), tr("Failed to read reference coordinate. Bad syntax?"), QMessageBox::Abort,QMessageBox::Abort);
                    return;
                }
                u = re.cap(1).toDouble();
                v = re.cap(2).toDouble();
            }
        }

        args << QString::number(u,'f',6);
        args << QString::number(v,'f',6);
        ++refpt;
    }

    tmpfile1 = new QTemporaryFile();
    tmpfile1->open();
    args << labelInputFile->text() << tmpfile1->fileName();

    textBrowser->clear();
    textBrowser->setTextColor(Qt::black);
    textBrowser->append("gdal_translate " +  args.join(" ") + "\n");

    state = eTranslate;
    cmd.start("gdal_translate", args);

    enableStep3();
}

void CCreateMapGeoTiff::slotStderr()
{
    textBrowser->setTextColor(Qt::red);
    textBrowser->append(cmd.readAllStandardError());
}

void CCreateMapGeoTiff::slotStdout()
{
    textBrowser->setTextColor(Qt::blue);
    textBrowser->append(cmd.readAllStandardOutput());
}

void CCreateMapGeoTiff::slotFinished( int exitCode, QProcess::ExitStatus status)
{
    qDebug() << exitCode << status;
    if(exitCode != 0){
        textBrowser->append(tr("Failed!\n"));
        cleanupTmpFiles();
        return;
    }

    if(state == eTranslate){
        state = eWarp;
        QStringList args;
        args << "-dstnodata" << "\"255\"";
        args << tmpfile1->fileName();

        tmpfile2 = new QTemporaryFile();
        tmpfile2->open();

        args << tmpfile2->fileName();

        textBrowser->setTextColor(Qt::black);
        textBrowser->append("gdalwarp " +  args.join(" ") + "\n");

        cmd.start("gdalwarp", args);
        return;
    }
    if(state == eWarp){
        state = eTile;

        QFile::remove(labelOutputFile->text());

        QStringList args;
        args << "-co" << "tiled=yes";
        args << "-co" << "blockxsize=256";
        args << "-co" << "blockysize=256";
        args << "-co" << "compress=deflate";
        args << "-co" << "predictor=1";
        args << tmpfile2->fileName();
        args << labelOutputFile->text();

        textBrowser->setTextColor(Qt::black);
        textBrowser->append("gdal_translate " +  args.join(" ") + "\n");

        cmd.start("gdal_translate", args);
        return;
    }
    cleanupTmpFiles();

    textBrowser->setTextColor(Qt::black);
    textBrowser->append(tr("--- finished ---\n"));

}


void CCreateMapGeoTiff::cleanupTmpFiles()
{
    delete tmpfile1;
    delete tmpfile2;
}

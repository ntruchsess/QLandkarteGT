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
#include "CCreateMapGridTool.h"
#include "CMainWindow.h"
#include "CMapDB.h"
#include "GeoMath.h"

#include <projects.h>
#include <ogr_spatialref.h>
#include <gdal_priv.h>

#include <QtGui>

CCreateMapGeoTiff * CCreateMapGeoTiff::m_self = 0;

CCreateMapGeoTiff::CCreateMapGeoTiff(QWidget * parent)
: QWidget(parent)
, refcnt(0)
, state(eNone)
, path("./")
{
    m_self = this;
    setupUi(this);
    labelStep1->setPixmap(QPixmap(":/pics/Step1"));
    labelStep2->setPixmap(QPixmap(":/pics/Step2"));
    labelStep3->setPixmap(QPixmap(":/pics/Step3"));

    connect(pushOpenFile, SIGNAL(clicked()), this, SLOT(slotOpenFile()));
    connect(comboMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotModeChanged(int)));
    connect(pushAddRef, SIGNAL(clicked()), this, SLOT(slotAddRef()));
    connect(pushDelRef, SIGNAL(clicked()), this, SLOT(slotDelRef()));
    connect(pushLoadRef, SIGNAL(clicked()), this, SLOT(slotLoadRef()));
    connect(pushSaveRef, SIGNAL(clicked()), this, SLOT(slotSaveRef()));
    connect(pushGridTool, SIGNAL(clicked()), this, SLOT(slotGridTool()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(slotItemDoubleClicked(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(slotItemChanged(QTreeWidgetItem*, int)));
    connect(pushGoOn, SIGNAL(clicked()), this, SLOT(slotGoOn()));
    connect(&cmd, SIGNAL(readyReadStandardError()), this, SLOT(slotStderr()));
    connect(&cmd, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdout()));
    connect(&cmd, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));
    connect(pushClearAll, SIGNAL(clicked()), this, SLOT(slotClearAll()));

    QSettings cfg;
    lineProjection->setText(cfg.value("create/def.proj","+proj=merc +ellps=WGS84 +datum=WGS84 +no_defs").toString());

    comboMode->addItem(tr("square pixels (2 Ref. Pts.)"), -2);
    comboMode->addItem(tr("linear (3 Ref. Pts.)"), 1);
    comboMode->addItem(tr("quadratic (6 Ref. Pts.)"), 2);
    comboMode->setCurrentIndex(1);

    theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveRefPoint);
}


CCreateMapGeoTiff::~CCreateMapGeoTiff()
{
    theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveArea);
    m_self = 0;
}

int CCreateMapGeoTiff::getNumberOfGCPs()
{
    int n = 0;
    int mode = comboMode->itemData(comboMode->currentIndex()).toInt();
    switch(mode){
        case 1:
            n = 3;
            break;
        case 2:
            n = 6;
            break;
        case -2:
            n = 2;
            break;
    }
    return n;
}

void CCreateMapGeoTiff::enableStep2()
{
    labelStep2->setEnabled(true);
    treeWidget->setEnabled(true);
    labelDoc2->setEnabled(true);
    pushAddRef->setEnabled(true);
    pushLoadRef->setEnabled(true);
    pushGridTool->setEnabled(true);

}

void CCreateMapGeoTiff::enableStep3()
{
    labelStep3->setEnabled(true);
    textBrowser->setEnabled(true);
    labelDoc3->setEnabled(true);
}

void CCreateMapGeoTiff::slotOpenFile()
{
    char str[1024];
    char * ptr = str;

    QSettings cfg;
    path = QDir(cfg.value("path/create",path.path()).toString());


    QString filename = QFileDialog::getOpenFileName(0, tr("Open map file..."),path.path(), tr("Raw TIFF (*.tif)"));
    if(filename.isEmpty()) return;

    CMapDB::self().openMap(filename, *theMainWindow->getCanvas());
    labelInputFile->setText(filename);

    QFileInfo fi(filename);
    path = QDir(fi.absolutePath());
    cfg.setValue("path/create",path.path());
    QString name = fi.baseName();

    labelOutputFile->setText(path.filePath(name + "_ref.tif"));

    GDALDataset * dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
    if(dataset == 0) return;

    sizeOfInputFile = QSize(dataset->GetRasterXSize(), dataset->GetRasterYSize());

    QString proj = dataset->GetGCPProjection();
    strncpy(str, dataset->GetGCPProjection(), sizeof(str));
    OGRSpatialReference oSRS;
    oSRS.importFromWkt(&ptr);
    oSRS.exportToProj4(&ptr);
    lineProjection->setText(ptr);

    gdalGCP2RefPt(dataset->GetGCPs(), dataset->GetGCPCount());

    delete dataset;

    enableStep2();
}

void CCreateMapGeoTiff::slotModeChanged(int)
{
    pushGoOn->setEnabled(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
}

void CCreateMapGeoTiff::slotAddRef()
{
    refpt_t& pt     = refpts[++refcnt];
    pt.item         = new QTreeWidgetItem();

    pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    pt.item->setData(eLabel,Qt::UserRole,refcnt);
    pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
    pt.item->setText(eLonLat,tr(""));


    QPoint center   = theMainWindow->getCanvas()->rect().center();
    IMap& map       = CMapDB::self().getMap();
    pt.x            = center.x();
    pt.y            = center.y();
    map.convertPt2M(pt.x,pt.y);

    pt.item->setText(eX,QString::number(pt.x));
    pt.item->setText(eY,QString::number(pt.y));

    treeWidget->addTopLevelItem(pt.item);

    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
    for(int i=0; i < eMaxColumn - 1; ++i) {
        treeWidget->resizeColumnToContents(i);
    }

    pushGoOn->setEnabled(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();
}

void CCreateMapGeoTiff::addRef(double x, double y, double u, double v)
{
    refpt_t& pt     = refpts[++refcnt];
    pt.item         = new QTreeWidgetItem();

    pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    pt.item->setData(eLabel,Qt::UserRole,refcnt);
    pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
    pt.item->setText(eLonLat,tr(""));

    pt.x            = x;
    pt.y            = y;

    pt.item->setText(eX,QString::number(pt.x));
    pt.item->setText(eY,QString::number(pt.y));
    pt.item->setText(eLonLat,QString("%1 %2").arg(u,0,'f',6).arg(v,0,'f',6));

    treeWidget->addTopLevelItem(pt.item);

    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
    for(int i=0; i < eMaxColumn - 1; ++i) {
        treeWidget->resizeColumnToContents(i);
    }

    pushGoOn->setEnabled(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);
}

void CCreateMapGeoTiff::keyPressEvent(QKeyEvent * e)
{
    if(e->key() == Qt::Key_Delete) {
        slotDelRef();
        e->accept();
    }
    else {
        QWidget::keyPressEvent(e);
    }
}

void CCreateMapGeoTiff::slotDelRef()
{
    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    QTreeWidgetItem * item;

    foreach(item, items){
        refpts.remove(item->data(eLabel,Qt::UserRole).toUInt());
        delete item;
    }
    pushGoOn->setEnabled(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();
}

void CCreateMapGeoTiff::slotLoadRef()
{

    QString filename = QFileDialog::getOpenFileName(0, tr("Load reference points..."),path.path(),"Ref. points (*.gcp *.tab)");
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix() == "gcp"){
        loadGCP(filename);
    }
    else if(fi.suffix() == "tab"){
        loadTAB(filename);
    }


    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
    for(int i=0; i < eMaxColumn - 1; ++i) {
        treeWidget->resizeColumnToContents(i);
    }

    pushGoOn->setEnabled(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();

}

void CCreateMapGeoTiff::loadGCP(const QString& filename)
{
    QRegExp re1("^-gcp\\s(.*)\\s(-{0,1}[0-9]+)\\s(-{0,1}[0-9]+)\\s(.*)$");
    QRegExp re2("^-proj\\s(.*)$");

    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd()){
        QString line = QString::fromUtf8(file.readLine());

        if(re1.exactMatch(line)){
            refpt_t& pt     = refpts[++refcnt];
            pt.item         = new QTreeWidgetItem();

            pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

//             pt.item->setText(eNum,tr("%1").arg(refcnt));
            pt.item->setData(eLabel,Qt::UserRole,refcnt);
            QString label = re1.cap(4).trimmed();
            if(label.isEmpty()){
                pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
            }
            else{
                pt.item->setText(eLabel,label);
            }
            pt.item->setText(eLonLat,re1.cap(1));

            pt.x = re1.cap(2).toDouble();
            pt.y = re1.cap(3).toDouble();
            pt.item->setText(eX,QString::number(pt.x));
            pt.item->setText(eY,QString::number(pt.y));

            treeWidget->addTopLevelItem(pt.item);
        }
        else if(re2.exactMatch(line)){
            lineProjection->setText(re2.cap(1).trimmed());
        }
    }

    file.close();
}

void CCreateMapGeoTiff::loadTAB(const QString& filename)
{
    char *pszTabWKT = 0;
    GDAL_GCP *gcpm;
    int n;

    double adfGeoTransform[6];
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;

    if(GDALReadTabFile(filename.toLocal8Bit(),adfGeoTransform,&pszTabWKT,&n,&gcpm)){

        gdalGCP2RefPt(gcpm,n);

        char * ptr = 0;
        OGRSpatialReference oSRS;
        oSRS.importFromWkt(&pszTabWKT);
        oSRS.exportToProj4(&ptr);

        lineProjection->setText(ptr);
    }
}


void CCreateMapGeoTiff::gdalGCP2RefPt(const GDAL_GCP* gcps, int n)
{
    int i;
    if (n) for(i=0;i<n;i++){
        refpt_t& pt     = refpts[++refcnt];
        pt.item         = new QTreeWidgetItem();

        pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

        pt.item->setData(eLabel,Qt::UserRole,refcnt);
        QString label = gcps[i].pszInfo;
        if(label.isEmpty()){
            pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
        }
        else{
            pt.item->setText(eLabel,label);
        }
        pt.item->setText(eLonLat,QString("%1 %2").arg(gcps[i].dfGCPX,0,'f',6).arg(gcps[i].dfGCPY,0,'f',6));

        pt.x = gcps[i].dfGCPPixel;
        pt.y = gcps[i].dfGCPLine;
        pt.item->setText(eX,QString::number(pt.x,'f',6));
        pt.item->setText(eY,QString::number(pt.y,'f',6));

        treeWidget->addTopLevelItem(pt.item);
    }
}

void CCreateMapGeoTiff::slotSaveRef()
{
    QFileInfo fin(labelInputFile->text());
    QString base = fin.baseName();

    QString filename = QFileDialog::getSaveFileName(0, tr("Save reference points..."),path.filePath(base + ".gcp"),"Ref. points (*.gcp)");
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix() != "gcp"){
        filename += ".gcp";
    }

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QStringList args;
    args << "-proj";
    args << lineProjection->text().trimmed();
    args << "\n";
    file.write(args.join(" ").toUtf8());


    QMap<quint32,refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end()){
        args.clear();
        args << "-gcp";
        args << refpt->item->text(eLonLat);
        args << QString::number(refpt->x,'f',0);
        args << QString::number(refpt->y,'f',0);
        args << refpt->item->text(eLabel);
        args << "\n";
        file.write(args.join(" ").toUtf8());
        ++refpt;
    }
    file.close();
}

void CCreateMapGeoTiff::slotGridTool()
{
    CCreateMapGridTool * tool = new CCreateMapGridTool(this, parentWidget());
    theMainWindow->setTempWidget(tool);
}

void CCreateMapGeoTiff::slotSelectionChanged()
{
    pushDelRef->setEnabled(treeWidget->currentItem());
}

void CCreateMapGeoTiff::slotItemDoubleClicked(QTreeWidgetItem * item)
{
    IMap& map   = CMapDB::self().getMap();
    refpt_t& pt = refpts[item->data(eLabel,Qt::UserRole).toUInt()];
    double x = pt.x;
    double y = pt.y;
    map.convertM2Pt(x,y);

    map.move(QPoint(x,y), theMainWindow->getCanvas()->rect().center());
    theMainWindow->getCanvas()->update();
}


void CCreateMapGeoTiff::slotItemChanged(QTreeWidgetItem * item, int column)
{
    refpt_t& pt = refpts[item->data(eLabel,Qt::UserRole).toUInt()];
    pt.x = item->text(eX).toDouble();
    pt.y = item->text(eY).toDouble();

    theMainWindow->getCanvas()->update();
}

void CCreateMapGeoTiff::slotGoOn()
{
    QStringList args;
    QRegExp re("^\\s*([0-9\\.]+)\\s+([0-9\\.]+)\\s*$");

    QString projection = lineProjection->text();
    if(projection.isEmpty()){
        projection = "+proj=merc +ellps=WGS84 +datum=WGS84 +no_defs";
    }

    QSettings cfg;
    cfg.setValue("create/def.proj",projection);

    PJ * pjTar      = pj_init_plus(projection.toLatin1());

    if(pjTar == 0){
        QMessageBox::warning(0,tr("Error ..."), tr("Failed to setup projection. Bad syntax?"), QMessageBox::Abort,QMessageBox::Abort);
        return;
    }
    PJ * pjWGS84    = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

    bool islonlat = projection.contains("longlat");

    args << "-a_srs" << projection;

    double x1, x2, y1, y2, u1, u2, v1, v2;
    x1 = x2 = y1 = y2 = u1 = u2 = v1 = v2 = 0;

    QMap<quint32,refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end()){
        float lon = 0, lat = 0;
        double u = 0, v = 0;
        args << "-gcp";

        x1 = x2; x2 = refpt->x;
        y1 = y2; y2 = refpt->y;

        args << QString::number(refpt->x,'f',0);
        args << QString::number(refpt->y,'f',0);
        if(islonlat){
            if(!GPS_Math_Str_To_Deg(refpt->item->text(eLonLat), lon, lat)){
                if(pjWGS84) pj_free(pjWGS84);
                if(pjTar) pj_free(pjTar);
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
                    if(pjWGS84) pj_free(pjWGS84);
                    if(pjTar) pj_free(pjTar);
                    return;
                }
                u = re.cap(1).toDouble();
                v = re.cap(2).toDouble();

                if((abs(u) <= 180) && (abs(v) <= 90)){
                    u = u * DEG_TO_RAD;
                    v = v * DEG_TO_RAD;
                    pj_transform(pjWGS84,pjTar,1,0,&u,&v,0);
                }
            }
        }

        u1 = u2; u2 = u;
        v1 = v2; v2 = v;

        args << QString::number(u,'f',6);
        args << QString::number(v,'f',6);
        ++refpt;
    }

    if(pjWGS84) pj_free(pjWGS84);
    if(pjTar) pj_free(pjTar);


    double adfGeoTransform[6];

    // as gdalwarp needs 3 GCPs at least we add an artificial one on two GCPs
    if ( treeWidget->topLevelItemCount() == 2 ) /* use square pixels */
    {
        double dx, dy, dX, dY, delta;
        double a, b;

        dx =   x2 - x1;
        dy = -(y2 - y1);

        dX =   u2 - u1;
        dY =   v2 - v1;

        delta = dx * dx + dy * dy;

        if (delta < 1 ) /* pixel */
        {
            QMessageBox::warning(0,tr("Error ..."), tr("Reference points are too close."), QMessageBox::Abort, QMessageBox::Abort);
            return;
        }

        a = (dX * dx + dY * dy) / delta;
        b = (dY * dx - dX * dy) / delta;

        adfGeoTransform[0] = u2 - a * x2 - b * y2;
        adfGeoTransform[1] =  a;
        adfGeoTransform[2] = -b;
        adfGeoTransform[3] = v2 - b * x2 + a * y2;
        adfGeoTransform[4] =  b;
        adfGeoTransform[5] =  a;

        /* -gcp 0 0 x0 y0. Image size is not known here?. Let's hope that GCP ! = 0,0 */
        args << "-gcp";
        args << QString::number(0.,'f',6);
        args << QString::number(0.,'f',6);
        args << QString::number(adfGeoTransform[0],'f',6);
        args << QString::number(adfGeoTransform[3],'f',6);
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
    QString str = cmd.readAllStandardOutput();

    textBrowser->insertPlainText(str);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
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

        int mode = comboMode->itemData(comboMode->currentIndex()).toInt();
        if(mode > 0){
            args << "-order" << QString::number(mode);
        }

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

    pushClearAll->setEnabled(true);
}


void CCreateMapGeoTiff::cleanupTmpFiles()
{
    delete tmpfile1;
    delete tmpfile2;
}


void CCreateMapGeoTiff::slotClearAll()
{
    refpts.clear();
    refcnt  = 0;
    state   = eNone;

    treeWidget->clear();
    textBrowser->clear();

    labelInputFile->clear();
    labelOutputFile->clear();

    labelStep2->setEnabled(false);
    treeWidget->setEnabled(false);
    labelDoc2->setEnabled(false);
    pushAddRef->setEnabled(false);
    pushLoadRef->setEnabled(false);
    pushSaveRef->setEnabled(false);
    pushGridTool->setEnabled(false);
    pushDelRef->setEnabled(false);
    pushGoOn->setEnabled(false);

    labelStep3->setEnabled(false);
    textBrowser->setEnabled(false);
    labelDoc3->setEnabled(false);
    pushClearAll->setEnabled(false);

    theMainWindow->getCanvas()->update();
}

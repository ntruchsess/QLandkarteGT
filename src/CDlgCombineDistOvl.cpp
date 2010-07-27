/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License; or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful;
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not; write to the Free Software
    Foundation; Inc.; 59 Temple Place - Suite 330; Boston; MA 02111 USA

**********************************************************************************************/

#include "CDlgCombineDistOvl.h"

#include <QtGui>

CDlgCombineDistOvl::CDlgCombineDistOvl(QWidget * parent)
    : QDialog(parent)
{

    setupUi(this);

    toolAdd->setIcon(QPixmap(":/icons/iconRight16x16.png"));
    connect(toolAdd, SIGNAL(clicked()), this, SLOT(slotAdd()));
    toolDel->setIcon(QPixmap(":/icons/iconLeft16x16.png"));
    connect(toolDel, SIGNAL(clicked()), this, SLOT(slotDel()));

    toolUp->setIcon(QPixmap(":/icons/iconUpload16x16.png"));
    connect(toolUp, SIGNAL(clicked()), this, SLOT(slotUp()));
    toolDown->setIcon(QPixmap(":/icons/iconDownload16x16.png"));
    connect(toolDown, SIGNAL(clicked()), this, SLOT(slotDown()));

    connect(listSelOverlays, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));

//    CTrack * track;
//    QList<CTrack*> tracks =  CTrackDB::self().getTracks().values();

//    foreach(track, tracks)
//    {
//        QListWidgetItem * item = new QListWidgetItem();
//        item->setText(track->getName());
//        item->setData(Qt::UserRole, track->key());
//        listTracks->addItem(item);
//    }

}

CDlgCombineDistOvl::~CDlgCombineDistOvl()
{

}

void CDlgCombineDistOvl::slotAdd()
{
//    QListWidgetItem * item;
//    QList<QListWidgetItem*> items = listTracks->selectedItems();

//    foreach(item, items)
//    {
//        listSelTracks->addItem(listTracks->takeItem(listTracks->row(item)));
//    }
}


void CDlgCombineDistOvl::slotDel()
{
//    QListWidgetItem * item;
//    QList<QListWidgetItem*> items = listSelTracks->selectedItems();

//    foreach(item, items)
//    {
//        listTracks->addItem(listSelTracks->takeItem(listSelTracks->row(item)));
//    }
}


void CDlgCombineDistOvl::accept()
{
//    const QMap<QString,CTrack*>& dict = CTrackDB::self().getTracks();

//    CTrack* track;
//    QList<CTrack*> tracks;

//    QListWidgetItem * item;
//    QList<QListWidgetItem*> items = listSelTracks->findItems("*",Qt::MatchWildcard);

//    foreach(item, items)
//    {
//        tracks << dict[item->data(Qt::UserRole).toString()];
//    }

//    if(tracks.isEmpty() || lineTrackName->text().isEmpty()) return;

//    if(checkSortTimestamp->isChecked())
//    {
//        qSort(tracks.begin(), tracks.end(), trackLessThan);
//    }

//    CTrack * newtrack = new CTrack(&CTrackDB::self());
//    newtrack->setName(lineTrackName->text());

//    foreach(track, tracks)
//    {
//        *newtrack += *track;
//    }

//    CTrackDB::self().addTrack(newtrack, false);

//    QDialog::accept();
}



void CDlgCombineDistOvl::slotItemSelectionChanged ()
{
//    slotSortTimestamp(checkSortTimestamp->isChecked());
}


void CDlgCombineDistOvl::slotUp()
{
//    QListWidgetItem * item = listSelTracks->currentItem();
//    if(item)
//    {
//        int row = listSelTracks->row(item);
//        if(row == 0) return;
//        listSelTracks->takeItem(row);
//        row = row - 1;
//        listSelTracks->insertItem(row,item);
//        listSelTracks->setCurrentItem(item);
//    }
}


void CDlgCombineDistOvl::slotDown()
{
//    QListWidgetItem * item = listSelTracks->currentItem();
//    if(item)
//    {
//        int row = listSelTracks->row(item);
//        if(row == (listSelTracks->count() - 1)) return;
//        listSelTracks->takeItem(row);
//        row = row + 1;
//        listSelTracks->insertItem(row,item);
//        listSelTracks->setCurrentItem(item);
//    }
}

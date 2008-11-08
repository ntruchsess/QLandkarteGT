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
#include "CGarminExport.h"
#include "CMapSelectionGarmin.h"
#include "Platform.h"
#include "CGarminTile.h"

#include <QtGui>

#undef DEBUG_SHOW_SECT_DESC

CGarminExport::CGarminExport(QWidget * parent)
: QDialog(parent)
, e1(9)
, e2(6)
, blocksize(pow(2, e1 + e2))
{
    setupUi(this);
    toolPath->setIcon(QPixmap(":/icons/iconFileLoad16x16"));

    QSettings cfg;
    labelPath->setText(cfg.value("path/export","./").toString());

    linePrefix->setText("gmapsupp.img");

    connect(toolPath, SIGNAL(clicked()), this, SLOT(slotOutputPath()));
    connect(pushExport, SIGNAL(clicked()), this, SLOT(slotStart()));

}

CGarminExport::~CGarminExport()
{

}

void CGarminExport::slotOutputPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select ouput path..."), labelPath->text());
    if(path.isEmpty()) return;

    QSettings cfg;
    cfg.setValue("path/export", path);
    labelPath->setText(path);
}


void CGarminExport::exportToFile(CMapSelectionGarmin& ms)
{
    stdout(tr("Creating image from maps:\n"));

    QMap<QString,CMapSelectionGarmin::map_t>::const_iterator map = ms.maps.begin();
    while(map != ms.maps.end()){

        map_t myMap;
        myMap.map = map->name;
        myMap.key = map->unlockKey;

        maps << myMap;
        if(myMap.key.isEmpty()){
            stdout(tr("Map: %1").arg(myMap.map));
        }
        else{
            stdout(tr("Map: %1 (Key: %2)").arg(myMap.map).arg(myMap.key));
        }

        stdout("Tiles:");

        QMap<QString, CMapSelectionGarmin::tile_t>::const_iterator tile = map->tiles.begin();
        while(tile != map->tiles.end()){
            tile_t myTile;
            myTile.name     = tile->name;
            myTile.filename = tile->filename;
            myTile.memsize  = tile->memSize;
            tiles << myTile;

            stdout(tr("    %1 (%2 MB)").arg(myTile.name).arg(double(myTile.memsize) / (1024 * 1024), 0, 'f', 2));
            ++tile;
        }
        stdout(" ");
        ++map;
    }

    exec();
}

void CGarminExport::stdout(const QString& msg)
{
    textBrowser->setTextColor(Qt::blue);
    textBrowser->append(msg);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
}

void CGarminExport::stderr(const QString& msg)
{
    textBrowser->setTextColor(Qt::red);
    textBrowser->append(msg);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
}

void CGarminExport::readFile(QFile& file, quint32 offset, quint32 size, QByteArray& data, quint8 mask)
{
    file.seek(offset);
    data = file.read(size);

    if((quint32)data.size() != size) {
        throw exce_t(eErrOpen, tr("Failed to read: ") + file.fileName());
    }

    quint8 * p = (quint8*)data.data();
    for(quint32 i = 0; i < size; ++i) {
        *p++ ^= mask;
    }

}

void CGarminExport::initGmapsuppImgHdr(gmapsupp_imghdr_t& hdr)
{
    // get the date
    QDateTime date = QDateTime::currentDateTime();

    // zero structure
    memset(&hdr,0,sizeof(gmapsupp_imghdr_t));

    // space-fill the descriptor strings
    memset(&hdr.desc1,0x20,sizeof(hdr.desc1));
    memset(&hdr.desc2,0x20,sizeof(hdr.desc2) - 1);

    // put in current month and year
    hdr.upMonth = date.date().month();
    hdr.upYear  = date.date().year() - 1900;

    // copy the signature to the 7 byte string
    strncpy(hdr.signature,"DSKIMG",7);

    // copy the identifier to the 7 byte string
    strncpy(hdr.identifier,"GARMIN",7);

    // identify creator in the map description
    memcpy(hdr.desc1,"QLandkarte",10);

    // as far as is known, E1 is always 9, to force a minimum 512 block size
    hdr.e1 = e1;
    // the E2 corresponding to the optimum block size
    hdr.e2 = e2;

    // set the number of file blocks in the named field (note: unaligned!)
//oe    gar_store(uint16_t, hdr.nBlocks1, (quint16) total_blocks);

    // add the "partition table" terminator
    hdr.terminator = gar_endian(uint16_t, 0xAA55);

    // add the data offset
//oe    hdr.dataoffset = gar_endian(uint32_t, dataoffset);

    // create a pointer to set various unnamed fields
    quint8 * p = (quint8*)&hdr;

    // various - watch out for unaligned destinations
                                 // non-standard?
    *(p + 0x0E)               = 0x01;
                                 // standard value
    *(p + 0x17)               = 0x02;
                                 // standard is 0x0004
    *(quint16*)(p + 0x18)     = gar_endian(uint16_t, 0x0020);
                                 // standard is 0x0010
    *(quint16*)(p + 0x1A)     = gar_endian(uint16_t, 0x0020);
                                 // standard is 0x0020
    *(quint16*)(p + 0x1C)     = gar_endian(uint16_t, 0x03C7);
                                 // copies (0x1a)
    gar_ptr_store(uint16_t, p + 0x5D, 0x0020);
                                 // copies (0x18)
    gar_ptr_store(uint16_t, p + 0x5F, 0x0020);

    // date stuff
    gar_ptr_store(uint16_t, p + 0x39, date.date().year());
    *(p + 0x3B)               = date.date().month();
    *(p + 0x3C)               = date.date().day();
    *(p + 0x3D)               = date.time().hour();
    *(p + 0x3E)               = date.time().minute();
    *(p + 0x3F)               = date.time().second();
                                 // 0x02 standard. bit for copied map set?
    *(p + 0x40)               = 0x08;

    // more
                                 // copies (0x18) to a *2* byte field
    *(quint16*)(p + 0x1C4)    = gar_endian(uint16_t, 0x0020);
    /*  // not really needed ???
     *(p + 0x1C0)               = 0x01; // standard value
     *(p + 0x1C3)               = 0x15; // normal, but *not* (0x1A) - 1
     *(p + 0x1C4)               = 0x10; // value above
     *(p + 0x1C5)               = 0x00;
     */

    // set the number of file blocks at 0x1CA
//oe    *(quint16*)(p + 0x1CA) = gar_endian(uint16_t, (quint16) total_blocks);

}

void CGarminExport::readTileInfo(tile_t& t)
{
    char tmpstr[64];
    quint8 mask;

    QMap<QString, gmapsupp_subfile_desc_t>& subfiles = t.subfiles;

    qint64  fsize = QFileInfo(t.filename).size();

    QFile file(t.filename);
    if(!file.open(QIODevice::ReadOnly)) {
        throw exce_t(eErrOpen, tr("Failed to open: ") + t.filename);
    }
    file.read((char*)&mask, 1);

    // read hdr_img_t
    QByteArray imghdr;
    readFile(file, 0, sizeof(CGarminTile::hdr_img_t), imghdr, mask);
    CGarminTile::hdr_img_t * pImgHdr = (CGarminTile::hdr_img_t*)imghdr.data();

    if(strncmp(pImgHdr->signature,"DSKIMG",7) != 0) {
        throw exce_t(errFormat,tr("Bad file format: ") + t.filename);
    }
    if(strncmp(pImgHdr->identifier,"GARMIN",7) != 0) {
        throw exce_t(errFormat,tr("Bad file format: ") + t.filename);
    }

//     mapdesc  = QByteArray((const char*)pImgHdr->desc1,20);
//     mapdesc += pImgHdr->desc2;
    //     qDebug() << mapdesc;

    size_t blocksize = pImgHdr->blocksize();

    // 1st read FAT
    QByteArray FATblock;
    readFile(file, sizeof(CGarminTile::hdr_img_t), sizeof(CGarminTile::FATblock_t), FATblock, mask);
    const CGarminTile::FATblock_t * pFATBlock = (const CGarminTile::FATblock_t * )FATblock.data();

    size_t dataoffset = sizeof(CGarminTile::hdr_img_t);

    // skip dummy blocks at the beginning
    while(dataoffset < (size_t)fsize) {
        if(pFATBlock->flag != 0x00) {
            break;
        }
        dataoffset += sizeof(CGarminTile::FATblock_t);
        readFile(file, dataoffset, sizeof(CGarminTile::FATblock_t), FATblock, mask);
        pFATBlock = (const CGarminTile::FATblock_t * )FATblock.data();
    }

    QSet<QString> subfileNames;
    while(dataoffset < (size_t)fsize) {
        if(pFATBlock->flag != 0x01) {
            break;
        }

        memcpy(tmpstr,pFATBlock->name,sizeof(pFATBlock->name) + sizeof(pFATBlock->type));
        tmpstr[sizeof(pFATBlock->name) + sizeof(pFATBlock->type)] = 0;

        if(gar_load(uint32_t, pFATBlock->size) != 0 && !subfileNames.contains(tmpstr) && tmpstr[0] != 0x20) {
            subfileNames << tmpstr;

            memcpy(tmpstr,pFATBlock->name,sizeof(pFATBlock->name));
            tmpstr[sizeof(pFATBlock->name)] = 0;

            // skip MAPSORC.MPS section
            if(strcmp(tmpstr,"MAPSOURC") && strcmp(tmpstr,"SENDMAP2")) {

                gmapsupp_subfile_desc_t& subfile = subfiles[tmpstr];
                subfile.name = tmpstr;

                memcpy(tmpstr,pFATBlock->type,sizeof(pFATBlock->type));
                tmpstr[sizeof(pFATBlock->type)] = 0;

                gmapsupp_subfile_part_t& part = subfile.parts[tmpstr];
                part.size   = gar_load(uint32_t, pFATBlock->size);
                part.offset = gar_load(uint16_t, pFATBlock->blocks[0]) * blocksize;
                part.key    = tmpstr;
            }

        }

        dataoffset += sizeof(CGarminTile::FATblock_t);
        readFile(file, dataoffset, sizeof(CGarminTile::FATblock_t), FATblock, mask);
        pFATBlock = (const CGarminTile::FATblock_t * )FATblock.data();
    }

    if((dataoffset == sizeof(CGarminTile::hdr_img_t)) || (dataoffset >= (size_t)fsize)) {
        throw exce_t(errFormat,tr("Failed to read file structure: ") + t.filename);
    }

    // gmapsupp.img files do not have a data offset field
    if(gar_load(uint32_t, pImgHdr->dataoffset) == 0) {
        pImgHdr->dataoffset = gar_load(uint32_t, dataoffset);
    }

    // sometimes there are dummy blocks at the end of the FAT
    if(gar_load(uint32_t, pImgHdr->dataoffset) != dataoffset) {
        dataoffset = gar_load(uint32_t, pImgHdr->dataoffset);
    }

#ifdef DEBUG_SHOW_SECT_DESC
    {
        QMap<QString,gmapsupp_subfile_desc_t>::const_iterator subfile = subfiles.begin();
        while(subfile != subfiles.end()) {
            qDebug() << "--- subfile" << subfile->name << "---";

            const QList<gmapsupp_subfile_part_t>& parts = subfile->parts.values();
            QList<gmapsupp_subfile_part_t>::const_iterator part = parts.begin();
            while(part != parts.end()) {
                qDebug() << part->key << hex << part->offset << part->size;
                ++part;
            }
            ++subfile;
        }
    }
#endif                       //DEBUG_SHOW_SECT_DESC

}

void CGarminExport::slotStart()
{

    // create image header
    gmapsupp_imghdr_t gmapsupp_imghdr;
    initGmapsuppImgHdr(gmapsupp_imghdr);

    // first run. read file structure of all tiles
    QVector<tile_t>::iterator tile = tiles.begin();
    while(tile != tiles.end()){
        readTileInfo(*tile);

        ++tile;
    }
}

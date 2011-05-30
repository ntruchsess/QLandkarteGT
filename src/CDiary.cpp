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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

**********************************************************************************************/

#include "CDiary.h"
#include "CDiaryDB.h"
#include "CDiaryEditWidget.h"
#include "CTabWidget.h"

#include <QtCore>

struct diary_head_entry_t
{
    diary_head_entry_t() : type(CDiary::eEnd), offset(0) {}
    qint32      type;
    quint32     offset;
    QByteArray  data;
};

QDataStream& operator >>(QDataStream& s, CDiary& diary)
{
    QIODevice * dev = s.device();
    qint64      pos = dev->pos();

    char magic[9];
    s.readRawData(magic,9);

    if(strncmp(magic,"QLDry   ",9))
    {
        dev->seek(pos);
        return s;
    }

    QList<diary_head_entry_t> entries;

    while(1)
    {
        diary_head_entry_t entry;
        s >> entry.type >> entry.offset;
        entries << entry;
        if(entry.type == CDiary::eEnd) break;
    }

    QList<diary_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        qint64 o = pos + entry->offset;
        dev->seek(o);
        s >> entry->data;
        switch(entry->type)
        {
            case CDiary::eBase:
            {

                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> diary.timestamp;
                s1 >> diary.m_text;

                break;
            }

            default:;
        }

        ++entry;
    }

    if(!diary.editWidget.isNull())
    {
        diary.editWidget->setHtml(diary.m_text);
    }

    return s;
}


QDataStream& operator <<(QDataStream& s, CDiary& diary)
{
    QList<diary_head_entry_t> entries;

    if(!diary.editWidget.isNull())
    {
        diary.m_text = diary.editWidget->getHtml();
    }

    //---------------------------------------
    // prepare base data
    //---------------------------------------
    diary_head_entry_t entryBase;
    entryBase.type = CDiary::eBase;
    QDataStream s1(&entryBase.data, QIODevice::WriteOnly);
    s1.setVersion(QDataStream::Qt_4_5);

    s1 << diary.timestamp;
    s1 << diary.m_text;
    entries << entryBase;

    //---------------------------------------
    // prepare terminator
    //---------------------------------------
    diary_head_entry_t entryEnd;
    entryEnd.type = CDiary::eEnd;
    entries << entryEnd;

    //---------------------------------------
    //---------------------------------------
    // now start to actually write data;
    //---------------------------------------
    //---------------------------------------
    // write magic key
    s.writeRawData("QLDry   ",9);

    // calculate offset table
    quint32 offset = entries.count() * 8 + 9;

    QList<diary_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        entry->offset = offset;
        offset += entry->data.size() + sizeof(quint32);
        ++entry;
    }

    // write offset table
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->type << entry->offset;
        ++entry;
    }

    // write entry data
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->data;
        ++entry;
    }

    return s;
}


void operator >>(QFile& f, CDiary& diary)
{
    f.open(QIODevice::ReadOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s >> diary;
    f.close();
}


void operator <<(QFile& f, CDiary& diary)
{
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s << diary;
    f.close();
}


CDiary::CDiary(QObject * parent)
: IItem(parent)
, keyProjectGeoDB(0)
, editWidget(0)
{

}


CDiary::~CDiary()
{
    if(!editWidget.isNull()) delete editWidget;
}

void CDiary::slotEditWidgetDied(QObject*)
{    
    CDiaryDB::self().delDiary(getKey(), false);
}

QString CDiary::getInfo()
{
    QString str;

    str += "This is a diary";

    return str;
}

void CDiary::linkToProject(quint64 key)
{
    keyProjectGeoDB = key;
}


void CDiary::showEditWidget(CTabWidget * tab)
{
    if(editWidget == 0)
    {
        editWidget = new CDiaryEditWidget("", tab);
        editWidget->setHtml(m_text);
        connect(editWidget.data(), SIGNAL(destroyed(QObject*)), this, SLOT(slotEditWidgetDied(QObject*)));
    }
    tab->addTab(editWidget, tr("Diary"));
}

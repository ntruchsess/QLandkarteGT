/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#include "CGeoDB.h"
#include "config.h"
#include "WptIcons.h"

#include "CWptDB.h"
#include "CWpt.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CRouteDB.h"
#include "CRoute.h"
#include "COverlayDB.h"
#include "COverlayText.h"
#include "COverlayTextBox.h"
#include "COverlayDistance.h"
#include "CDlgSelGeoDBFolder.h"
#include "CResources.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "IMap.h"
#include "CDlgEditFolder.h"

#include "CQlb.h"

#include <QtGui>
#include <QSqlQuery>
#include <QSqlError>


#define QUERY_EXEC(cmd) \
if(!query.exec())\
{\
    qDebug() << query.lastQuery();\
    qDebug() << query.lastError();\
    cmd;\
}\

#define PROGRESS_SETUP(lbl, max) \
QProgressDialog progress(lbl, "Abort", 0, max, 0);\

#define PROGRESS(x, cmd) \
progress.setValue(x); \
if (progress.wasCanceled()) cmd; \
qApp->processEvents();\


class CGeoDBInternalEditLock
{
    public:
        CGeoDBInternalEditLock(CGeoDB * db) : db(db){db->isInternalEdit += 1;}
        ~CGeoDBInternalEditLock(){db->isInternalEdit -= 1;}
    private:
        CGeoDB * db;
};

CGeoDB::CGeoDB(QTabWidget * tb, QWidget * parent)
    : QWidget(parent)
    , tabbar(tb)
    , isInternalEdit(0)
{
    setupUi(this);
    setObjectName("GeoDB");

    tabbar->insertTab(0,this, QIcon(":/icons/iconGeoDB16x16"),"");
    tabbar->setTabToolTip(tabbar->indexOf(this), tr("Manage your Geo Data Base"));

    itemWorkspace = new QTreeWidgetItem(treeWorkspace);
    itemWorkspace->setData(eCoName, eUrType, eFolder0);
    itemWorkspace->setText(eCoName, tr("Workspace"));
    itemWorkspace->setIcon(eCoName, QIcon(":/icons/iconGlobe16x16"));
    itemWorkspace->setToolTip(eCoName, tr("All items you see on the map."));
    itemWorkspace->setFlags(itemWorkspace->flags() & ~Qt::ItemIsDragEnabled);

    itemWksWpt = new  QTreeWidgetItem(itemWorkspace);
    itemWksWpt->setData(eCoName, eUrType, eFolderT);
    itemWksWpt->setText(eCoName, tr("Waypoints"));
    itemWksWpt->setIcon(eCoName, QIcon(":/icons/iconWaypoint16x16"));
    itemWksWpt->setFlags(itemWksWpt->flags() & ~Qt::ItemIsDragEnabled);
    itemWksWpt->setHidden(true);

    itemWksTrk = new  QTreeWidgetItem(itemWorkspace);
    itemWksTrk->setData(eCoName, eUrType, eFolderT);
    itemWksTrk->setText(eCoName, tr("Tracks"));
    itemWksTrk->setIcon(eCoName, QIcon(":/icons/iconTrack16x16"));
    itemWksTrk->setFlags(itemWksTrk->flags() & ~Qt::ItemIsDragEnabled);
    itemWksTrk->setHidden(true);

    itemWksRte = new  QTreeWidgetItem(itemWorkspace);
    itemWksRte->setData(eCoName, eUrType, eFolderT);
    itemWksRte->setText(eCoName, tr("Routes"));
    itemWksRte->setIcon(eCoName, QIcon(":/icons/iconRoute16x16"));
    itemWksRte->setFlags(itemWksRte->flags() & ~Qt::ItemIsDragEnabled);
    itemWksRte->setHidden(true);

    itemWksOvl = new  QTreeWidgetItem(itemWorkspace);
    itemWksOvl->setData(eCoName, eUrType, eFolderT);
    itemWksOvl->setText(eCoName, tr("Overlays"));
    itemWksOvl->setIcon(eCoName, QIcon(":/icons/iconOverlay16x16"));
    itemWksOvl->setFlags(itemWksOvl->flags() & ~Qt::ItemIsDragEnabled);
    itemWksOvl->setHidden(true);

    itemLostFound = new QTreeWidgetItem(treeDatabase);
    itemLostFound->setData(eCoName, eUrType, eFolder0);
    itemLostFound->setText(eCoName, tr("Lost & Found"));
    itemLostFound->setIcon(eCoName, QIcon(":/icons/iconDelete16x16"));
    itemLostFound->setFlags(itemLostFound->flags() & ~Qt::ItemIsDragEnabled);
    itemLostFound->setToolTip(eCoName, tr("All items that lost their parent folder as you deleted it."));

    itemDatabase = new QTreeWidgetItem(treeDatabase);
    itemDatabase->setData(eCoName, eUrType, eFolder0);
    itemDatabase->setText(eCoName, tr("Database"));
    itemDatabase->setIcon(eCoName, QIcon(":/icons/iconGeoDB16x16"));
    itemDatabase->setData(eCoName, eUrDBKey, 1);
    itemDatabase->setFlags(itemDatabase->flags() & ~Qt::ItemIsDragEnabled);
    itemDatabase->setToolTip(eCoName, tr("All your data grouped by folders."));


    db = QSqlDatabase::addDatabase("QSQLITE","qlandkarte");
    db.setDatabaseName(CResources::self().pathGeoDB().absoluteFilePath("qlgt_new.db"));
    db.open();

    QSqlQuery query(db);

    if(!query.exec("PRAGMA locking_mode=EXCLUSIVE")) {
      return;
    }

    if(!query.exec("SELECT version FROM versioninfo"))
    {
        initDB();
    }
    else if(query.next())
    {
        int version = query.value(0).toInt();
        if(version != DB_VERSION)
        {
            migrateDB(version);
        }
    }
    else
    {
        initDB();
    }

    contextMenuFolder   = new QMenu(this);
    actEditDir          = contextMenuFolder->addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit"),this,SLOT(slotEditFolder()));
    actAddDir           = contextMenuFolder->addAction(QPixmap(":/icons/iconAdd16x16.png"),tr("New"),this,SLOT(slotAddFolder()));
    actDelDir           = contextMenuFolder->addAction(QPixmap(":/icons/iconDelete16x16.png"),tr("Delete"),this,SLOT(slotDelFolder()));
    actCopyDir          = contextMenuFolder->addAction(QPixmap(":/icons/editcopy"), tr("Copy"), this, SLOT(slotCopyFolder()));
    actMoveDir          = contextMenuFolder->addAction(QPixmap(":/icons/iconWptMove16x16"), tr("Move"), this, SLOT(slotMoveFolder()));

    contextMenuItem     = new QMenu(this);
    actCopyItem         = contextMenuItem->addAction(QPixmap(":/icons/editcopy"), tr("Copy"), this, SLOT(slotCopyItems()));
    actMoveItem         = contextMenuItem->addAction(QPixmap(":/icons/iconWptMove16x16"), tr("Move"), this, SLOT(slotMoveItems()));
    actDelItem          = contextMenuItem->addAction(QPixmap(":/icons/iconDelete16x16"), tr("Delete"), this, SLOT(slotDelItems()));

    contextMenuLost     = new QMenu(this);
    actMoveLost         = contextMenuLost->addAction(QPixmap(":/icons/iconWptMove16x16"), tr("Move"), this, SLOT(slotMoveLost()));
    actDelLost          = contextMenuLost->addAction(QPixmap(":/icons/iconDelete16x16"), tr("Delete"), this, SLOT(slotDelLost()));

    contextMenuWks      = new QMenu(this);
    actAddToDB          = contextMenuWks->addAction(QPixmap(":/icons/iconAdd16x16"), tr("Add to database"), this, SLOT(slotAddItems()));
    actSaveToDB         = contextMenuWks->addAction(QPixmap(":/icons/iconFileSave16x16"), tr("Save changes"), this, SLOT(slotSaveItems()));
    actHardCopy         = contextMenuWks->addAction(QPixmap(":/icons/editcopy"), tr("Check-out as copy"), this, SLOT(slotHardCopyItem()));


    connect(treeDatabase,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenuDatabase(const QPoint&)));
    connect(treeDatabase,SIGNAL(itemExpanded(QTreeWidgetItem *)),this,SLOT(slotItemExpanded(QTreeWidgetItem *)));
    connect(treeDatabase,SIGNAL(itemChanged(QTreeWidgetItem *, int)),this,SLOT(slotItemChanged(QTreeWidgetItem *, int)));

    connect(treeWorkspace,SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),this,SLOT(slotItemDoubleClicked(QTreeWidgetItem *, int)));
    connect(treeWorkspace,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenuWorkspace(const QPoint&)));

    connect(&CWptDB::self(), SIGNAL(sigChanged()), this, SLOT(slotWptDBChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigChanged()), this, SLOT(slotTrkDBChanged()));
    connect(&CRouteDB::self(), SIGNAL(sigChanged()), this, SLOT(slotRteDBChanged()));
    connect(&COverlayDB::self(), SIGNAL(sigChanged()), this, SLOT(slotOvlDBChanged()));

    connect(&CWptDB::self(), SIGNAL(sigModified(const QString&)), this, SLOT(slotModifiedWpt(const QString&)));
    connect(&CTrackDB::self(), SIGNAL(sigModified(const QString&)), this, SLOT(slotModifiedTrk(const QString&)));
    connect(&CRouteDB::self(), SIGNAL(sigModified(const QString&)), this, SLOT(slotModifiedRte(const QString&)));
    connect(&COverlayDB::self(), SIGNAL(sigModified(const QString&)), this, SLOT(slotModifiedOvl(const QString&)));

    connect(qApp, SIGNAL(aboutToQuit ()), this, SLOT(saveWorkspace()));

    // restore workspace
    loadWorkspace();
    itemWorkspace->setExpanded(true);
    // restore database widget
    initTreeWidget();
    itemDatabase->setExpanded(true);

    saveOnExit = CResources::self().saveGeoDBOnExit();
}

CGeoDB::~CGeoDB()
{

}

void CGeoDB::gainFocus()
{
    if(tabbar->currentWidget() != this)
    {
        tabbar->setCurrentWidget(this);
    }
}

void CGeoDB::initDB()
{
    qDebug() << "void CGeoDB::initDB()";
    QSqlQuery query(db);

    if(query.exec( "CREATE TABLE versioninfo ( version TEXT )"))
    {
        query.prepare( "INSERT INTO versioninfo (version) VALUES(:version)");
        query.bindValue(":version", DB_VERSION);
        QUERY_EXEC();
    }

    if(!query.exec( "CREATE TABLE folders ("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type           INTEGER,"
        "date           DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "icon           TEXT NOT NULL,"
        "name           TEXT NOT NULL,"
        "comment        TEXT"
    ")"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }


    if(!query.exec( "CREATE TABLE items ("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type           INTEGER,"
        "key            TEXT NOT NULL,"
        "date           DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "icon           TEXT NOT NULL,"
        "name           TEXT NOT NULL,"
        "comment        TEXT,"
        "data           BLOB NOT NULL"
    ")"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }

    if(!query.exec( "CREATE TABLE workspace ("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type           INTEGER NOT NULL,"
        "changed        BOOLEAN DEFAULT FALSE,"
        "data           BLOB NOT NULL,"
        "key            TEXT NOT NULL"
    ")"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }

    if(!query.exec("INSERT INTO folders (icon, name, comment) VALUES ('', 'database', '')"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }

    if(!query.exec( "CREATE TABLE folder2folder ("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent         INTEGER NOT NULL,"
        "child          INTEGER NOT NULL,"
        "FOREIGN KEY(parent) REFERENCES folders(id),"
        "FOREIGN KEY(child) REFERENCES folders(id)"
    ")"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }

    if(!query.exec( "CREATE TABLE folder2item ("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent         INTEGER NOT NULL,"
        "child          INTEGER NOT NULL,"
        "FOREIGN KEY(parent) REFERENCES folders(id),"
        "FOREIGN KEY(child) REFERENCES items(id)"
    ")"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }

}

void CGeoDB::migrateDB(int version)
{
    qDebug() << "void CGeoDB::migrateDB(int version)" << version;
    QSqlQuery query(db);



    for(version++; version <= DB_VERSION; version++)
    {

        switch(version)
        {
            case 1:
                break;

            case 2:
            {
                if(!query.exec( "CREATE TABLE workspace ("
                    "id             INTEGER PRIMARY KEY NOT NULL,"
                    "changed        BOOLEAN DEFAULT FALSE,"
                    "data           BLOB NOT NULL"
                ")"))
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }

                break;
            }

            case 3:
            {
                if(!query.exec("ALTER TABLE workspace ADD COLUMN key TEXT"))
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }
                if(!query.exec("ALTER TABLE workspace ADD COLUMN type INTEGER"))
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }
                break;
            }
            case 4:
            {
                if(!query.exec("ALTER TABLE folders ADD COLUMN type INTEGER"))
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }

                query.prepare("UPDATE folders SET type=:type WHERE icon=:icon");
                query.bindValue("type", eFolder1);
                query.bindValue("icon", ":/icons/iconFolderBlue16x16");
                if(!query.exec())
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }

                query.prepare("UPDATE folders SET type=:type WHERE icon=:icon");
                query.bindValue(":type", eFolder2);
                query.bindValue(":icon", ":/icons/iconFolderGreen16x16");
                if(!query.exec())
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }

                break;
            }
            case 5:
            {
                QSqlQuery query2(db);

                if(!query.exec("SELECT id, type, icon FROM items"))
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }

                quint32 progCnt = 0;
                PROGRESS_SETUP(tr("Mirgrating database from version 4 to 5."), query.size());

                while(query.next())
                {
                    QPixmap pixmap;

                    if(query.value(1).toInt() == eWpt || query.value(1).toInt() == eRte)
                    {
                        pixmap = getWptIconByName(query.value(2).toString());
                    }
                    else if(query.value(1).toInt() == eTrk)
                    {
                        pixmap = QPixmap(16,16);
                        pixmap.fill(query.value(2).toString());
                    }
                    else
                    {
                        pixmap = QPixmap(query.value(2).toString());
                    }

                    QByteArray bytes;
                    QBuffer buffer(&bytes);
                    buffer.open(QIODevice::WriteOnly);
                    pixmap.save(&buffer, "XPM");

                    query2.prepare("UPDATE items SET icon=:icon WHERE id=:id");
                    query2.bindValue(":id", query.value(0).toULongLong());
                    query2.bindValue(":icon", bytes);
                    if(!query2.exec())
                    {
                        qDebug() << query.lastQuery();
                        qDebug() << query.lastError();
                        return;
                    }

                    PROGRESS(progCnt++, continue);
                }
                break;
            }
            case 6:
            {
                QSqlQuery query2(db);

                if(!query.exec("SELECT id, data, type FROM items"))
                {
                    qDebug() << query.lastQuery();
                    qDebug() << query.lastError();
                    return;
                }

                quint32 progCnt = 0;
                PROGRESS_SETUP(tr("Mirgrating database from version 5 to 6."), query.size());

                while(query.next())
                {

                    QByteArray array = query.value(1).toByteArray();
                    QBuffer buffer(&array);
                    CQlb qlb(this);
                    qlb.load(&buffer);

                    switch(query.value(2).toInt())
                    {
                    case eWpt:
                        array = qlb.waypoints();
                        break;

                    case eTrk:
                        array = qlb.tracks();
                        break;

                    case eRte:
                        array = qlb.routes();
                        break;

                    case eOvl:
                        array = qlb.overlays();
                        break;
                    }

                    query2.prepare("UPDATE items SET data=:data WHERE id=:id");
                    query2.bindValue(":data", array);
                    query2.bindValue(":id", query.value(0));

                    if(!query2.exec())
                    {
                        qDebug() << query.lastQuery();
                        qDebug() << query.lastError();
                        return;
                    }

                    PROGRESS(progCnt++, continue);
                }

            }
        }
    }
    query.prepare( "UPDATE versioninfo set version=:version");
    query.bindValue(":version", version - 1);
    QUERY_EXEC();
}


bool sortItemsLessThan(QTreeWidgetItem * item1, QTreeWidgetItem * item2)
{
    int type1 = item1->data(CGeoDB::eCoName, CGeoDB::eUrType).toInt();
    int type2 = item2->data(CGeoDB::eCoName, CGeoDB::eUrType).toInt();

    // yeah, a bit peculiar, but the only way to weasle out of the selfmade enum mess
    if(type1 >= CGeoDB::eFolder0) type1 -= CGeoDB::eFolderN;
    if(type2 >= CGeoDB::eFolder0) type2 -= CGeoDB::eFolderN;

    if(type1 != type2)
    {
        return type1 < type2;
    }
    else
    {
        return item1->text(CGeoDB::eCoName) < item2->text(CGeoDB::eCoName);
    }

    return true;
}

void CGeoDB::sortItems(QTreeWidgetItem * item)
{
    CGeoDBInternalEditLock lock(this);
    QList<QTreeWidgetItem*> items = item->takeChildren();

    qSort(items.begin(), items.end(), sortItemsLessThan);

    item->addChildren(items);
}

void CGeoDB::loadWorkspace()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    query.prepare("SELECT changed, type, key, data FROM workspace");
    QUERY_EXEC(return);

    CQlb qlb(this);
    QByteArray& wpts = qlb.waypoints();
    QByteArray& trks = qlb.tracks();
    QByteArray& rtes = qlb.routes();
    QByteArray& ovls = qlb.overlays();

    quint32 progCnt = 0;
    PROGRESS_SETUP(tr("Loading workspace. Please wait."), query.size());

    while(query.next())
    {
        PROGRESS(progCnt++, return);

        switch(query.value(1).toInt())
        {
            case eWpt:
                wpts += query.value(3).toByteArray();
                if(query.value(0).toBool()){
                    keysWptModified << query.value(2).toString();
                }
                break;
            case eTrk:
                trks += query.value(3).toByteArray();
                if(query.value(0).toBool()){
                    keysTrkModified << query.value(2).toString();
                }
                break;
            case eRte:
                rtes += query.value(3).toByteArray();
                if(query.value(0).toBool()){
                    keysRteModified << query.value(2).toString();
                }
                break;
            case eOvl:
                ovls += query.value(3).toByteArray();
                if(query.value(0).toBool()){
                    keysOvlModified << query.value(2).toString();
                }
                break;
        }

    }

    CWptDB::self().loadQLB(qlb, false);
    CTrackDB::self().loadQLB(qlb, false);
    CRouteDB::self().loadQLB(qlb, false);
    COverlayDB::self().loadQLB(qlb, false);

    changedWorkspace();
}


void CGeoDB::updateModifyMarker()
{
    // reset modify marker in text label
    itemWorkspace->setText(eCoName, tr("Workspace"));
    updateModifyMarker(itemWksWpt, keysWptModified, tr("Waypoints"));
    updateModifyMarker(itemWksTrk, keysTrkModified, tr("Tracks"));
    updateModifyMarker(itemWksRte, keysRteModified, tr("Routes"));
    updateModifyMarker(itemWksOvl, keysOvlModified, tr("Overlays"));

}

void CGeoDB::updateModifyMarker(QTreeWidgetItem * itemWks, QSet<QString>& keys, const QString& label)
{    
    QTreeWidgetItem * item;
    bool modified = false;

    if(!keys.isEmpty())
    {
        // iterate over all children and test agains the keys of known modified items
        const int size = itemWks->childCount();
        for(int i = 0; i < size; i++)
        {
            item = itemWks->child(i);
            if(keys.contains(item->data(eCoName, eUrQLKey).toString()))
            {
                item->setText(eCoState,"*");
                modified = true;
            }
        }

        // if there are modified elements update the global folders, too
        if(modified)
        {
            itemWorkspace->setText(eCoName, tr("Workspace") + " *");
            itemWks->setText(eCoName, label + " *");
        }
        else
        {
            itemWks->setText(eCoName, label);
        }
    }
    else
    {
        itemWks->setText(eCoName, label);
    }

}

void CGeoDB::updateDatabaseMarker()
{
    updateDatabaseMarker(itemWksWpt, keysWksWpt);
    updateDatabaseMarker(itemWksTrk, keysWksTrk);
    updateDatabaseMarker(itemWksRte, keysWksRte);
    updateDatabaseMarker(itemWksOvl, keysWksOvl);
}

void CGeoDB::updateDatabaseMarker(QTreeWidgetItem * itemWks, QSet<quint64> &keysWks)
{
    QSqlQuery query(db);
    QTreeWidgetItem * item;

    keysWks.clear();

    const int size = itemWks->childCount();

    PROGRESS_SETUP(tr("Update workspace."), size);

    for(int i = 0; i < size; i++)
    {
        PROGRESS(i, break);

        item = itemWks->child(i);

        query.prepare("SELECT id FROM items WHERE key=:key");
        query.bindValue(":key", item->data(eCoName, eUrQLKey));
        QUERY_EXEC(continue);

        if(query.next())
        {
            item->setData(eCoName, eUrDBKey, query.value(0));
            item->setIcon(eCoState, QIcon(":/icons/iconGeoDB16x16"));
            keysWks << query.value(0).toULongLong();
        }
        else
        {
            item->setData(eCoName, eUrDBKey, 0);
            item->setIcon(eCoState, QIcon());
        }
    }
}

void CGeoDB::changedWorkspace()
{
    updateDatabaseMarker();
    updateModifyMarker();
    updateCheckmarks();

    treeWorkspace->header()->setResizeMode(eCoName,QHeaderView::ResizeToContents);
    treeWorkspace->header()->setResizeMode(eCoState,QHeaderView::ResizeToContents);

    CGeoDBInternalEditLock lock(this);
    treeDatabase->header()->setResizeMode(eCoName,QHeaderView::ResizeToContents);
    treeDatabase->header()->setResizeMode(eCoState,QHeaderView::ResizeToContents);
}

void CGeoDB::initTreeWidget()
{
    CGeoDBInternalEditLock lock(this);

    queryChildrenFromDB(itemDatabase, 2);
    updateLostFound();
    updateCheckmarks();
}


void CGeoDB::queryChildrenFromDB(QTreeWidgetItem * parent, int levels)
{
    CGeoDBInternalEditLock lock(this);

    QSqlQuery query(db);
    const quint64 parentId = parent->data(eCoName, eUrDBKey).toULongLong();

    if(parentId == 0)
    {
        return;
    }

    // folders 1st
    query.prepare("SELECT t1.child FROM folder2folder AS t1, folders AS t2 WHERE t1.parent = :id AND t2.id = t1.child ORDER BY t2.name");
    query.bindValue(":id", parentId);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 childId = query.value(0).toULongLong();

        // get child folder's properties
        QSqlQuery query2(db);
        query2.prepare("SELECT icon, name, comment, type FROM folders WHERE id = :id ORDER BY name");
        query2.bindValue(":id", childId);
        if(!query2.exec())
        {
            qDebug() << query2.lastQuery();
            qDebug() << query2.lastError();
            continue;
        }
        query2.next();

        // add the treewidget item
        QTreeWidgetItem * item = new QTreeWidgetItem(parent);
        item->setData(eCoName, eUrType, query2.value(3).toInt());
        item->setData(eCoName, eUrDBKey, childId);
        item->setIcon(eCoName, QIcon(query2.value(0).toString()));
        item->setText(eCoName, query2.value(1).toString());
        item->setToolTip(eCoName, query2.value(2).toString());
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        // all types larger than eFolder1 are checkable (eFolder2, eFolderN)
        if(query2.value(3).toInt() > eFolder1)
        {
            item->setCheckState(eCoState, Qt::Unchecked);
        }

        // query next level
        if(levels)
        {
            queryChildrenFromDB(item, levels - 1);
        }
    }

    // items 2nd
    query.prepare("SELECT t1.child FROM folder2item AS t1, items AS t2 WHERE t1.parent = :id AND t2.id = t1.child ORDER BY t2.type, t2.name");
    query.bindValue(":id", parentId);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 childId = query.value(0).toULongLong();

        QSqlQuery query2(db);
        query2.prepare("SELECT type, icon, name, comment FROM items WHERE id = :id");
        query2.bindValue(":id", childId);
        if(!query2.exec())
        {
            qDebug() << query2.lastQuery();
            qDebug() << query2.lastError();
            continue;
        }
        query2.next();

        QPixmap pixmap;
        pixmap.loadFromData(query2.value(1).toByteArray());

        QTreeWidgetItem * item = new QTreeWidgetItem(parent);
        item->setData(eCoName, eUrType, query2.value(0).toInt());
        item->setData(eCoName, eUrDBKey, childId);
        item->setIcon(eCoName, pixmap);
        item->setText(eCoName, query2.value(2).toString());
        item->setToolTip(eCoName, query2.value(3).toString());
        item->setCheckState(eCoState, Qt::Unchecked);
    }

    treeDatabase->header()->setResizeMode(eCoName,QHeaderView::ResizeToContents);
    treeDatabase->header()->setResizeMode(eCoState,QHeaderView::ResizeToContents);
}

void CGeoDB::updateLostFound()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    qDeleteAll(itemLostFound->takeChildren());

    query.prepare("SELECT type, id, icon, name, comment FROM items AS t1 WHERE NOT EXISTS(SELECT * FROM folder2item WHERE child=t1.id) ORDER BY t1.name");
    QUERY_EXEC(return);
    QList<QTreeWidgetItem*> items;
    while(query.next())
    {
        QPixmap pixmap;
        QTreeWidgetItem * item = new QTreeWidgetItem();

        pixmap.loadFromData(query.value(2).toByteArray());

        item->setData(eCoName, eUrType, query.value(0).toInt());
        item->setData(eCoName, eUrDBKey, query.value(1).toULongLong());
        item->setIcon(eCoName, pixmap);
        item->setText(eCoName, query.value(3).toString());
        item->setToolTip(eCoName, query.value(4).toString());
        item->setCheckState(eCoState, Qt::Unchecked);

        items << item;
    }
    if(items.size())
    {
        itemLostFound->setText(eCoName, tr("Lost & Found (%1)").arg(items.size()));
    }
    else
    {
        itemLostFound->setText(eCoName, tr("Lost & Found"));
    }

    itemLostFound->addChildren(items);

}

void CGeoDB::updateCheckmarks()
{
    CGeoDBInternalEditLock lock(this);
    updateCheckmarks(itemDatabase);
    updateCheckmarks(itemLostFound);
}

void CGeoDB::updateCheckmarks(QTreeWidgetItem * parent)
{
    CGeoDBInternalEditLock lock(this);
    QTreeWidgetItem * item;
    quint64 id;
    bool selected, selectedAll;


    const int size  = parent->childCount();
    selectedAll     = size != 0;
    for(int i = 0; i < size; i++)
    {
        item    = parent->child(i);

        if(item->data(eCoName, eUrType).toInt() >= eFolder0)
        {
            updateCheckmarks(item);
            if(item->checkState(eCoState) == Qt::Unchecked)
            {
                if(item->data(eCoName, eUrType).toInt() == eFolder2)
                {
                    selectedAll = false;
                }
            }
            continue;
        }

        id = item->data(eCoName, eUrDBKey).toULongLong();
        if(keysWksWpt.contains(id))
        {
            selected = true;
        }
        else if(keysWksTrk.contains(id))
        {
            selected = true;
        }
        else if(keysWksRte.contains(id))
        {
            selected = true;
        }
        else if(keysWksOvl.contains(id))
        {
            selected = true;
        }
        else
        {
            selected    = false;
            selectedAll = false;
        }

        item->setCheckState(eCoState, selected ? Qt::Checked : Qt::Unchecked);

    }


    if(parent->data(eCoName, eUrType).toInt() > eFolder1)
    {
        parent->setCheckState(eCoState, selectedAll ? Qt::Checked : Qt::Unchecked);
    }

}

void CGeoDB::updateFolderById(quint64 id)
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->findItems("*", Qt::MatchWildcard|Qt::MatchRecursive, eCoName);

    QSqlQuery query(db);
    query.prepare("SELECT icon, name, comment, type FROM folders WHERE id=:id");
    query.bindValue(":id", id);
    QUERY_EXEC();
    query.next();

    foreach(item, items)
    {
        // skip all non folder items
        if(item->data(eCoName, eUrType).toInt() < eFolder0)
        {
            continue;
        }

        if(item->data(eCoName, eUrDBKey).toULongLong() == id)
        {
            item->setToolTip(eCoName, query.value(2).toString());
            item->setIcon(eCoName, QIcon(query.value(0).toString()));
            item->setText(eCoName, query.value(1).toString());
            item->setData(eCoName, eUrType, query.value(3).toInt());
        }
    }
}

void CGeoDB::addChildrenToWks(quint64 parentId)
{
    QSqlQuery query(db);

    query.prepare("SELECT t1.data, t1.type FROM items AS t1, folder2item AS t2 WHERE t2.parent=:parent AND t1.id = t2.child");
    query.bindValue(":parent", parentId);
    QUERY_EXEC(return);

    quint32 progCnt = 0;
    PROGRESS_SETUP(tr("Loading items from database."), query.size());

    CQlb qlb(this);

    while(query.next())
    {
        PROGRESS(progCnt++, break);

        switch(query.value(1).toInt())
        {
            case eWpt:
                qlb.waypoints() += query.value(0).toByteArray();
                break;
            case eTrk:
                qlb.tracks() += query.value(0).toByteArray();
                break;
            case eRte:
                qlb.routes() += query.value(0).toByteArray();
                break;
            case eOvl:
                qlb.overlays() += query.value(0).toByteArray();
                break;
        }
    }

    CWptDB::self().loadQLB(qlb, false);
    CTrackDB::self().loadQLB(qlb, false);
    CRouteDB::self().loadQLB(qlb, false);
    COverlayDB::self().loadQLB(qlb, false);


    query.prepare("SELECT t1.child FROM folder2folder AS t1, folders AS t2 WHERE t1.parent=:parent AND t1.child=t2.id AND t2.type=:type");
    query.bindValue(":parent", parentId);
    query.bindValue(":type", eFolder2);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 childId = query.value(0).toULongLong();
        addChildrenToWks(childId);
    }
}

void CGeoDB::delChildrenFromWks(quint64 parentId)
{
    QStringList keysWpt;
    QStringList keysTrk;
    QStringList keysRte;
    QStringList keysOvl;

    QSqlQuery query(db);

    query.prepare("SELECT t1.key, t1.type FROM items AS t1, folder2item AS t2 WHERE t2.parent=:parent AND t1.id = t2.child");
    query.bindValue(":parent", parentId);
    QUERY_EXEC(return);
    while(query.next())
    {

        QString key = query.value(0).toString();
        int type    = query.value(1).toInt();
        switch(type)
        {
            case eWpt:
                keysWpt << key;
                keysWptModified.remove(key);
                break;
            case eTrk:
                keysTrk << key;
                keysTrkModified.remove(key);
                break;
            case eRte:
                keysRte << key;
                keysRteModified.remove(key);
                break;
            case eOvl:
                keysOvl << key;
                keysOvlModified.remove(key);
                break;
        }
    }

    CWptDB::self().delWpt(keysWpt);
    CTrackDB::self().delTracks(keysTrk);
    CRouteDB::self().delRoutes(keysRte);
    COverlayDB::self().delOverlays(keysOvl);

    query.prepare("SELECT child FROM folder2folder WHERE parent=:parent");
    query.bindValue(":parent", parentId);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 childId = query.value(0).toULongLong();
        delChildrenFromWks(childId);
    }
}

void CGeoDB::addFolder(QTreeWidgetItem * parent, const QString& name, const QString& comment, qint32 type)
{
    CGeoDBInternalEditLock lock(this);

    QSqlQuery query(db);
    quint64 parentId = parent->data(eCoName, eUrDBKey).toULongLong();
    quint64 childId = 0;

    query.prepare("INSERT INTO folders (icon, name, comment, type) VALUES (:icon, :name, :comment, :type)");

    QString icon;
    switch(type)
    {
    case eFolder1:
        icon = ":/icons/iconFolderBlue16x16";
        break;
    case eFolder2:
        icon = ":/icons/iconFolderGreen16x16";
        break;
    case eFolderN:
        icon = ":/icons/iconFolderOrange16x16";
        break;
    }

    query.bindValue(":icon", icon);
    query.bindValue(":name", name);
    query.bindValue(":comment", comment);
    query.bindValue(":type", type);
    QUERY_EXEC(return);
    if(!query.exec("SELECT last_insert_rowid() from folders"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
        return;
    }
    query.next();
    childId = query.value(0).toULongLong();
    if(childId == 0)
    {
        qDebug() << "childId equals 0. bad.";
        return;
    }

    query.prepare("INSERT INTO folder2folder (parent, child) VALUES (:parent, :child)");
    query.bindValue(":parent", parentId);
    query.bindValue(":child", childId);
    QUERY_EXEC(return);

    QTreeWidgetItem * item = new QTreeWidgetItem();
    item->setData(eCoName, eUrType, type);
    item->setData(eCoName, eUrDBKey, childId);
    item->setIcon(eCoName, QIcon(icon));
    item->setText(eCoName, name);
    item->setToolTip(eCoName, comment);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    if(type > eFolder1)
    {
        item->setCheckState(eCoState, Qt::Unchecked);
    }

    addFolderById(parentId, item);

    // delete item as it will be cloned by addFolderById() and never used directly
    delete item;
}

void CGeoDB::addFolderById(quint64 parentId, QTreeWidgetItem * child)
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->findItems("*", Qt::MatchWildcard|Qt::MatchRecursive, eCoName);

    foreach(item, items)
    {

        if(item->data(eCoName, eUrType).toInt() < eFolder0)
        {
            // if item is not a folder
            continue;
        }

        if(item->data(eCoName, eUrDBKey).toULongLong() == parentId)
        {
            QTreeWidgetItem * clone = new QTreeWidgetItem();
            clone->setData(eCoName, eUrType, child->data(eCoName, eUrType));
            clone->setIcon(eCoName, child->icon(eCoName));
            clone->setData(eCoName, eUrDBKey, child->data(eCoName, eUrDBKey));
            clone->setData(eCoName, eUrQLKey, child->data(eCoName, eUrQLKey));
            clone->setText(eCoName, child->text(eCoName));
            clone->setToolTip(eCoName, child->toolTip(eCoName));
            clone->setFlags(child->flags());


            if(child->data(eCoName, eUrType).toInt() > eFolder1)
            {
                clone->setCheckState(eCoState, Qt::Unchecked);
            }

            item->insertChild(0,clone);

            queryChildrenFromDB(clone,1);
            updateCheckmarks(clone);

            sortItems(item);
        }
    }
}

void CGeoDB::delFolder(QTreeWidgetItem * item, bool isTopLevel)
{
    CGeoDBInternalEditLock lock(this);

    if(item->data(eCoName, eUrType).toInt() < eFolder0)
    {
        return;
    }

    int i;
    const int size   = item->childCount();
    quint64 itemId   = item->data(eCoName, eUrDBKey).toULongLong();
    quint64 parentId = item->parent()->data(eCoName, eUrDBKey).toULongLong();

    QSqlQuery query(db);
    // delete this particular relation first
    query.prepare("DELETE FROM folder2folder WHERE parent=:parent AND child=:child");
    query.bindValue(":parent", parentId);
    query.bindValue(":child", itemId);
    QUERY_EXEC();

    // next query if the item is used as child in any other relation
    query.prepare("SELECT * FROM folder2folder WHERE child=:id");
    query.bindValue(":id", itemId);
    QUERY_EXEC();
    // if there is no other relation delete the children, too.
    if(!query.next())
    {
        for(i = 0; i < size; i++)
        {
            delFolder(item->child(i), false);
        }

        // remove the child items relations
        query.prepare("DELETE FROM folder2item WHERE parent=:id");
        query.bindValue(":id", itemId);
        QUERY_EXEC();

        // and remove the folder
        query.prepare("DELETE FROM folders WHERE id=:id");
        query.bindValue(":id", itemId);
        QUERY_EXEC();
    }

    if(isTopLevel)
    {
        delFolderById(parentId, itemId);
    }

}

void CGeoDB::delFolderById(quint64 parentId, quint64 childId)
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->findItems("*", Qt::MatchWildcard|Qt::MatchRecursive, eCoName);
    QList<QTreeWidgetItem*> itemsToDelete;

    foreach(item, items)
    {
        if(item->data(eCoName, eUrType).toInt() < eFolder0)
        {
            continue;
        }

        if(item->data(eCoName, eUrDBKey).toULongLong() == parentId)
        {
            int i;
            const int size = item->childCount();
            for(i = 0; i < size; i++)
            {
                if(item->child(i)->data(eCoName, eUrType).toInt() < eFolder0)
                {
                    continue;
                }
                if(item->child(i)->data(eCoName, eUrDBKey).toULongLong() == childId)
                {
                    // just collect the items, do not delete now to prevent crash
                    itemsToDelete << item->takeChild(i);
                    break;
                }
            }
        }
    }

    // now it's save to delete all items
    qDeleteAll(itemsToDelete);
}


void CGeoDB::addItemById(quint64 parentId, QTreeWidgetItem * child)
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->findItems("*", Qt::MatchWildcard|Qt::MatchRecursive, eCoName);

    foreach(item, items)
    {

        if(item->data(eCoName, eUrType).toInt() <= eFolder0)
        {
            // if item is not a folder
            continue;
        }

        if(item->data(eCoName, eUrDBKey).toULongLong() == parentId)
        {
            QTreeWidgetItem * clone = new QTreeWidgetItem();
            clone->setData(eCoName, eUrType, child->data(eCoName, eUrType));
            clone->setIcon(eCoName, child->icon(eCoName));
            clone->setData(eCoName, eUrDBKey, child->data(eCoName, eUrDBKey));
            clone->setData(eCoName, eUrQLKey, child->data(eCoName, eUrQLKey));
            clone->setText(eCoName, child->text(eCoName));
            clone->setToolTip(eCoName, child->toolTip(eCoName));
            clone->setFlags(child->flags());
            clone->setCheckState(eCoState, child->checkState(eCoState));

            item->insertChild(0,clone);

            sortItems(item);
        }
    }
}

void CGeoDB::delItemById(quint64 parentId, quint64 childId)
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->findItems("*", Qt::MatchWildcard|Qt::MatchRecursive, eCoName);
    QList<QTreeWidgetItem*> itemsToDelete;

    foreach(item, items)
    {
        if(item->data(eCoName, eUrType).toInt() < eFolder0)
        {
            continue;
        }
        if(item->data(eCoName, eUrDBKey).toULongLong() == parentId)
        {
            int i;
            const int size = item->childCount();
            for(i = 0; i < size; i++)
            {
                if(item->child(i)->data(eCoName, eUrType).toInt() >= eFolder0)
                {
                    continue;
                }
                if(item->child(i)->data(eCoName, eUrDBKey).toULongLong() == childId)
                {
                    // just collect the items, do not delete now to prevent crash
                    itemsToDelete << item->takeChild(i);
                    break;
                }
            }
        }
    }

    // now it's save to delete all items
    qDeleteAll(itemsToDelete);
}

void CGeoDB::addItemToDB(quint64 parentId, QTreeWidgetItem * item)
{
    quint64 childId;
    QSqlQuery query(db);

    // test for item with qlandkarte key
    QString key = item->data(eCoName, eUrQLKey).toString();
    query.prepare("SELECT id FROM items WHERE key=:key");
    query.bindValue(":key", key);
    QUERY_EXEC();

    if(query.next())
    {
        childId = query.value(0).toULongLong();
    }
    else
    {
        // insert item
        IItem * qlItem = 0;
        QString key  = item->data(eCoName, eUrQLKey).toString();
        qint32  type = item->data(eCoName, eUrType).toInt();

        QByteArray icon;
        QBuffer buffer(&icon);
        buffer.open(QIODevice::WriteOnly);

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);

        switch(type)
        {
            case eWpt:
            {
                CWpt * wpt = CWptDB::self().getWptByKey(key);
                stream << *wpt;
                qlItem = wpt;
                break;
            }
            case eTrk:
            {
                CTrack * trk = CTrackDB::self().getTrackByKey(key);
                stream << *trk;
                qlItem = trk;
                break;
            }
            case eRte:
            {
                CRoute * rte = CRouteDB::self().getRouteByKey(key);
                stream << *rte;
                qlItem = rte;
                break;
            }
            case eOvl:
            {
                IOverlay * ovl = COverlayDB::self().getOverlayByKey(key);
                stream << *ovl;
                qlItem = ovl;
                break;
            }
        }

        QPixmap pixmap = qlItem->getIcon();
        pixmap.save(&buffer, "XPM");

        // add item to database
        query.prepare("INSERT INTO items (type, key, date, icon, name, comment, data) "
                      "VALUES (:type, :key, :date, :icon, :name, :comment, :data)");

        query.bindValue(":type", type);
        query.bindValue(":key", qlItem->getKey());
        query.bindValue(":date", QDateTime::fromTime_t(qlItem->getTimestamp()).toString("yyyy-MM-dd hh-mm-ss"));
        query.bindValue(":icon", icon);
        query.bindValue(":name", qlItem->getName());
        query.bindValue(":comment", qlItem->getComment());
        query.bindValue(":data", data);
        QUERY_EXEC();

        if(!query.exec("SELECT last_insert_rowid() from items"))
        {
            qDebug() << query.lastQuery();
            qDebug() << query.lastError();
            return;
        }
        query.next();
        childId = query.value(0).toULongLong();
        if(childId == 0)
        {
            qDebug() << "childId equals 0. bad.";
            return;
        }
    }
    item->setData(eCoName, eUrDBKey, childId);

    // create link folder <-> item
    query.prepare("SELECT id FROM folder2item WHERE parent=:parent AND child=:child");
    query.bindValue(":parent", parentId);
    query.bindValue(":child", childId);
    QUERY_EXEC();

    if(!query.next() && parentId != 0)
    {
        query.prepare("INSERT INTO folder2item (parent, child) VALUES (:parent, :child)");
        query.bindValue(":parent", parentId);
        query.bindValue(":child", childId);
        QUERY_EXEC(return);
        // update tree widget
        addItemById(parentId, item);
    }
}


// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
//                       all slots
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
void CGeoDB::saveWorkspace()
{
    int i, size;
    QSqlQuery query(db);
    QTreeWidgetItem * item;


    if(!query.exec("DELETE FROM workspace"))
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
    }

    if(!saveOnExit)
    {
        return;
    }

    quint32 total, progCnt = 0;
    total = itemWksWpt->childCount() + itemWksTrk->childCount() + itemWksRte->childCount() + itemWksOvl->childCount();
    PROGRESS_SETUP(tr("Saving workspace. Please wait."), total);

    size = itemWksWpt->childCount();
    for(i=0; i<size; i++)
    {
        PROGRESS(progCnt++, return);

        item        = itemWksWpt->child(i);
        CWpt * wpt  = CWptDB::self().getWptByKey(item->data(eCoName, eUrQLKey).toString());

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_4_5);
        stream << *wpt;

        query.prepare("INSERT INTO workspace (type, key, changed, data) VALUES (:type, :key, :changed, :data)");
        query.bindValue(":changed", item->text(eCoState) == "*");
        query.bindValue(":type", eWpt);
        query.bindValue(":key", wpt->getKey());
        query.bindValue(":data", data);
        QUERY_EXEC(continue);
    }

    size = itemWksTrk->childCount();
    for(i=0; i<size; i++)
    {
        PROGRESS(progCnt++, return);

        item         = itemWksTrk->child(i);
        CTrack * trk = CTrackDB::self().getTrackByKey(item->data(eCoName, eUrQLKey).toString());

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_4_5);
        stream << *trk;

        query.prepare("INSERT INTO workspace (type, key, changed, data) VALUES (:type, :key, :changed, :data)");
        query.bindValue(":changed", item->text(eCoState) == "*");
        query.bindValue(":type", eTrk);
        query.bindValue(":key", trk->getKey());
        query.bindValue(":data", data);
        QUERY_EXEC(continue);
    }

    size = itemWksRte->childCount();
    for(i=0; i<size; i++)
    {
        PROGRESS(progCnt++, return);

        item = itemWksRte->child(i);
        CRoute * rte = CRouteDB::self().getRouteByKey(item->data(eCoName, eUrQLKey).toString());
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_4_5);
        stream << *rte;

        query.prepare("INSERT INTO workspace (type, key, changed, data) VALUES (:type, :key, :changed, :data)");
        query.bindValue(":changed", item->text(eCoState) == "*");
        query.bindValue(":type", eRte);
        query.bindValue(":key", rte->getKey());
        query.bindValue(":data", data);
        QUERY_EXEC(continue);
    }

    size = itemWksOvl->childCount();
    for(i=0; i<size; i++)
    {
        PROGRESS(progCnt++, return);

        item = itemWksOvl->child(i);
        IOverlay * ovl = COverlayDB::self().getOverlayByKey(item->data(eCoName, eUrQLKey).toString());

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_4_5);
        stream << *ovl;

        query.prepare("INSERT INTO workspace (type, key, changed, data) VALUES (:type, :key, :changed, :data)");
        query.bindValue(":changed", item->text(eCoState) == "*");
        query.bindValue(":type", eOvl);
        query.bindValue(":key", ovl->getKey());
        query.bindValue(":data", data);
        QUERY_EXEC(continue);
    }
}


void CGeoDB::slotWptDBChanged()
{
    CGeoDBInternalEditLock lock(this);

    qDeleteAll(itemWksWpt->takeChildren());

    CWptDB& wptdb = CWptDB::self();
    CWptDB::keys_t key;
    QList<CWptDB::keys_t> keys = wptdb.keys();

    QList<QTreeWidgetItem*> items;

    foreach(key, keys)
    {

        CWpt * wpt = wptdb.getWptByKey(key.key);
        if(!wpt || wpt->sticky)
        {
            continue;
        }

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setData(eCoName, eUrType, eWpt);
        item->setData(eCoName, eUrQLKey, wpt->getKey());
        item->setIcon(eCoName, wpt->getIcon());
        item->setText(eCoName, wpt->getName());
        item->setToolTip(eCoName, wpt->getInfo());

        items << item;        
    }

    itemWksWpt->addChildren(items);
    itemWksWpt->setHidden(itemWksWpt->childCount() == 0);

    if(!isInternalEdit)
    {
        changedWorkspace();
    }
}

void CGeoDB::slotTrkDBChanged()
{
    CGeoDBInternalEditLock lock(this);

    qDeleteAll(itemWksTrk->takeChildren());

    CTrackDB& trkdb = CTrackDB::self();
    CTrackDB::keys_t key;
    QList<CTrackDB::keys_t> keys = trkdb.keys();

    QList<QTreeWidgetItem*> items;

    foreach(key, keys)
    {
        CTrack * trk = trkdb.getTrackByKey(key.key);

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setData(eCoName, eUrType, eTrk);
        item->setData(eCoName, eUrQLKey, trk->getKey());
        item->setIcon(eCoName, trk->getIcon());
        item->setText(eCoName, trk->getName());
        item->setToolTip(eCoName, trk->getInfo());

        items << item;
    }

    itemWksTrk->addChildren(items);
    itemWksTrk->setHidden(itemWksTrk->childCount() == 0);

    if(!isInternalEdit)
    {
        changedWorkspace();
    }
}

void CGeoDB::slotRteDBChanged()
{
    CGeoDBInternalEditLock lock(this);

    qDeleteAll(itemWksRte->takeChildren());

    CRouteDB& rtedb = CRouteDB::self();
    CRouteDB::keys_t key;
    QList<CRouteDB::keys_t> keys = rtedb.keys();

    QList<QTreeWidgetItem*> items;

    foreach(key, keys)
    {
        CRoute * rte = rtedb.getRouteByKey(key.key);

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setData(eCoName, eUrType, eRte);
        item->setData(eCoName, eUrQLKey, rte->getKey());
        item->setIcon(eCoName, rte->getIcon());
        item->setText(eCoName, rte->getName());
        item->setToolTip(eCoName, rte->getInfo());

        items << item;
    }

    itemWksRte->addChildren(items);
    itemWksRte->setHidden(itemWksRte->childCount() == 0);

    if(!isInternalEdit)
    {
        changedWorkspace();
    }
}

void CGeoDB::slotOvlDBChanged()
{
    CGeoDBInternalEditLock lock(this);

    qDeleteAll(itemWksOvl->takeChildren());

    COverlayDB& ovldb = COverlayDB::self();
    COverlayDB::keys_t key;
    QList<COverlayDB::keys_t> keys = ovldb.keys();

    QList<QTreeWidgetItem*> items;

    foreach(key, keys)
    {
        IOverlay * ovl = ovldb.getOverlayByKey(key.key);

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setData(eCoName, eUrType, eOvl);
        item->setData(eCoName, eUrQLKey, ovl->getKey());
        item->setIcon(eCoName, ovl->getIcon());
        item->setText(eCoName, ovl->getName());
        item->setToolTip(eCoName, ovl->getInfo());

        items << item;
    }

    itemWksOvl->addChildren(items);
    itemWksOvl->setHidden(itemWksOvl->childCount() == 0);

    if(!isInternalEdit)
    {
        changedWorkspace();
    }
}

void CGeoDB::slotModifiedWpt(const QString& key)
{
    keysWptModified << key;
    updateModifyMarker();
}

void CGeoDB::slotModifiedTrk(const QString& key)
{
    keysTrkModified << key;
    updateModifyMarker();
}

void CGeoDB::slotModifiedRte(const QString& key)
{
    keysRteModified << key;
    updateModifyMarker();
}

void CGeoDB::slotModifiedOvl(const QString& key)
{
    keysOvlModified << key;
    updateModifyMarker();
}


void CGeoDB::slotItemExpanded(QTreeWidgetItem * item)
{
    CGeoDBInternalEditLock lock(this);

    if(item->data(eCoName, eUrType).toInt() == eFolderT || (item->parent() && item->parent()->data(eCoName, eUrDBKey) == 1))
    {
        return;
    }

    const int size = item->childCount();
    if(size == 0)
    {
        queryChildrenFromDB(item, 2);
    }
    else
    {
        for(int i = 0; i< size; i++)
        {
            QTreeWidgetItem  * child = item->child(i);
            if(child->data(eCoName, eUrType).toInt() >= eFolder0 && child->childCount() == 0)
            {
                queryChildrenFromDB(child, 1);
            }
        }
    }

    updateCheckmarks(item);
}


void CGeoDB::slotItemDoubleClicked(QTreeWidgetItem * item, int column)
{

    QStringList keys;
    keys << item->data(eCoName, eUrQLKey).toString();

    switch(item->data(eCoName, eUrType).toInt())
    {
        case eWpt:
        {
            CWptDB::self().makeVisible(keys);
            break;
        }
        case eTrk:
        {
            CTrackDB::self().makeVisible(keys);
            break;
        }
        case eRte:
        {
            CRouteDB::self().makeVisible(keys);
            break;
        }
        case eOvl:
        {
            COverlayDB::self().makeVisible(keys);
            break;
        }
    }
}

void CGeoDB::slotItemChanged(QTreeWidgetItem * item, int column)
{
    if(isInternalEdit != 0)
    {
        return;
    }


    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    if(column == eCoName)
    {
        quint64 itemId = item->data(eCoName, eUrDBKey).toULongLong();
        QString itemText = item->text(eCoName);

        if(itemText.isEmpty() || item->data(eCoName, eUrType).toInt() < eFolder0)
        {
            return;
        }

        query.prepare("UPDATE folders SET name=:name WHERE id=:id");
        query.bindValue(":name", itemText);
        query.bindValue(":id", itemId);
        QUERY_EXEC(return);

        updateFolderById(itemId);
    }
    else if(column == eCoState)
    {
        if(item->checkState(eCoState) == Qt::Checked)
        {
            if(item->data(eCoName, eUrType).toInt() >= eFolder0)
            {
                addChildrenToWks(item->data(eCoName, eUrDBKey).toULongLong());
            }
            else
            {
                query.prepare("SELECT data, type FROM items WHERE id=:id");
                query.bindValue(":id", item->data(eCoName, eUrDBKey));
                QUERY_EXEC(return);
                if(query.next())
                {
                    CQlb qlb(this);
                    switch(query.value(1).toInt())
                    {
                        case eWpt:
                            qlb.waypoints() = query.value(0).toByteArray();
                            CWptDB::self().loadQLB(qlb, false);
                            break;
                        case eTrk:
                            qlb.tracks() = query.value(0).toByteArray();
                            CTrackDB::self().loadQLB(qlb, false);
                            break;
                        case eRte:
                            qlb.routes() = query.value(0).toByteArray();
                            CRouteDB::self().loadQLB(qlb, false);
                            break;
                        case eOvl:
                            qlb.overlays() = query.value(0).toByteArray();
                            COverlayDB::self().loadQLB(qlb, false);
                            break;
                    }
                }
            }
        }
        else
        {
            if(item->data(eCoName, eUrType).toInt() >= eFolder0)
            {
                delChildrenFromWks(item->data(eCoName, eUrDBKey).toULongLong());
            }
            else
            {
                query.prepare("SELECT key FROM items WHERE id=:id");
                query.bindValue(":id", item->data(eCoName, eUrDBKey));
                QUERY_EXEC(return);
                if(query.next())
                {
                    QString key = query.value(0).toString();

                    switch(item->data(eCoName, eUrType).toInt())
                    {
                        case eWpt:
                            CWptDB::self().delWpt(key, false);
                            keysWptModified.remove(key);
                            break;
                        case eTrk:
                            CTrackDB::self().delTrack(key, false);
                            keysTrkModified.remove(key);
                            break;
                        case eRte:
                            CRouteDB::self().delRoute(key, false);
                            keysRteModified.remove(key);
                            break;
                        case eOvl:
                            COverlayDB::self().delOverlay(key,false);
                            keysOvlModified.remove(key);
                            break;
                    }
                }
            }
        }
        changedWorkspace();
    }
}

void CGeoDB::slotContextMenuDatabase(const QPoint& pos)
{
    QTreeWidgetItem * item = treeDatabase->currentItem();
    QTreeWidgetItem * top  = item->parent();
    while(top && top->parent()) top = top->parent();
    if(top == 0) top = item;

    if(top == itemLostFound)
    {
        QPoint p = treeDatabase->mapToGlobal(pos);
        contextMenuLost->exec(p);
    }
    else if(top == itemDatabase)
    {
        if(item->data(eCoName, eUrType).toInt() >= eFolder0)
        {
            if(item == itemDatabase)
            {
                actDelDir->setVisible(false);
                actEditDir->setVisible(false);
                actMoveDir->setVisible(false);
                actCopyDir->setVisible(false);
            }
            else
            {
                actDelDir->setVisible(true);
                actEditDir->setVisible(true);

                if(item->data(eCoName, eUrType).toInt() == eFolder2)
                {
                    actMoveDir->setVisible(true);
                    actCopyDir->setVisible(true);
                }
                else
                {
                    actMoveDir->setVisible(false);
                    actCopyDir->setVisible(false);

                }
            }

            QPoint p = treeDatabase->mapToGlobal(pos);
            contextMenuFolder->exec(p);
        }
        else
        {
            QPoint p = treeDatabase->mapToGlobal(pos);
            contextMenuItem->exec(p);
        }
    }

}

void CGeoDB::slotContextMenuWorkspace(const QPoint& pos)
{
    QTreeWidgetItem * item = treeWorkspace->currentItem();
    if(item->data(eCoName, eUrType).toInt() >= eFolder0)
    {
        actHardCopy->setVisible(false);
    }
    else
    {
        if(item->data(eCoName,eUrDBKey).toULongLong() == 0)
        {
            actHardCopy->setVisible(false);
        }
        else
        {
            actHardCopy->setVisible(true);
        }

    }

    QPoint p = treeWorkspace->mapToGlobal(pos);
    contextMenuWks->exec(p);
}



void CGeoDB::slotAddFolder()
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item = treeDatabase->currentItem();
    if(item == 0)
    {
        return;
    }

    QString name;
    QString comment;
    int type = eFolder2;

    CDlgEditFolder dlg(name, comment, type);
    if(dlg.exec() == QDialog::Rejected)
    {
        return;
    }

    addFolder(item, name, comment, type);
}

void CGeoDB::slotDelFolder()
{
    CGeoDBInternalEditLock lock(this);

    /// @todo delete just current folder or all selected?
    QTreeWidgetItem * item = treeDatabase->currentItem();
    QMessageBox::StandardButton but = QMessageBox::question(0, tr("Delete folder..."), tr("You are sure you want to delete '%1' and all items below?").arg(item->text(eCoName)), QMessageBox::Ok|QMessageBox::Abort);
    if(but == QMessageBox::Ok)
    {
        delFolder(item, true);
    }

    changedWorkspace();
    updateLostFound();
}



void CGeoDB::slotEditFolder()
{
    CGeoDBInternalEditLock lock(this);

    QTreeWidgetItem * item = treeDatabase->currentItem();
    quint64 itemId  = item->data(eCoName, eUrDBKey).toULongLong();
    QString name    = item->text(eCoName);
    QString comment = item->toolTip(eCoName);
    int type        = item->data(eCoName, eUrType).toInt();

    CDlgEditFolder dlg(name,comment,type);
    dlg.exec();

    QSqlQuery query(db);
    query.prepare("UPDATE folders SET name=:name, comment=:comment, type=:type, icon=:icon WHERE id=:id");
    query.bindValue(":name", name);
    query.bindValue(":comment", comment);
    query.bindValue(":type", type);
    query.bindValue(":id", itemId);
    switch(type)
    {

    case eFolder1:
        query.bindValue(":icon", ":/icons/iconFolderBlue16x16");
        break;
    case eFolder2:
        query.bindValue(":icon", ":/icons/iconFolderGreen16x16");
        break;
    case eFolderN:
        query.bindValue(":icon", ":/icons/iconFolderOrange16x16");
        break;
    }

    QUERY_EXEC(return);

    updateFolderById(itemId);
}

void CGeoDB::slotCopyFolder()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    quint64 parentId = 0, childId = 0;
    CDlgSelGeoDBFolder dlg(db, parentId, true);

    dlg.exec();

    if(parentId == 0)
    {
        return;
    }

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->selectedItems();

    foreach(item, items)
    {
        if(item->data(eCoName, eUrType).toInt() < eFolder1)
        {
            continue;
        }

        childId = item->data(eCoName, eUrDBKey).toULongLong();

        // create link folder <-> item
        query.prepare("SELECT id FROM folder2folder WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId);
        query.bindValue(":child", childId);
        QUERY_EXEC();

        if(!query.next())
        {
            query.prepare("INSERT INTO folder2folder (parent, child) VALUES (:parent, :child)");
            query.bindValue(":parent", parentId);
            query.bindValue(":child", childId);
            QUERY_EXEC(return);
            // update tree widget
            addFolderById(parentId, item);
        }
    }
}


void CGeoDB::slotMoveFolder()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    quint64 parentId1 = 0, parentId2 = 0, childId = 0;
    CDlgSelGeoDBFolder dlg(db, parentId2, true);

    dlg.exec();

    if(parentId2 == 0)
    {
        return;
    }

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->selectedItems();

    foreach(item, items)
    {
        if(item->data(eCoName, eUrType).toInt() < eFolder2)
        {
            continue;
        }

        childId     = item->data(eCoName, eUrDBKey).toULongLong();
        parentId1   = item->parent()->data(eCoName, eUrDBKey).toULongLong();

        query.prepare("DELETE FROM folder2folder WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId1);
        query.bindValue(":child", childId);
        QUERY_EXEC(return);

        // create link folder <-> item
        query.prepare("SELECT id FROM folder2folder WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId2);
        query.bindValue(":child", childId);
        QUERY_EXEC();

        if(!query.next())
        {
            query.prepare("INSERT INTO folder2folder (parent, child) VALUES (:parent, :child)");
            query.bindValue(":parent", parentId2);
            query.bindValue(":child", childId);
            QUERY_EXEC(return);
            // update tree widget
            addFolderById(parentId2, item);
        }

        delFolderById(parentId1, childId);
    }
}

void CGeoDB::slotDelItems()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);
    QTreeWidgetItem* item         = treeDatabase->currentItem();
    QList<QTreeWidgetItem*> items = treeDatabase->selectedItems();

    quint64 parentId = item->parent()->data(eCoName, eUrDBKey).toULongLong();

    quint32 progCnt = 0;
    PROGRESS_SETUP(tr("Delete items."), items.size());

    foreach(item, items)
    {
        PROGRESS(progCnt++, break);

        if(item->data(eCoName, eUrType).toInt() >= eFolder0)
        {
            continue;
        }
        quint64 childId = item->data(eCoName, eUrDBKey).toULongLong();

        query.prepare("DELETE FROM folder2item WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId);
        query.bindValue(":child", childId);
        QUERY_EXEC(continue);

        delItemById(parentId, childId);
    }

    changedWorkspace();
    updateLostFound();
}


void CGeoDB::slotCopyItems()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    quint64 parentId = 0;
    CDlgSelGeoDBFolder dlg(db, parentId);

    dlg.exec();

    if(parentId == 0)
    {
        return;
    }

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items = treeDatabase->selectedItems();

    quint32 progCnt = 0;
    PROGRESS_SETUP(tr("Copy items."), items.size());

    foreach(item, items)
    {

        PROGRESS(progCnt++, break);

        if(item->data(eCoName, eUrType).toInt() >= eFolder0)
        {
            continue;
        }
        quint64 childId = item->data(eCoName, eUrDBKey).toULongLong();

        // create link folder <-> item
        query.prepare("SELECT id FROM folder2item WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId);
        query.bindValue(":child", childId);
        QUERY_EXEC();

        if(!query.next())
        {
            query.prepare("INSERT INTO folder2item (parent, child) VALUES (:parent, :child)");
            query.bindValue(":parent", parentId);
            query.bindValue(":child", childId);
            QUERY_EXEC(return);
            // update tree widget
            addItemById(parentId, item);
        }
    }

    changedWorkspace();
}


void CGeoDB::slotMoveItems()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    quint64 parentId1 = 0, parentId2 = 0;
    CDlgSelGeoDBFolder dlg(db, parentId2);

    dlg.exec();

    if(parentId2 == 0)
    {
        return;
    }

    QTreeWidgetItem * item        = treeDatabase->currentItem();;
    QList<QTreeWidgetItem*> items = treeDatabase->selectedItems();

    parentId1 = item->parent()->data(eCoName, eUrDBKey).toULongLong();

    quint32 progCnt = 0;
    PROGRESS_SETUP(tr("Move items."), items.size());

    foreach(item, items)
    {
        PROGRESS(progCnt++, break);

        if(item->data(eCoName, eUrType).toInt() >= eFolder0)
        {
            continue;
        }
        quint64 childId = item->data(eCoName, eUrDBKey).toULongLong();

        query.prepare("DELETE FROM folder2item WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId1);
        query.bindValue(":child", childId);
        QUERY_EXEC(continue);

        // create link folder <-> item
        query.prepare("SELECT id FROM folder2item WHERE parent=:parent AND child=:child");
        query.bindValue(":parent", parentId2);
        query.bindValue(":child", childId);
        QUERY_EXEC();

        if(!query.next())
        {
            query.prepare("INSERT INTO folder2item (parent, child) VALUES (:parent, :child)");
            query.bindValue(":parent", parentId2);
            query.bindValue(":child", childId);
            QUERY_EXEC(return);
            // update tree widget
            addFolderById(parentId2, item);
        }

        delItemById(parentId1, childId);
    }

    changedWorkspace();
}

void CGeoDB::slotMoveLost()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    quint64 parentId, childId = 0;
    CDlgSelGeoDBFolder dlg(db, parentId);

    dlg.exec();

    if(parentId == 0)
    {
        return;
    }

    bool moveAll = treeDatabase->currentItem() == itemLostFound;

    QTreeWidgetItem * item;
    const int size = itemLostFound->childCount();

    PROGRESS_SETUP(tr("Move items."), size);

    for(int i = 0; i < size; i++)
    {
        PROGRESS(i, break);

        item = itemLostFound->child(i);
        if(!item->isSelected() && !moveAll)
        {
            continue;
        }

        childId = item->data(eCoName, eUrDBKey).toULongLong();

        query.prepare("INSERT INTO folder2item (parent, child) VALUES (:parent, :child)");
        query.bindValue(":parent", parentId);
        query.bindValue(":child", childId);
        QUERY_EXEC(return);
        // update tree widget
        addFolderById(parentId, item);
    }

    updateLostFound();
    changedWorkspace();
}

void CGeoDB::slotDelLost()
{
    CGeoDBInternalEditLock lock(this);
    QSqlQuery query(db);

    bool delAll = treeDatabase->currentItem() == itemLostFound;

    QTreeWidgetItem * item;
    const int size = itemLostFound->childCount();

    PROGRESS_SETUP(tr("Delete items."), size);

    for(int i = 0; i < size; i++)
    {
        PROGRESS(i, break);

        item = itemLostFound->child(i);
        if(!item->isSelected() && !delAll)
        {
            continue;
        }

        query.prepare("DELETE FROM items WHERE id=:id");
        query.bindValue(":id",item->data(eCoName, eUrDBKey));
        QUERY_EXEC(continue);
    }

    updateLostFound();
    changedWorkspace();
}

void CGeoDB::slotAddItems()
{
    CGeoDBInternalEditLock lock(this);


    quint64 parentId;
    CDlgSelGeoDBFolder dlg(db, parentId);

    dlg.exec();

    if(parentId == 0)
    {
        return;
    }

    quint32 total = itemWksWpt->childCount() + itemWksTrk->childCount() + itemWksRte->childCount() + itemWksOvl->childCount();
    quint32 progCnt = 0;

    PROGRESS_SETUP(tr("Add items to database."), total);

    int size, i;
    QTreeWidgetItem * item;

    bool addAll     = treeWorkspace->currentItem() == itemWorkspace;
    bool addAllWpt  = treeWorkspace->currentItem() == itemWksWpt;
    bool addAllTrk  = treeWorkspace->currentItem() == itemWksTrk;
    bool addAllRte  = treeWorkspace->currentItem() == itemWksRte;
    bool addAllOvl  = treeWorkspace->currentItem() == itemWksOvl;

    //////////// add waypoints ////////////
    size = itemWksWpt->childCount();
    for(i = 0; i < size; i++)
    {
        PROGRESS(progCnt++, goto slotAddItems_end);

        item = itemWksWpt->child(i);
        if(!item->isSelected() && !addAll && !addAllWpt)
        {
            continue;
        }

        addItemToDB(parentId, item);
    }
    //////////// add tracks ////////////
    size = itemWksTrk->childCount();
    for(i = 0; i < size; i++)
    {
        PROGRESS(progCnt++, goto slotAddItems_end);

        item = itemWksTrk->child(i);
        if(!item->isSelected() && !addAll && !addAllTrk)
        {
            continue;
        }

        addItemToDB(parentId, item);
    }
    //////////// add tracks ////////////
    size = itemWksRte->childCount();
    for(i = 0; i < size; i++)
    {
        PROGRESS(progCnt++, goto slotAddItems_end);

        item = itemWksRte->child(i);
        if(!item->isSelected() && !addAll && !addAllRte)
        {
            continue;
        }

        addItemToDB(parentId, item);
    }
    //////////// add overlays ////////////
    size = itemWksOvl->childCount();
    for(i = 0; i < size; i++)
    {
        PROGRESS(progCnt++, goto slotAddItems_end);

        item = itemWksOvl->child(i);
        if(!item->isSelected() && !addAll&& !addAllOvl)
        {
            continue;
        }

        addItemToDB(parentId, item);
    }

 slotAddItems_end:
    changedWorkspace();
}



//void CGeoDB::updateItemById(quint64 id)
//{
//    CGeoDBInternalEditLock lock(this);

//    QTreeWidgetItem * item;
//    QList<QTreeWidgetItem*> items = treeDatabase->findItems("*", Qt::MatchWildcard|Qt::MatchRecursive, eCoName);

//    QSqlQuery query(db);
//    query.prepare("SELECT icon, name, comment FROM items WHERE id=:id");
//    query.bindValue(":id", id);
//    QUERY_EXEC();
//    query.next();

//    foreach(item, items)
//    {
//        if(item->data(eCoName, eUrType).toInt() >= eFolder0)
//        {
//            continue;
//        }
//        if(item->data(eCoName, eUrDBKey).toULongLong() == id)
//        {
//            switch(item->data(eCoName, eUrType).toInt())
//            {
//                case eWpt:
//                {
//                    item->setIcon(eCoName, getWptIconByName(query.value(0).toString()));
//                    break;
//                }
//                case eTrk:
//                {
//                    QPixmap pixmap(16,16);
//                    pixmap.fill(query.value(0).toString());
//                    item->setIcon(eCoName, pixmap);
//                    break;
//                }
//                case eRte:
//                {

//                    break;
//                }
//                case eOvl:
//                {
//                    item->setIcon(eCoName, QIcon(query.value(0).toString()));
//                    break;
//                }
//            }


//            item->setText(eCoName, query.value(1).toString());
//            item->setToolTip(eCoName, query.value(2).toString());
//        }
//    }
//}







//void CGeoDB::slotSaveItems()
//{
//    int i;
//    CGeoDBInternalEditLock lock(this);
//    QSqlQuery query(db);

//    bool saveAll = treeDatabase->currentItem() == itemWorkspace;
//    bool savedAll = true;
//    bool addUnknownItems = false;

//    QList<QTreeWidgetItem*> items;
//    for(i = 0; i < itemWksWpt->childCount(); i++)
//    {
//        items << itemWksWpt->child(i);
//    }
//    for(i = 0; i < itemWksTrk->childCount(); i++)
//    {
//        items << itemWksTrk->child(i);
//    }
//    for(i = 0; i < itemWksRte->childCount(); i++)
//    {
//        items << itemWksRte->child(i);
//    }
//    for(i = 0; i < itemWksOvl->childCount(); i++)
//    {
//        items << itemWksOvl->child(i);
//    }


//    const int size = items.size();
//    QTreeWidgetItem * item;
//    for(i = 0; i < size; i++)
//    {
//        item = items[i];
//        if(item->data(eCoName, eUrDBKey).toULongLong() == 0)
//        {

//            addUnknownItems = (item->isSelected()||saveAll);

//        }
//        else
//        {
//            if(!item->isSelected())
//            {
//                savedAll = false;
//            }
//        }
//    }

//    if(addUnknownItems)
//    {
//        QMessageBox::Button res = QMessageBox::question(0, tr("Unknown items..."),
//                                                        tr("There are items that haven't been added to the database. Add them to Lost & Found?"),
//                                                        QMessageBox::Ok|QMessageBox::Ignore, QMessageBox::Ignore
//                                                        );
//        addUnknownItems = res == QMessageBox::Ok;
//    }

//    for(i = 0; i < size; i++)
//    {
//        QString icon, name, comment;

//        item = items[i];
//        if(!item->isSelected() && !saveAll)
//        {
//            continue;
//        }

//        quint64 childId = item->data(eCoName, eUrDBKey).toULongLong();
//        if(childId == 0 && addUnknownItems)
//        {
//            // add to database
//            switch(item->data(eCoName, eUrType).toInt())
//            {
//                case eWpt:
//                {
//                    addWptToDB(0, item);
//                    break;
//                }
//                case eTrk:
//                {
//                    addTrkToDB(0, item);
//                    break;
//                }
//                case eRte:
//                {

//                    break;
//                }
//                case eOvl:
//                {
//                    addOvlToDB(0, item);
//                    break;
//                }
//            }

//        }
//        else
//        {
//            QSet<QString> * keysWksModified = 0;
//            // update database
//            QString key = item->data(eCoName, eUrQLKey).toString();
//            QBuffer buffer;
//            CQlb qlb(this);

//            switch(item->data(eCoName, eUrType).toInt())
//            {
//                case eWpt:
//                {
//                    keysWksModified = &keysWptModified;

//                    CWpt * wpt = CWptDB::self().getWptByKey(key);
//                    icon = wpt->getIconString();
//                    name = wpt->getName();
//                    comment = wpt->getComment();
//                    qlb << *wpt;
//                    break;
//                }
//                case eTrk:
//                {
//                    keysWksModified = &keysTrkModified;

//                    CTrack * trk = CTrackDB::self().getTrackByKey(key);
//                    icon = trk->getColor().name();
//                    name = trk->getName();
//                    comment = trk->getComment();
//                    qlb << *trk;
//                    break;
//                }
//                case eRte:
//                {
//                    keysWksModified = &keysRteModified;
//                    break;
//                }
//                case eOvl:
//                {
//                    keysWksModified = &keysOvlModified;

//                    /// @todo make that stuff common to all overlays via IOverlay
//                    IOverlay * ovl = COverlayDB::self().getOverlayByKey(key);
//                    if(ovl->type == "Text")
//                    {
//                        COverlayText * _ovl_ = qobject_cast<COverlayText*>(ovl);
//                        icon = ":/icons/iconText16x16";
//                        name = tr("Static text");
//                        comment = _ovl_->getInfo();
//                    }
//                    else if(ovl->type == "TextBox")
//                    {
//                        COverlayTextBox * _ovl_ = qobject_cast<COverlayTextBox*>(ovl);
//                        icon = ":/icons/iconTextBox16x16";
//                        name = tr("Geo ref. text");
//                        comment = _ovl_->getInfo();
//                    }
//                    else if(ovl->type == "Distance")
//                    {
//                        COverlayDistance * _ovl_ = qobject_cast<COverlayDistance*>(ovl);
//                        icon = ":/icons/iconDistance16x16";
//                        name = _ovl_->getName();
//                        comment = _ovl_->getInfo();
//                    }
//                    qlb << *ovl;
//                    break;
//                }
//            }
//            qlb.save(&buffer);

//            query.prepare("UPDATE items SET icon=:icon, name=:name, comment=:comment, data=:data WHERE id=:id");
//            query.bindValue(":icon", icon);
//            query.bindValue(":name", name);
//            query.bindValue(":comment", comment);
//            query.bindValue(":data", buffer.data());
//            query.bindValue(":id", childId);
//            QUERY_EXEC(continue);

//            keysWksModified->remove(item->data(eCoName, eUrQLKey).toString());
//            item->setText(eCoState,"");
//            updateItemById(childId);

//        }

//    }

//    updateModifyMarker();
//    updateLostFound();
//}

//void CGeoDB::slotHardCopyItem()
//{
//    QTreeWidgetItem * item;
//    QList<QTreeWidgetItem*> items = treeDatabase->selectedItems();

//    CQlb qlb(this);

//    QStringList keysWpt;
//    QStringList keysTrk;
////    QStringList keysRte;
//    QStringList keysOvl;

//    foreach(item, items)
//    {
//        switch(item->data(eCoName, eUrType).toInt())
//        {
//            case eWpt:
//            {
//                CWpt * wpt = CWptDB::self().getWptByKey(item->data(eCoName,eUrQLKey).toString());
//                qlb     << *wpt;
//                keysWpt << wpt->getKey();
//                break;
//            }
//            case eTrk:
//            {
//                CTrack * trk = CTrackDB::self().getTrackByKey(item->data(eCoName,eUrQLKey).toString());
//                qlb     << *trk;
//                keysTrk << trk->getKey();
//                break;
//            }
//            case eRte:
//            {
////                CRoute * rte = CRouteDB::self().getRouteByKey(item->data(eCoName,eUrQLKey).toString());
////                iortes << rte;

//                break;
//            }
//            case eOvl:
//            {
//                IOverlay * ovl = COverlayDB::self().getOverlayByKey(item->data(eCoName,eUrQLKey).toString());
//                qlb     << *ovl;
//                keysOvl << ovl->getKey();
//                break;
//            }
//            default:
//                break;
//        }
//    }

//    CWptDB::self().delWpt(keysWpt);
//    CTrackDB::self().delTracks(keysTrk);
////    CRouteDB::self().delRoutes(keysRte);
//    COverlayDB::self().delOverlays(keysOvl);

//    CWptDB::self().loadQLB(qlb, true);
//    CTrackDB::self().loadQLB(qlb, true);
//    CRouteDB::self().loadQLB(qlb, true);
//    COverlayDB::self().loadQLB(qlb, true);

//}





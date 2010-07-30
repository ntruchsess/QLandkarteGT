/**********************************************************************************************

  DSP Solutions GmbH & Co. KG
  http://www.dspsolutions.de/  

  Author:      Oliver Eichler
  Email:       oliver.eichler@dspsolutions.de
  Phone:       +49-941-83055-1
  Fax:         +49-941-83055-79

  File:        CGeoDB.cpp

  Module:      

  Description:

  Created:     07/29/2010

  (C) 2010 DSP Solutions. All rights reserved.


**********************************************************************************************/

#include "CGeoDB.h"

#include <QtGui>

CGeoDB::CGeoDB(QTabWidget * tb, QWidget * parent)
    : QWidget(parent)
    , tabbar(tb)
{
    setupUi(this);
    setObjectName("GeoDB");

    tabbar->insertTab(0,this, QIcon(":/icons/iconGeoDB16x16"),"");
    tabbar->setTabToolTip(tabbar->indexOf(this), tr("Manage your Geo Data Base"));

    itemWorkspace = new QTreeWidgetItem(treeWidget);
    itemWorkspace->setText(0, tr("Workspace"));
    itemWorkspace->setIcon(0, QIcon(":/icons/iconGlobe16x16"));

    itemDatabase = new QTreeWidgetItem(treeWidget);
    itemDatabase->setText(0, tr("Database"));
    itemDatabase->setIcon(0, QIcon(":/icons/iconGeoDB16x16"));
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

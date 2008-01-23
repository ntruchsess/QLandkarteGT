/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDeviceTBDOE.h"
#include "CWpt.h"

#include <QtGui>


CDeviceTBDOE::CDeviceTBDOE(const QString& ipaddr, quint16 port, QObject * parent)
    : IDevice("QLandkarteM",parent)
    , ipaddr(ipaddr)
    , port(port)
    , timeout(3000)
{

}

CDeviceTBDOE::~CDeviceTBDOE()
{

}

void CDeviceTBDOE::send(const packet_e type, const QByteArray& data)
{
    QByteArray packet;
    QDataStream out(&packet,QIODevice::WriteOnly);
    out << (qint32)type;
    out << (qint32)0;
    out << data;
    out.device()->seek(sizeof(type));
    out << (qint32)(packet.size() - 2 * sizeof(qint32));

    socket.write(packet);
}

bool CDeviceTBDOE::recv(packet_e& type, QByteArray& data)
{
    qint32 size;
    QDataStream in(&socket);

    while(socket.bytesAvailable() < (int)(2 * sizeof(qint32))){
        if (!socket.waitForReadyRead(timeout)) {
                return false;
        }
    }

    in >> (qint32&)type;
    in >> size;

    while(socket.bytesAvailable() < size){
        if (!socket.waitForReadyRead(timeout)) {
                return false;
        }
    }

    in >> data;
    return true;
}


void CDeviceTBDOE::uploadWpts(QList<CWpt*>& wpts)
{
    qint32 ack = -1;


    qDebug() << ipaddr << port;
    socket.connectToHost(ipaddr,port);
    if(!socket.waitForConnected(timeout)){
        QMessageBox::critical(0,tr("Error..."), tr("Failed to connect to device."),QMessageBox::Abort,QMessageBox::Abort);
        return;
    }

    QDataStream socketstream(&socket);

    QList<CWpt*>::iterator wpt = wpts.begin();
    while(wpt != wpts.end()){
        QByteArray buf;
        QDataStream s(&buf,QIODevice::WriteOnly);

        s << *(*wpt);
        send(eWpt,buf);


        while(socket.bytesAvailable() < (int)sizeof(ack)){
            if (!socket.waitForReadyRead(10000)) {
                 QMessageBox::critical(0,tr("Error..."), tr("Response timeout."),QMessageBox::Abort,QMessageBox::Abort);
                 return;
            }
        }
        socketstream >> ack;
        ++wpt;
    }


    socket.disconnectFromHost();
}

void CDeviceTBDOE::downloadWpts(QList<CWpt*>& wpts)
{

}


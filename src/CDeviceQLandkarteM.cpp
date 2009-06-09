/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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
#include "CDeviceQLandkarteM.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CTrack.h"
#include "CTrackDB.h"

#include <QtGui>
#include <QNetworkInterface>

#define REMOTE_PORT 45454

CDeviceQLandkarteM::CDeviceQLandkarteM(const QString& ipaddr, quint16 port, QObject * parent)
: IDevice("QLandkarteM",parent)
, ipaddr(ipaddr)
, port(port)
, timeout(60000)
{
    udpSocket.bind(45453);
    connect(&udpSocket, SIGNAL(readyRead()),this, SLOT(detectedDevice()));

}


CDeviceQLandkarteM::~CDeviceQLandkarteM()
{

}


bool CDeviceQLandkarteM::acquire(const QString& operation, int max)
{
    createProgress(operation, tr("Connect to device."), max);
    qApp->processEvents();

    QByteArray datagram;
    datagram = "START";
    udpSocket.writeDatagram(datagram.data(), datagram.size(), QHostAddress::QHostAddress(ipaddr), port);

    qApp->processEvents();

    tcpSocket.connectToHost(ipaddr,port);
    if(!tcpSocket.waitForConnected(timeout)) {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to connect to device."),QMessageBox::Abort,QMessageBox::Abort);
        release();
        return false;
    }
    return true;
}


void CDeviceQLandkarteM::send(const packet_e type, const QByteArray& data)
{
    QByteArray packet;
    QDataStream out(&packet,QIODevice::WriteOnly);
    out << (qint32)type;
    out << (qint32)0;
    out << data;
    out.device()->seek(sizeof(type));
    out << (qint32)(packet.size() - 2 * sizeof(qint32));

    tcpSocket.write(packet);
}


bool CDeviceQLandkarteM::recv(packet_e& type, QByteArray& data)
{
    qint32 size;
    QDataStream in(&tcpSocket);

    while(tcpSocket.bytesAvailable() < (int)(2 * sizeof(qint32))) {
        if (!tcpSocket.waitForReadyRead(timeout)) {
            return false;
        }
    }

    in >> (qint32&)type;
    in >> size;

    while(tcpSocket.bytesAvailable() < size) {
        if (!tcpSocket.waitForReadyRead(timeout)) {
            return false;
        }
    }

    in >> data;
    return true;
}


bool CDeviceQLandkarteM::exchange(packet_e& type,QByteArray& data)
{
    send(type,data);
    qApp->processEvents();
    data.clear();
    return recv(type,data);
}


void CDeviceQLandkarteM::release()
{
    if(progress) progress->close();
    tcpSocket.disconnectFromHost();
}


void CDeviceQLandkarteM::uploadWpts(const QList<CWpt*>& wpts)
{

    if(!startDeviceDetection()) return;
    if(!acquire(tr("Upload waypoints ..."), wpts.count())) return;

    packet_e type;
    int cnt = 0;
    QList<CWpt*>::const_iterator wpt = wpts.begin();
    while(wpt != wpts.end() && !progress->wasCanceled()) {
        QByteArray data;
        QDataStream s(&data,QIODevice::WriteOnly);

        progress->setLabelText(tr("%1\n%2 of %3").arg((*wpt)->name).arg(++cnt).arg(wpts.count()));
        progress->setValue(cnt);
        qApp->processEvents();

        s << *(*wpt);

        if(!exchange(type = eC2HWpt,data)) {
            QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to transfer waypoints."),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        if(type == eError) {
            QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        ++wpt;
    }

    return release();
}


void CDeviceQLandkarteM::downloadWpts(QList<CWpt*>& wpts)
{
    if(!startDeviceDetection()) return;
    if(!acquire(tr("Download waypoints ..."), wpts.count())) return;

    progress->setLabelText(tr("Query list of waypoints from the device"));
    qApp->processEvents();

    packet_e type;
    QByteArray  data1;

    if(!exchange(type = eH2CWptQuery,data1)) {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to query waypoints from device."),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    if(type == eError) {
        QMessageBox::critical(0,tr("Error..."), QString(data1),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    quint32     nWpt = 0;
    quint32     n;
    QString     key, name;

    QDataStream wptlist(&data1, QIODevice::ReadOnly);

    wptlist >> nWpt;

    progress->setMaximum(nWpt);
    for(n = 0; n < nWpt; ++n) {
        QByteArray data;
        QDataStream stream(&data,QIODevice::ReadWrite);
        wptlist >> key >> name;

        progress->setLabelText(tr("Download waypoint: %1").arg(name));
        qApp->processEvents();

        stream << key;

        if(!exchange(type = eH2CWpt,data)) {
            QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to transfer waypoints."),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        if(type == eError) {
            QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        stream.device()->seek(0);

        CWpt * wpt = new CWpt(&CWptDB::self());
        stream >> *wpt;

        wpts.push_back(wpt);

        progress->setValue(n + 1);
    }

    return release();
}


void CDeviceQLandkarteM::uploadTracks(const QList<CTrack*>& trks)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Upload tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceQLandkarteM::downloadTracks(QList<CTrack*>& trks)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Download tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceQLandkarteM::uploadMap(const QList<IMapSelection*>& mss)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Upload map is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceQLandkarteM::uploadRoutes(const QList<CRoute*>& rtes)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceQLandkarteM::downloadRoutes(QList<CRoute*>& rtes)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


bool CDeviceQLandkarteM::startDeviceDetection()
{
    if(ipaddr.isEmpty() || port == 0) {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QByteArray datagram = "GETADRESS";
		// Send query on all network broadcast for each network interface 
		QList<QNetworkInterface> netdevices = QNetworkInterface::allInterfaces();
		QNetworkInterface netdevice;
		foreach(netdevice, netdevices){
			QList<QNetworkAddressEntry> networks = netdevice.addressEntries();
			QNetworkAddressEntry network;
			foreach(network, networks){
				udpSocket.writeDatagram(datagram.data(), datagram.size(), network.broadcast(), REMOTE_PORT);
				QTime time;
				time.start();
				while(time.elapsed() < 3000) {
					QApplication::processEvents();
				}
			}
		}
        QApplication::restoreOverrideCursor();
    }

    if(ipaddr.isEmpty() || port == 0) {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: No device found. Is it connected to the network?"),QMessageBox::Abort,QMessageBox::Abort);
        return false;
    }

    return true;
}


void CDeviceQLandkarteM::detectedDevice()
{
    while (udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress qlmAddress;
        quint16 qlmPort;

        datagram.resize(udpSocket.pendingDatagramSize());
        udpSocket.readDatagram(datagram.data(), datagram.size(), &qlmAddress, &qlmPort);

        ipaddr  = qlmAddress.toString();
        port    = qlmPort;
        qDebug() << "Device detected is " << datagram << " with address " << ipaddr << " and port " << port << "\r\n";

    }
}

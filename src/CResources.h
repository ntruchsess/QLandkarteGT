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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

**********************************************************************************************/
#ifndef CRESOURCES_H
#define CRESOURCES_H

#include <QObject>
#include <QFont>
#include <QPointer>
#include <QDir>

class IDevice;
class IUnit;

/// all global resources
class CResources : public QObject
{
    Q_OBJECT;
    public:
        virtual ~CResources();

        static CResources& self(){return *m_self;}

        /// get HTTP proxy settings
        /**
            @param url a string to store the proxy's URL
            @param port a 16bit unsigned integer to store the proxy's port

            @return The method will return true if the proxy is enabled.
        */
        bool getHttpProxy(QString& url, quint16& port);

        /// get pointer to current device handler
        IDevice * device();
        QString charset();

        /// the font used for text on the map
        const QFont& getMapFont(){return m_mapfont;}

        /// open an URL in a webbrowser
        void openLink(const QString& link);

        /// root path of all maps
        QString pathMaps;

#ifdef HAS_GEODB
        bool useGeoDB(){return m_useGeoDB;}
        bool saveGeoDBOnExit(){return m_saveGeoDBOnExit;}
        QDir pathGeoDB(){return m_pathGeoDB;}
#endif
        bool flipMouseWheel(){return m_flipMouseWheel;}
        bool showTrackProfilePreview(){return m_showTrackProfile;}
        bool showNorthIndicator(){return m_showNorth;}
        bool showScale(){return m_showScale;}
        bool showToolTip(){return m_showToolTip;}
        bool showTrackMax(){return m_showTrackMax;}
        bool showZoomLevel(){return m_showZoomLevel;}
        bool playSound(){return m_playSound;}

        signals:
        void sigProxyChanged();
        void sigDeviceChanged();

    private:
        friend class CMainWindow;
        friend class CDlgConfig;
        CResources(QObject * parent);

        static CResources * m_self;

        enum browser_e
        {
            eFirefox = 0
            ,eKonqueror = 1
            ,eOther = 2
        };

        /// use proxy for http requests
        bool m_useHttpProxy;
        /// the  proxy name or address
        QString m_httpProxy;
        /// the proxy port
        quint16 m_httpProxyPort;

        /// font used by the map
        QFont m_mapfont;

        /// the installed browser type
        browser_e m_eBrowser;
        /// command string to start Firefox
        QString cmdFirefox;
        /// command string to start Konqueror
        QString cmdKonqueror;
        /// user defined command string
        QString cmdOther;

        /// this offset is needed to correct time in seconds until Dec. 30th, 1989 12:00 to POSIX standard
        quint32 time_offset;

        /// the device key for the desired device
        QString m_devKey;
        /// the actual device access object
        /**
            This can be different from m_devKey. In this case the next call
            to device() will destroy it and load the correct one.
        */
        IDevice * m_device;

        QString m_devIPAddress;
        quint16 m_devIPPort;
        QString m_devSerialPort;
        QString m_devBaudRate;

        QString m_devType;
        QString m_devCharset;

        /// mouse wheel zoom direction
        bool m_flipMouseWheel;

        /// play sound after finishing transfers
        bool m_playSound;

#ifdef HAS_GEODB
        bool m_useGeoDB;
        bool m_saveGeoDBOnExit;
        QDir m_pathGeoDB;
#endif


        bool m_showTrackProfile;
        bool m_showNorth;
        bool m_showScale;
        bool m_showToolTip;
        bool m_showTrackMax;
        bool m_showZoomLevel;

        /// unit translator object
        QPointer<IUnit> unit;
};
#endif                           //CRESOURCES_H

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

#include "WptIcons.h"
#include <QtCore>

const char * wptDefault = ":/icons/wpt/flag15x15.png";

wpt_icon_t wptIcons[] =
{
    /*    mps    desc */
    //     { wptDefault, "Airport" },
    //     { wptDefault, "Amusement Park" },
    //     { wptDefault, "Ball Park" },
    //     { wptDefault, "Bank" },
    //     { wptDefault, "Bar" },
    //     { wptDefault, "Beach" },
    //     { wptDefault, "Bell" },
    //     { wptDefault, "Boat Ramp" },
    //     { wptDefault, "Bowling" },
    //     { wptDefault, "Bridge" },
    //     { wptDefault, "Building" },
    //     { wptDefault, "Campground" },
    //     { ":/icons/wpt/car15x15.png", "Car" },
    //     { wptDefault, "Car Rental" },
    //     { wptDefault, "Car Repair" },
    //     { wptDefault, "Cemetery" },
    //     { wptDefault, "Church" },
    //     { wptDefault, "Circle with X" },
    { ":/icons/wpt/capitol_city15x15.png", "City (Capitol)" },
    { ":/icons/wpt/large_city15x15.png", "City (Large)" },
    { ":/icons/wpt/medium_city15x15.png", "City (Medium)" },
    { ":/icons/wpt/small_city15x15.png", "City (Small)" },
    { ":/icons/wpt/small_city15x15.png", "Small City" },
    //     { wptDefault, "Civil" },
    //     { wptDefault, "Contact, Afro" },
    //     { wptDefault, "Contact, Alien" },
    //     { wptDefault, "Contact, Ball Cap" },
    //     { wptDefault, "Contact, Big Ears" },
    //     { wptDefault, "Contact, Biker" },
    //     { wptDefault, "Contact, Bug" },
    //     { wptDefault, "Contact, Cat" },
    //     { wptDefault, "Contact, Dog" },
    //     { wptDefault, "Contact, Dreadlocks" },
    //     { wptDefault, "Contact, Female1" },
    //     { wptDefault, "Contact, Female2" },
    //     { wptDefault, "Contact, Female3" },
    //     { wptDefault, "Contact, Goatee" },
    //     { wptDefault, "Contact, Kung-Fu" },
    //     { wptDefault, "Contact, Pig" },
    //     { wptDefault, "Contact, Pirate" },
    //     { wptDefault, "Contact, Ranger" },
    //     { wptDefault, "Contact, Smiley" },
    //     { wptDefault, "Contact, Spike" },
    //     { wptDefault, "Contact, Sumo" },
    //     { wptDefault, "Controlled Area" },
    //     { wptDefault, "Convenience Store" },
    //     { ":/icons/wpt/crossing15x15.png", "Crossing" },
    //     { wptDefault, "Dam" },
    //     { ":/icons/wpt/danger15x15.png", "Danger Area" },
    //     { wptDefault, "Department Store" },
    //     { wptDefault, "Diver Down Flag 1" },
    //     { wptDefault, "Diver Down Flag 2" },
    //     { wptDefault, "Drinking Water" },
    //     { wptDefault, "Exit" },
    //     { wptDefault, "Fast Food" },
    //     { wptDefault, "Fishing Area" },
    //     { wptDefault, "Fitness Center" },
    //     { wptDefault, "Flag" },
    //     { ":/icons/wpt/tree15x15.png", "Forest" },
    //     { wptDefault, "Gas Station" },
    { ":/icons/wpt/geocache15x15.png", "Geocache" },
    { ":/icons/wpt/geocache_fnd15x15.png", "Geocache Found" },
    //     { wptDefault, "Ghost Town" },
    //     { wptDefault, "Glider Area" },
    //     { wptDefault, "Golf Course" },
    //     { wptDefault, "Green Diamond" },
    //     { wptDefault, "Green Square" },
    //     { wptDefault, "Heliport" },
    //     { wptDefault, "Horn" },
    //     { wptDefault, "Hunting Area" },
    //     { ":/icons/wpt/info15x15.png", "Information" },
    //     { wptDefault, "Levee" },
    //     { wptDefault, "Light" },
    //     { wptDefault, "Live Theater" },
    //     { wptDefault, "Lodging" },
    //     { wptDefault, "Hotel" },
    //     { wptDefault, "Man Overboard" },
    //     { wptDefault, "Marina" },
    //     { wptDefault, "Medical Facility" },
    //     { ":/icons/wpt/mile_marker15x15.png", "Mile Marker" },
    //     { wptDefault, "Military" },
    //     { wptDefault, "Mine" },
    //     { wptDefault, "Movie Theater" },
    //     { wptDefault, "Museum" },
    //     { wptDefault, "Navaid, Amber" },
    //     { wptDefault, "Navaid, Black" },
    //     { wptDefault, "Navaid, Blue" },
    //     { wptDefault, "Navaid, Green" },
    //     { wptDefault, "Navaid, Green/Red" },
    //     { wptDefault, "Navaid, Green/White" },
    //     { wptDefault, "Navaid, Orange" },
    //     { wptDefault, "Navaid, Red" },
    //     { wptDefault, "Navaid, Red/Green" },
    //     { wptDefault, "Navaid, Red/White" },
    //     { wptDefault, "Navaid, Violet" },
    //     { wptDefault, "Navaid, White" },
    //     { wptDefault, "Navaid, White/Green" },
    //     { wptDefault, "Navaid, White/Red" },
    //     { wptDefault, "Oil Field" },
    //     { wptDefault, "Parachute Area" },
    //     { wptDefault, "Park" },
    //     { ":/icons/wpt/parking15x15.png", "Parking Area" },
    //     { wptDefault, "Pharmacy" },
    //     { wptDefault, "Picnic Area" },
    //     { wptDefault, "Pizza" },
    //     { wptDefault, "Post Office" },
    //     { wptDefault, "Private Field" },
    //     { wptDefault, "Radio Beacon" },
    //     { wptDefault, "Red Diamond" },
    //     { wptDefault, "Red Square" },
    //     { ":/icons/wpt/house15x15.png", "Residence" },
    //     { ":/icons/wpt/house15x15.png", "House" },
    //     { wptDefault, "Restaurant" },
    //     { wptDefault, "Restricted Area" },
    //     { wptDefault, "Restroom" },
    //     { wptDefault, "RV Park" },
    //     { wptDefault, "Scales" },
    //     { wptDefault, "Scenic Area" },
    //     { wptDefault, "School" },
    //     { wptDefault, "Seaplane Base" },
    //     { wptDefault, "Shipwreck" },
    //     { wptDefault, "Shopping Center" },
    //     { wptDefault, "Short Tower" },
    //     { wptDefault, "Shower" },
    //     { wptDefault, "Skiing Area" },
    //     { wptDefault, "Skull and Crossbones" },
    //     { wptDefault, "Soft Field" },
    //     { ":/icons/wpt/stadium15x15.png", "Stadium" },
    //     { ":/icons/wpt/summit15x15.png", "Summit" },
    //     { wptDefault, "Swimming Area" },
    //     { wptDefault, "Tall Tower" },
    //     { wptDefault, "Telephone" },
    //     { ":/icons/wpt/toll15x15.png", "Toll Booth" },
    //     { wptDefault, "TracBack Point" },
    //     { wptDefault, "Trail Head" },
    //     { wptDefault, "Truck Stop" },
    //     { wptDefault, "Tunnel" },
    //     { wptDefault, "Ultralight Area" },
    //     { wptDefault, "Water Hydrant" }, /* new in MapSource V5 */
    //     { wptDefault, "Waypoint" },
    //     { wptDefault, "White Buoy" },
    //     { wptDefault, "White Dot" },
    //     { wptDefault, "Zoo" },

    { ":/icons/wpt/custom1.png", "Custom 1" },
    { ":/icons/wpt/custom2.png", "Custom 2" },
    { ":/icons/wpt/custom3.png", "Custom 3" },
    { ":/icons/wpt/custom4.png", "Custom 4" },
    { ":/icons/wpt/custom5.png", "Custom 5" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 6" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 7" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 8" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 9" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 10" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 11" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 12" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 13" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 14" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 15" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 16" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 17" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 18" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 19" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 20" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 21" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 22" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 23" },
    { ":/icons/wpt/custom15x15.bmp", "Custom 24" },

    //     { ":/icons/wpt/toll15x15.png", "Micro-Cache" },      /* icon for "Toll Booth" */
    //     { wptDefault, "Virtual cache" },     /* icon for "Scenic Area" */
    //     { ":/icons/wpt/box_blue15x15.png", "Multi-Cache" },    /* icon for "Stadium" */
    //     { wptDefault, "Unknown Cache" },     /* icon for "Information" */
    //     { wptDefault, "Locationless (Reverse) Cache" }, /* Icon for "Flag" */
    //     { wptDefault, "Post Office" },   /* Icon for "Post Office" */
    //     { wptDefault, "Event Cache" },   /* Icon for "Event" */
    //     { wptDefault, "Webcam Cache" },  /* Icon for "Live Theatre" */

    { ":/icons/wpt/flag_pin_red15x15.png", "Flag, Red" },
    { ":/icons/wpt/flag_pin_blue15x15.png", "Flag, Blue" },
    { ":/icons/wpt/flag_pin_green15x15.png", "Flag, Green" },
    { ":/icons/wpt/pin_red15x15.png", "Pin, Red" },
    { ":/icons/wpt/pin_blue15x15.png", "Pin, Blue" },
    { ":/icons/wpt/pin_green15x15.png", "Pin, Green" },
    { ":/icons/wpt/box_red15x15.png", "Block, Red" },
    { ":/icons/wpt/box_blue15x15.png", "Block, Blue" },
    { ":/icons/wpt/box_green15x15.png", "Block, Green" },
    //     { wptDefault, "Bike Trail" },
    //     { wptDefault, "Fishing Hot Spot Facility" },
    //     { wptDefault, "Police Station"},
    //     { wptDefault, "Ski Resort" },
    //     { wptDefault, "Ice Skating" },
    //     { wptDefault, "Wrecker" },
    //     { wptDefault, "Anchor Prohibited" },
    //     { wptDefault, "Beacon" },
    //     { wptDefault, "Coast Guard" },
    //     { wptDefault, "Reef" },
    //     { wptDefault, "Weed Bed" },
    //     { wptDefault, "Dropoff" },
    //     { wptDefault, "Dock" },
    //
    //     { wptDefault, "Asian Food" },
    //     { wptDefault, "Blue Circle" },
    //     { wptDefault, "Blue Diamond" },
    //     { wptDefault, "Blue Letter A" },
    //     { wptDefault, "Blue Letter B" },
    //     { ":/icons/wpt/letter_c_blue15x15.png", "Blue Letter C" },
    //     { wptDefault, "Blue Letter D" },
    //     { wptDefault, "Blue Number 0" },
    //     { wptDefault, "Blue Number 1" },
    //     { wptDefault, "Blue Number 2" },
    //     { wptDefault, "Blue Number 3" },
    //     { wptDefault, "Blue Number 4" },
    //     { wptDefault, "Blue Number 5" },
    //     { wptDefault, "Blue Number 6" },
    //     { wptDefault, "Blue Number 7" },
    //     { wptDefault, "Blue Number 8" },
    //     { wptDefault, "Blue Number 9" },
    //     { wptDefault, "Blue Oval" },
    //     { wptDefault, "Blue Rectangle" },
    //     { wptDefault, "Blue Square" },
    //     { ":/icons/wpt/triangle_blue15x15.png", "Blue Triangle" },
    //     { wptDefault, "Border Crossing (Port Of Entry)" },
    //     { wptDefault, "Bottom Conditions" },
    //     { wptDefault, "Deli" },
    //     { wptDefault, "Elevation point" },
    //     { wptDefault, "Exit without services" },
    //     { wptDefault, "First approach fix" },
    //     { wptDefault, "Gambling/casino" },
    //     { wptDefault, "Geographic place name, land" },
    //     { wptDefault, "Geographic place name, Man-made" },
    //     { wptDefault, "Geographic place name, water" },
    //     { wptDefault, "Green circle" },
    //     { wptDefault, "Green Letter A" },
    //     { wptDefault, "Green Letter B" },
    //     { wptDefault, "Green Letter C" },
    //     { wptDefault, "Green Letter D" },
    //     { wptDefault, "Green Number 0" },
    //     { wptDefault, "Green Number 1" },
    //     { wptDefault, "Green Number 2" },
    //     { wptDefault, "Green Number 3" },
    //     { wptDefault, "Green Number 4" },
    //     { wptDefault, "Green Number 5" },
    //     { wptDefault, "Green Number 6" },
    //     { wptDefault, "Green Number 7" },
    //     { wptDefault, "Green Number 8" },
    //     { wptDefault, "Green Number 9" },
    //     { wptDefault, "Green Oval" },
    //     { wptDefault, "Green Rectangle" },
    //     { ":/icons/wpt/triangle_green15x15.png", "Green Triangle" },
    //     { wptDefault, "Intersection" },
    //     { wptDefault, "Intl freeway hwy" },
    //     { wptDefault, "Intl national hwy" },
    //     { wptDefault, "Italian food" },
    //     { wptDefault, "Large exit without services" },
    //     { wptDefault, "Large Ramp intersection" },
    //     { wptDefault, "Localizer Outer Marker" },
    //     { wptDefault, "Missed approach point" },
    //     { wptDefault, "Non-directional beacon" },
    //     { wptDefault, "Null" },
    //     { wptDefault, "Open 24 Hours" },
    //     { wptDefault, "Ramp intersection" },
    //     { wptDefault, "Red circle" },
    //     { wptDefault, "Red Letter A" },
    //     { wptDefault, "Red Letter B" },
    //     { wptDefault, "Red Letter C" },
    //     { wptDefault, "Red Letter D" },
    //     { wptDefault, "Red Number 0" },
    //     { wptDefault, "Red Number 1" },
    //     { wptDefault, "Red Number 2" },
    //     { wptDefault, "Red Number 3" },
    //     { wptDefault, "Red Number 4" },
    //     { wptDefault, "Red Number 5" },
    //     { wptDefault, "Red Number 6" },
    //     { wptDefault, "Red Number 7" },
    //     { wptDefault, "Red Number 8" },
    //     { wptDefault, "Red Number 9" },
    //     { wptDefault, "Red Oval" },
    //     { wptDefault, "Red Rectangle" },
    //     { ":/icons/wpt/triangle_red15x15.png", "Red Triangle" },
    //     { wptDefault, "Seafood" },
    //     { wptDefault, "State Hwy" },
    //     { wptDefault, "Steak" },
    //     { wptDefault, "Street Intersection" },
    //     { wptDefault, "TACAN" },
    //     { wptDefault, "Tide/Current PRediction Station" },
    //     { wptDefault, "U Marina" },
    //     { wptDefault, "US hwy" },
    //     { wptDefault, "U stump" },
    //     { wptDefault, "VHF Omni-range" },
    //     { wptDefault, "VOR-DME" },
    //     { wptDefault, "VOR/TACAN" },
    { 0, 0 },
};

void initWptIcons()
{
    QSettings cfg;

    setWptIconByName("Custom 1", cfg.value("garmin/icons/custom1", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 2", cfg.value("garmin/icons/custom2", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 3", cfg.value("garmin/icons/custom3", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 4", cfg.value("garmin/icons/custom4", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 5", cfg.value("garmin/icons/custom5", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 6", cfg.value("garmin/icons/custom6", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 7", cfg.value("garmin/icons/custom7", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 8", cfg.value("garmin/icons/custom8", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 9", cfg.value("garmin/icons/custom9", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 10", cfg.value("garmin/icons/custom10", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 11", cfg.value("garmin/icons/custom11", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 12", cfg.value("garmin/icons/custom12", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 13", cfg.value("garmin/icons/custom13", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 14", cfg.value("garmin/icons/custom14", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 15", cfg.value("garmin/icons/custom15", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 16", cfg.value("garmin/icons/custom16", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 17", cfg.value("garmin/icons/custom17", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 18", cfg.value("garmin/icons/custom18", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 19", cfg.value("garmin/icons/custom19", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 20", cfg.value("garmin/icons/custom20", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 21", cfg.value("garmin/icons/custom21", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 22", cfg.value("garmin/icons/custom22", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 23", cfg.value("garmin/icons/custom23", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 24", cfg.value("garmin/icons/custom24", ":/icons/wpt/custom15x15.bmp").toString());

}


QPixmap loadIcon(const QString& path)
{
    QFileInfo finfo(path);
    if(finfo.completeSuffix() != "bmp") {
        return QPixmap(path);
    }
    else {
        QImage img = QPixmap(path).toImage().convertToFormat(QImage::Format_Indexed8);
        img.setColor(0,qRgba(0,0,0,0));
        return QPixmap::fromImage(img);
    }

    return QPixmap();
}


QPixmap getWptIconByName(const QString& name, QString * src)
{
    const wpt_icon_t * ptr = wptIcons;
    while(ptr->name != 0) {
        if(ptr->name == name) {
            if(src) *src = ptr->icon;
            return loadIcon(ptr->icon);
        }

        ++ptr;
    }

    return QPixmap(wptDefault);
}


void setWptIconByName(const QString& name, const QString& filename)
{
    wpt_icon_t * ptr = wptIcons;
    while(ptr->name != 0) {
        if(ptr->name == name) {
            ptr->icon = filename;
        }

        ++ptr;
    }
}


QString getWptResourceByName(const QString& name)
{
    const wpt_icon_t * ptr = wptIcons;
    while(ptr->name != 0) {
        if(ptr->name == name) {
            return ptr->icon;
        }

        ++ptr;
    }

    return QString("");
}


const wpt_icon_t* getWptIcons()
{
    return wptIcons;
}

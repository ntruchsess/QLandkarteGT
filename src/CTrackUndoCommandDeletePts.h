/** ********************************************************************************************
    Copyright (c) 2009 Marc Feld

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************* */

#ifndef CTRACKUNDOCOMMANDDELETEPTS_T_H_
#define CTRACKUNDOCOMMANDDELETEPTS_T_H_
#include <QUndoCommand>
#include <QList>
#include "CTrack.h"

class CTrackUndoCommandDeletePts : public QUndoCommand
{
    public:
        CTrackUndoCommandDeletePts(CTrack *track);
        virtual ~CTrackUndoCommandDeletePts();
        virtual void undo();
        virtual void redo();
    private:
        CTrack *track;
        QList<CTrack::pt_t> oldList;
};
#endif                           /* CTRACKUNDOCOMMANDDELETEPT_TS_H_ */

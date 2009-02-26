/***************************************************************************
 *   Copyright (C) 2008-2009 by Чернов А.А.                                *
 *   valexlin@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _command_h
#define _command_h

#include <QQueue>

class GCameraCommand
{
public:
	GCameraCommand();
	GCameraCommand(int cmd, int p1, int p2, bool internal = false);
	GCameraCommand(const GCameraCommand& cmdobj);
	GCameraCommand& operator=(const GCameraCommand& cmdobj);
	int command() const { return Command; }
	int param1() const { return Param1; }
	int param2() const { return Param2; }
	bool isInternal() const { return Internal; }
private:
	int Command;
	int Param1;
	int Param2;
	bool Internal;
};

typedef QQueue<GCameraCommand> GCameraCommandsQueue;

#define COMMAND_SET_WB		0
#define COMMAND_SET_AV		1
#define COMMAND_REQ_AV		2
#define COMMAND_REQ_AVLIST	3
#define COMMAND_REQ_EVF_OUT	4
#define COMMAND_REQ_AEMODE	5
#define COMMAND_ADJ_FOCUS	6
#define COMMAND_REQ_AFMODE	7

#endif	// _command_h

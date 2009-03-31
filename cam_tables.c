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

#include "cam_tables.h"

struct EOSAvTable AvTable[EOS_AV_TABLE_SZ] = {
	{ 0x00, "0" },
	{ 0x08, "1" },
	{ 0x0B, "1.1" },
	{ 0x0C, "1.2" },
	{ 0x0D, "1.2" },	// (1/3)
	{ 0x10, "1.4" },
	{ 0x13, "1.6" },
	{ 0x14, "1.8" },
	{ 0x15, "1.8" },	// (1/3)
	{ 0x18, "2" },
	{ 0x1B, "2.2" },
	{ 0x1C, "2.5" },
	{ 0x1D, "2.5" },	// (1/3)
	{ 0x20, "2.8" },
	{ 0x23, "3.2" },
	{ 0x24, "3.5" },
	{ 0x25, "3.5" },	// (1/3)
	{ 0x28, "4" },
	{ 0x2B, "4.5" },
	{ 0x2C, "4.5" },	// (1/3)
	{ 0x2D, "5.0" },
	{ 0x30, "5.6" },
	{ 0x33, "6.3" },
	{ 0x34, "6.7" },
	{ 0x35, "7.1" },
	{ 0x38, "8" },
	{ 0x3B, "9" },
	{ 0x3C, "9.5" },
	{ 0x3D, "10" },
	{ 0x40, "11" },
	{ 0x43, "13" },
	{ 0x44, "13" },
	{ 0x45, "14" },
	{ 0x48, "16" },
	{ 0x4B, "18" },
	{ 0x4C, "19" },
	{ 0x4D, "20" },
	{ 0x50, "22" },
	{ 0x53, "25" },
	{ 0x54, "27" },
	{ 0x55, "29" },
	{ 0x58, "32" },
	{ 0x5B, "36" },
	{ 0x5C, "38" },
	{ 0x5D, "40" },
	{ 0x60, "45" },
	{ 0x63, "51" },
	{ 0x64, "54" },
	{ 0x65, "57" },
	{ 0x68, "64" },
	{ 0x6B, "72" },
	{ 0x6C, "76" },
	{ 0x6D, "80" },
	{ 0x70, "91" },
	{ 0xffffffff, "Err" }
};

struct EOSTvTable TvTable[EOS_TV_TABLE_SZ] = {
	//{ 0x0C, "Bulb" },
	{ 0x10, "30\"" },
	{ 0x13, "25\"" },
	{ 0x14, "20\"" },
	{ 0x15, "20\"" },
	{ 0x18, "15\"" },
	{ 0x1B, "13\"" },
	{ 0x1C, "10\"" },
	{ 0x1D, "10\"" },	// (1/3)
	{ 0x20, "8\"" },
	{ 0x23, "6\"" },	// (1/3)
	{ 0x24, "6\"" },
	{ 0x25, "5\"" },
	{ 0x28, "4\"" },
	{ 0x2B, "3\"2" },
	{ 0x2C, "3\"" },
	{ 0x2D, "2\"5" },
	{ 0x30, "2\"" },
	{ 0x33, "1\"6" },
	{ 0x34, "1\"5" },
	{ 0x35, "1\"3" },
	{ 0x38, "1\"" },
	{ 0x3B, "0\"8" },
	{ 0x3C, "0\"7" },
	{ 0x3D, "0\"6" },
	{ 0x40, "0\"5" },
	{ 0x43, "0\"4" },
	{ 0x44, "0\"3" },
	{ 0x45, "0\"3" },	// (1/3)
	{ 0x48, "1/4" },
	{ 0x4B, "1/5" },
	{ 0x4C, "1/6" },
	{ 0x4D, "1/6" },	// (1/3)
	{ 0x50, "1/8" },
	{ 0x53, "1/10" },	// (1/3)
	{ 0x54, "1/10" },
	{ 0x55, "1/13" },
	{ 0x58, "1/15" },
	{ 0x5B, "1/20" },	// (1/3)
	{ 0x5C, "1/20" },
	{ 0x5D, "1/25" },
	{ 0x60, "1/30" },
	{ 0x63, "1/40" },
	{ 0x64, "1/45" },
	{ 0x65, "1/50" },
	{ 0x68, "1/60" },
	{ 0x6B, "1/80" },
	{ 0x6C, "1/90" },
	{ 0x6D, "1/100" },
	{ 0x70, "1/125" },
	{ 0x73, "1/160" },
	{ 0x74, "1/180" },
	{ 0x75, "1/200" },
	{ 0x78, "1/250" },
	{ 0x7B, "1/320" },
	{ 0x7C, "1/350" },
	{ 0x7D, "1/400" },
	{ 0x80, "1/500" },
	{ 0x83, "1/640" },
	{ 0x84, "1/750" },
	{ 0x85, "1/800" },
	{ 0x88, "1/1000" },
	{ 0x8B, "1/1250" },
	{ 0x8C, "1/1500" },
	{ 0x8D, "1/1600" },
	{ 0x90, "1/2000" },
	{ 0x94, "1/3000" },
	{ 0x95, "1/3200" },
	{ 0x98, "1/4000" },
	{ 0x9B, "1/5000" },
	{ 0x9C, "1/6000" },
	{ 0x9D, "1/6400" },
	{ 0xA0, "1/8000" },
	{ 0xffffffff, "Err" }
};

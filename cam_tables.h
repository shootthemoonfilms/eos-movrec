/***************************************************************************
 *   Copyright (C) 2008-2010 by Чернов А.А.                                *
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

#ifndef _cam_tables_h
#define _cam_tables_h

#ifdef __cplusplus
extern "C" {
#endif

struct EOSAvTable
{
	unsigned int edsdk_val;
	char av[4];
};

struct EOSTvTable
{
	unsigned int edsdk_val;
	char tv[8];
};

struct EOSISOTable
{
	unsigned int edsdk_val;
	char ISO[6];
};

struct EOSAEMTable
{
	unsigned int edsdk_val;
	char gphoto_str[20];
	char aem[6];
};

#define EOS_AV_TABLE_SZ		55
#define EOS_TV_TABLE_SZ		73
#define EOS_ISO_TABLE_SZ	22
#define EOS_AEM_TABLE_SZ	6

extern struct EOSAvTable AvTable[];
extern struct EOSTvTable TvTable[];
extern struct EOSISOTable ISOTable[];
extern struct EOSAEMTable AEMTable[];

int findAV_edsdk(int edsdk_val);
int findAV_str(const char* str);
int findTV_edsdk(int edsdk_val);
int findTV_str(const char* str);
int findISO_edsdk(int edsdk_val);
int findISO_str(const char* str);
int findAEM_edsdk(int edsdk_val);
int findAEM_str(const char* str);

#ifdef __cplusplus
}
#endif

#endif	// _cam_tables_h

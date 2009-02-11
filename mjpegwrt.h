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

/* Work with one file ONLY in ONE thread!!!
*/

#ifndef _mjpegwrt_h
#define _mjpegwrt_h

#ifdef __cplusplus
extern "C" {
#endif

void* mjpegCreateFile(const char* fname);
int mjpegSetup(void* rf, int fwidth, int fheight, double fps, int quality);
int mjpegSetCache(void* p, int sz);
int mjpegSetMaxChunkSize(void* rf, unsigned int sz);
int mjpegWriteChunk(void* rf, const unsigned char* jpeg_data, unsigned int size);
int mjpegCloseFile(void* rf);

#ifdef __cplusplus
}
#endif

#endif	// _mjpegwrt_h

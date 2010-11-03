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

#include "mjpegwrt.h"

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#ifdef __GLIBC__
#include <unistd.h>
#endif

#ifndef O_BINARY
#define O_BINARY	0
#endif

#if defined(WIN32) || defined (_WIN32)
#define fsync _commit
#endif

#define HEADERBYTES 0x1000			// 4k
#define DEFCACHE_SZ	0x100000		// 1M

#define MAXSOFTDESC_SZ	30
#define MAXCOMMENT_SZ	64
#define MAXDATE_SZ		22

#pragma pack(push, 2)

struct RiffChunk
{
	__uint32_t ckID;				// fourcc
	__uint32_t ckSize;				// size of chunk data
	unsigned char *ckData;			// chunk data
};

// size - 56
struct AVIHeader
{
	__uint32_t dwMicroSecPerFrame;
	__uint32_t dwMaxBytesPerSec;	//
	__uint32_t dwReserved1;			// must be 0
	__uint32_t dwFlags;				// 0 ?
	__uint32_t dwTotalFrames;
	__uint32_t dwInitialFrames;		// here must be 0
	__uint32_t dwStreams;			// number of streams
	__uint32_t dwSuggestedBufferSize;
	__uint32_t dwWidth;				// width of frame
	__uint32_t dwHeight;			// height of frame
	__uint32_t dwReserved[4];		// all must be 0
};

// size - 56
struct AVIStreamHeader
{
	__uint32_t fccType;				// 'vids'
	__uint32_t fccHandler;			// 'mjpg'
	__uint32_t dwFlags;				// here 0
	__uint16_t wPriority;			// here 0
	__uint16_t wLanguage;			// here 0
	__uint32_t dwInitialFrames;		// here 0
	__uint32_t dwScale;				// dwMicroSecPerFrame
	__uint32_t dwRate;				// 1000000
	__uint32_t dwStart;				// here 0
	__uint32_t dwLength;			// dwTotalFrames
	__uint32_t dwSuggestedBufferSize;	//  size largest chunk in the stream
	__uint32_t dwQuality;			// from 0 to 10000
	__uint32_t dwSampleSize;		// here 0
	struct							// here all field zero
	{
		__uint16_t left;
		__uint16_t top;
		__uint16_t right;
		__uint16_t bottom;
	} rcFrame;
};

// size - 40
struct AVIStreamFormat
{
	__uint32_t biSize;				// must be 40
	__int32_t   biWidth;			// width of frame
	__int32_t   biHeight;			// height of frame
	__uint16_t biPlanes;			// here must be 1
	__uint16_t biBitCount;			// here must be 24
	__uint32_t biCompression;		// here 'MJPG' or 0x47504A4D
	__uint32_t biSizeImage;			// size, in bytes, of the image (in D90_orig.avi 2764800)
	__int32_t   biXPelsPerMeter;	// here 0
	__int32_t   biYPelsPerMeter;	// here 0
	__uint32_t biClrUsed;			// here 0
	__uint32_t biClrImportant;		// here 0
};

struct AVIIndexChunk
{
	__uint32_t ckID;
	__uint32_t flags;				// unknown?
	__uint32_t offset;				// offset from 'movi' to video frame chunk
	__uint32_t size;				// size of video frame chunk
};

#pragma pack(pop)

typedef struct
{
	int fd;							// file descriptor
	__uint32_t realHeaderSize;
	__uint32_t frames;
	double fps;
	unsigned char* index;
	__uint32_t index_curpos;
	__uint32_t* pindex_real_size;
	__uint32_t index_size;
	unsigned char* header;
	__uint32_t *pFileSize;
	__uint32_t *pDataSize;
	struct AVIHeader* aviheader;
	struct AVIStreamHeader* avistreamheader;
	struct AVIStreamFormat* avistreamformat;
	char* pSoftStr;
	char* pCommentStr;
	char* pDateStr;
	unsigned char* cache;
	unsigned int cache_sz;
	unsigned int cache_pos;
} RIFFFILE;

void* mjpegCreateFile(const char* fname)
{
	int fd = open(fname, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return 0;

	RIFFFILE* riff = (RIFFFILE*)malloc(sizeof(RIFFFILE));
	if (!riff)
	{
		close(riff->fd);
		return 0;
	}
	memset(riff, 0, sizeof(RIFFFILE));
	riff->fd = fd;
	riff->header = (unsigned char*)malloc(HEADERBYTES);
	if (!riff->header)
	{
		close(riff->fd);
		free(riff);
		return 0;
	}
	memset(riff->header, 0, HEADERBYTES);

	riff->index_size = 0x100;
	riff->index = (unsigned char*)malloc(riff->index_size);
	if (!riff->index)
	{
		close(riff->fd);
		free(riff->header);
		free(riff);
		return 0;
	}

	riff->pFileSize = (__uint32_t*)(riff->header + 4);
	*riff->pFileSize = HEADERBYTES - 8;
	__uint32_t* pnum = 0;
	__uint32_t offset = 0;
	char* ptr = (char*)riff->header;					// addr = 0
	strncpy(ptr, "RIFF", 4);
	offset += 8;
	ptr = (char*)(riff->header + offset);				// addr = 8
	strncpy(ptr, "AVI LIST", 8);
	offset += 8;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 16
	*pnum = 40 + sizeof(struct AVIHeader) + sizeof(struct AVIStreamHeader) + sizeof(struct AVIStreamFormat);
	offset += 4;
	ptr = (char*)(riff->header + offset);				// addr = 20
	strncpy(ptr, "hdrlavih", 8);
	offset += 8;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 28
	*pnum = sizeof(struct AVIHeader);
	offset += 4;
	riff->aviheader = (struct AVIHeader*)(riff->header + offset);	// addr = 32
	riff->aviheader->dwStreams = 1;						// only video stream
	riff->aviheader->dwFlags = 0x110;					// has index & interlieved
	offset += sizeof(struct AVIHeader);
	ptr = (char*)(riff->header + offset);				// addr = 88
	strncpy(ptr, "LIST", 4);
	offset += 4;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 92
	*pnum = 20 + sizeof(struct AVIStreamHeader) + sizeof(struct AVIStreamFormat);
	offset += 4;
	ptr = (char*)(riff->header + offset);				// addr = 96
	strncpy(ptr, "strlstrh", 8);
	offset += 8;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 104
	*pnum = sizeof(struct AVIStreamHeader);
	offset += 4;
	riff->avistreamheader = (struct AVIStreamHeader*)(riff->header + offset);	// addr = 108
	riff->avistreamheader->fccType = 0x73646976;		// 'vids'
	riff->avistreamheader->fccHandler = 0x67706a6d;		// 'mjpg'
	offset += sizeof(struct AVIStreamHeader);
	ptr = (char*)(riff->header + offset);				// addr = 164
	strncpy(ptr, "strf", 4);
	offset += 4;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 168
	*pnum = sizeof(struct AVIStreamFormat);
	offset += 4;
	riff->avistreamformat = (struct AVIStreamFormat*)(riff->header + offset);	// addr = 172
	offset += sizeof(struct AVIStreamFormat);

	ptr = (char*)(riff->header + offset);				// addr = 212
	strncpy(ptr, "LIST", 4);
	offset += 4;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 216
	*pnum = 12 + MAXSOFTDESC_SZ + 8 + MAXCOMMENT_SZ + 8 + MAXDATE_SZ;	// ISFT, ICMT & ICRD
	offset += 4;
	ptr = (char*)(riff->header + offset);				// addr = 220
	strncpy(ptr, "INFOISFT", 8);
	offset += 8;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 228
	*pnum = MAXSOFTDESC_SZ;								// ISFT
	offset += 4;
	riff->pSoftStr = (char*)(riff->header + offset);	// addr = 232
	// fill this later
	offset += MAXSOFTDESC_SZ;
	ptr = (char*)(riff->header + offset);				// addr = 262
	strncpy(ptr, "ICMT", 4);
	offset += 4;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 266
	*pnum = MAXCOMMENT_SZ;								// ICMT
	offset += 4;
	riff->pCommentStr = (char*)(riff->header + offset);	// addr = 270
	// fill this later
	offset += MAXCOMMENT_SZ;

	ptr = (char*)(riff->header + offset);				// addr = 334
	strncpy(ptr, "ICRD", 4);
	offset += 4;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 338
	*pnum = MAXDATE_SZ;									// ICRD
	offset += 4;
	riff->pDateStr = (char*)(riff->header + offset);	// addr = 342
	// fill this later
	offset += MAXDATE_SZ;

	riff->realHeaderSize = offset;

	// JUNK chunk
	ptr = (char*)(riff->header + offset);				// addr = 364
	strncpy(ptr, "JUNK", 4);
	offset += 4;
	__uint32_t junk_size = HEADERBYTES - riff->realHeaderSize - 20;
	pnum = (__uint32_t*)(riff->header + offset);		// addr = 368
	*pnum = junk_size;
	offset += 4;

	ptr = (char*)(riff->header + HEADERBYTES - 12);		// addr = 4084
	strncpy(ptr, "LIST", 4);
	riff->pDataSize = (__uint32_t*)(riff->header + HEADERBYTES - 8);	// 4088
	*riff->pDataSize = 4;
	ptr += 8;											// addr = 4092
	strncpy(ptr, "movi", 4);

	// for index chunk
	riff->pindex_real_size = (__uint32_t*)(riff->index + 4);
	ptr = (char*)riff->index;
	strncpy(ptr, "idx1", 4);
	*riff->pindex_real_size = 0;
	riff->index_curpos = 8;
	*riff->pFileSize += 8;

	riff->cache_sz = DEFCACHE_SZ;
	riff->cache_pos = 0;
	riff->cache = (unsigned char*)malloc(riff->cache_sz);
	if (!riff->cache)
	{
		close(riff->fd);
		free(riff->header);
		free(riff->index);
		free(riff);
		return 0;
	}

	if (!write(riff->fd, riff->header, HEADERBYTES))
	{
		close(riff->fd);
		free(riff->header);
		free(riff->index);
		free(riff);
		return 0;
	}

	return (void*)riff;
}

static int cached_write(RIFFFILE* rf, const void* buf, unsigned int sz)
{
	if (!rf || !rf->cache)
		return -1;
	int res = 0;
	int inbuf_pos = 0;
	char* ptr = (char*)rf->cache + rf->cache_pos;
	char* buf_ptr = (char*)buf;
	if (rf->cache_pos + sz > rf->cache_sz)
	{
		inbuf_pos = rf->cache_sz - rf->cache_pos;
		memcpy(ptr, buf_ptr, inbuf_pos);
		res = write(rf->fd, rf->cache, rf->cache_sz);
		if (res > 0)
		{
			rf->cache_pos = 0;
			ptr = (char*)rf->cache;
		}
		else
			return res;

		buf_ptr += inbuf_pos;
		int full_count = (sz - inbuf_pos)/rf->cache_sz;
		if (full_count > 0)
		{
			unsigned int f_pos = full_count*rf->cache_sz;
			res = write(rf->fd, buf_ptr, f_pos);
			if (res != f_pos)
				return res;
			inbuf_pos += f_pos;
			buf_ptr += f_pos;
		}
	}
	memcpy(ptr, buf_ptr, sz - inbuf_pos);
	rf->cache_pos += sz - inbuf_pos;
	return sz;
}

static int cache_flush(RIFFFILE* rf)
{
	if (!rf || !rf->cache)
		return -1;
	if (rf->cache_pos > 0)
	{
		int res = write(rf->fd, rf->cache, rf->cache_pos);
		if (res > 0)
			rf->cache_pos = 0;
		fsync(rf->fd);
		return res;
	}
	return 0;
}

int mjpegSetup(void* p, int fwidth, int fheight, double fps, int quality)
{
	RIFFFILE* rf = (RIFFFILE*)p;
	if (!rf)
		return 0;
	rf->fps = fps;
	//memset(rf->aviheader, 0, sizeof(struct AVIHeader));
	rf->aviheader->dwMicroSecPerFrame = (__uint32_t)(1000000.0/fps);
	rf->aviheader->dwWidth = fwidth;
	rf->aviheader->dwHeight = fheight;
	//memset(rf->avistreamheader, 0, sizeof(struct AVIStreamHeader));
	rf->avistreamheader->dwScale = 1000;
	rf->avistreamheader->dwRate = (__uint32_t)(1000.0*fps);
	rf->avistreamheader->dwQuality = quality;
	//memset(rf->avistreamformat, 0, sizeof(struct AVIStreamFormat));
	rf->avistreamformat->biSize = 40;
	rf->avistreamformat->biWidth = fwidth;
	rf->avistreamformat->biHeight = fheight;
	rf->avistreamformat->biPlanes = 1;
	rf->avistreamformat->biBitCount = 24;
	rf->avistreamformat->biCompression = 0x47504A4D;			// 'MJPG'
	rf->avistreamformat->biSizeImage = fwidth*fheight*3;
	return 1;
}

int mjpegSetInfo(void* p, const char* software, const char* comment, const char* date)
{
	RIFFFILE* rf = (RIFFFILE*)p;
	if (!rf)
		return 0;
	if (software)
	{
		strncpy(rf->pSoftStr, software, MAXSOFTDESC_SZ - 1);
		rf->pSoftStr[MAXSOFTDESC_SZ - 1] = 0;
	}
	else
		rf->pSoftStr[0] = 0;
	if (comment)
	{
		strncpy(rf->pCommentStr, comment, MAXCOMMENT_SZ - 1);
		rf->pCommentStr[MAXCOMMENT_SZ - 1] = 0;
	}
	else
		rf->pCommentStr[0] = 0;
	if (date)
	{
		strncpy(rf->pDateStr, date, MAXDATE_SZ - 1);
		rf->pDateStr[MAXDATE_SZ - 1] = 0;
	}
	else
		rf->pDateStr[0] = 0;
	return 1;
}

int mjpegSetCache(void* p, int sz)
{
	RIFFFILE* rf = (RIFFFILE*)p;
	if (!rf)
		return 0;
	unsigned char* tmp = (unsigned char*)malloc(sz);
	if (!tmp)
		return 0;
	free(rf->cache);
	rf->cache = tmp;
	rf->cache_sz = sz;
	return 1;
}

int mjpegSetMaxChunkSize(void* p, unsigned int sz)
{
	RIFFFILE* rf = (RIFFFILE*)p;
	if (!rf)
		return 0;
	rf->aviheader->dwSuggestedBufferSize = sz;
	rf->aviheader->dwMaxBytesPerSec = (int)(sz*rf->fps) + 1;
	rf->avistreamheader->dwSuggestedBufferSize = sz;
	return 1;
}

int mjpegWriteChunk(void* p, const unsigned char* jpeg_data, unsigned int size)
{
	RIFFFILE* rf = (RIFFFILE*)p;
	if (!rf)
		return 0;
	if (!rf->index)
		return 0;
	if (rf->index_curpos + sizeof(struct AVIIndexChunk) > rf->index_size)
	{
		unsigned char* p = (unsigned char*)realloc(rf->index, rf->index_size + 0x100);
		if (p)
		{
			rf->index = p;
			rf->pindex_real_size = (__uint32_t*)(rf->index + 4);
			rf->index_size += 0x100;
		}
		else
		{
			printf("FATAL: Can't realloc memory for index chunk!\n");
			free(rf->index);
			rf->index = 0;
			return 0;
		}
	}

	char buff[9];
	strncpy(buff, "00dc", 4);
	__uint32_t* pnum = (__uint32_t*)(buff + 4);
	*pnum = (__uint32_t)size;
	if (cached_write(rf, buff, 8) != 8)
		return 0;
	if (cached_write(rf, jpeg_data, size) != size)
		return 0;
	if (size % 2 != 0)
	{
		char junk = 0;
		if (cached_write(rf, &junk, 1) != 1)
			return 0;
	}
	// fill index
	__uint32_t key_frame = (__uint32_t)(rf->fps);
	struct AVIIndexChunk* ick = (struct AVIIndexChunk*)(rf->index + rf->index_curpos);
	ick->ckID = 0x63643030;					// '00dc'
	ick->flags = (rf->frames % key_frame == 0) ? 0x10 : 0;
	ick->offset = *rf->pDataSize;
	ick->size = (__uint32_t)size;
	rf->index_curpos += sizeof(struct AVIIndexChunk);
	*rf->pindex_real_size += sizeof(struct AVIIndexChunk);
	// increase counters
	*rf->pDataSize += size + 8;
	*rf->pFileSize += size + 8 + sizeof(struct AVIIndexChunk);
	rf->frames++;
	if (size % 2 != 0)
	{
		*rf->pDataSize += 1;
		*rf->pFileSize += 1;
	}
	return 1;
}

int mjpegCloseFile(void* p)
{
	RIFFFILE* rf = (RIFFFILE*)p;
	if (!rf)
		return 0;

	int res = 1;
	// write index
	if (rf->index)
	{
		if (cached_write(rf, rf->index, *rf->pindex_real_size + 8) != *rf->pindex_real_size + 8)
			res = 0;
	}
	else
		res = 0;
	cache_flush(rf);
	// write modified header
	rf->aviheader->dwTotalFrames = rf->frames;
	rf->avistreamheader->dwLength = rf->frames;
	lseek(rf->fd, 0, SEEK_SET);
	if (write(rf->fd, rf->header, HEADERBYTES) != HEADERBYTES)
		res = 0;

	close(rf->fd);
	free(rf->header);
	if (rf->index)
		free(rf->index);
	if (rf->cache)
		free(rf->cache);
	free(rf);
	return res;
}

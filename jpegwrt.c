
#include <fcntl.h>
#include <stdio.h>
#include "jpegwrt.h"


void* jpegCreateFile(const char* fname)
{
	int fd = open(fname,  O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return 0;
}
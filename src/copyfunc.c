#include <stdio.h>
#include <unistd.h>
#include "md5.h"
#include "fadvise.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> 
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "copyfunc.h"


void copy_process_block (const void *buffer, size_t len, FILE* dststream);

extern unsigned int dSpeed;
void DrawProgressBar(double percent, unsigned int speed, char *file, char progresssign);

static inline void *
ptr_align (void const *ptr, size_t alignment)
{
	char const *p0 = ptr;
	char const *p1 = p0 + alignment - 1;
	return (void *) (p1 - (size_t) p1 % alignment);
}



bool rawcopy(char *src, char *dst, unsigned char **checksum)
{
	*checksum = NULL;
	FILE *fp, *fpdst;

	fp = fopen (src, "rb");
	if (fp == NULL)
	{
		printf("error on opening: %s", src);
		return false;
	}

	fpdst = fopen (dst, "wb");
	if (fp == NULL)
	{
		fclose(fp);
		printf("error on opening: %s", dst);
		return false;
	}


	

	fadvise (fp, FADVISE_SEQUENTIAL);
	char buffer[100];

	while(feof(fp)==0)
	{  
		int numr;
		if((numr=fread(buffer,1,100,fp))!=100)
		{
			if(ferror(fp)!=0)
			{
				printf("read file error.\n");
				return 1;
			}
			else if(feof(fp)!=0);
		}
		copy_process_block(buffer, numr, fpdst);
	} 
	DrawProgressBar(100, dSpeed, src, '=');


	if (fclose (fpdst) != 0)
	{
		fclose(fp);
		printf("error on close: %s", dst);
		return false;
	}
	if (fclose (fp) != 0)
	{
		printf("error on close: %s", src);
		return false;
	}
	return true;
}


bool md5copy(char *src, char *dst, unsigned char **checksum)
{

	unsigned char *ret = (unsigned char*)malloc(sizeof(unsigned char*)*16);
	if (ret==NULL)
	{
		printf("md5copy: Failed to alloc memory (node-checksum)\n");
		return false;
	}

	unsigned char bin_buffer_unaligned[20];
	unsigned char *bin_buffer = ptr_align (bin_buffer_unaligned, 4);


	FILE *fp, *fpdst;
	int err;

	fp = fopen (src, "rb");
	if (fp == NULL)
	{
	 printf("error on opening: %s", src);
	 return false;
	}

	if (dst != NULL)
	{
	 fpdst = fopen (dst, "wb");
	 if (fp == NULL)
	 {
		 fclose(fp);
		 printf("error on opening: %s", dst);
		 return false;
	 }
	}
	else
	{
		fpdst = NULL;
	}


	fadvise (fp, FADVISE_SEQUENTIAL);
	err = md5_stream(fp, fpdst, bin_buffer);
	if (err)
	{
		printf("error on copy: %s", src);
		fclose (fp);
		return false;
	}
	if (fpdst)
	{
		DrawProgressBar(100, dSpeed, src, '=');
	}
	else
		DrawProgressBar(100, dSpeed, src, '#');

	if (fclose (fpdst) != 0)
	{
		fclose(fp);
		printf("error on close: %s", dst);
		return false;
	}
	if (fclose (fp) != 0)
	{
		printf("error on close: %s", src);
		return false;
	}

	memcpy(ret, bin_buffer, 16);
	*checksum = ret;
	return true;
}

bool md5sum(char *src, unsigned char **checksum)
{
	return md5copy(src, NULL, checksum);
}
bool md5cmp(unsigned char *checksum1, unsigned char *checksum2)
{
	if (checksum1 != NULL && checksum2 != NULL)
		return (memcmp(checksum1, checksum2, 16) == 0) ? true : false;
	return false;
}
void md5print(unsigned char *checksum)
{
	int i,j;
	for (i = 0, j = 0; i < 16 ; ++i, j+=2)
	{
		printf("%02x", checksum[i]);
	}
}

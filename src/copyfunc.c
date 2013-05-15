#include <stdio.h>
#include <unistd.h>
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
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

bool hashcopy(char *src, char *dst, unsigned char **checksum, int (*hashfunc)(), int DIGEST_BYTES, int DIGEST_ALIGN)
{
	unsigned char *ret = (unsigned char*)malloc(sizeof(unsigned char*)*DIGEST_BYTES);
	if (ret==NULL)
	{
		printf("sha1copy: Failed to alloc memory (node-checksum)\n");
		return false;
	}
	
	unsigned char bin_buffer_unaligned[DIGEST_BYTES+DIGEST_ALIGN];
	unsigned char *bin_buffer = ptr_align (bin_buffer_unaligned, DIGEST_ALIGN);


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
	err = hashfunc(fp, fpdst, bin_buffer);
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
	{
		DrawProgressBar(100, dSpeed, src, '#');
	}

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

	memcpy(ret, bin_buffer, DIGEST_BYTES);
	*checksum = ret;
	return true;
}


bool md5copy(char *src, char *dst, unsigned char **checksum)
{
	return hashcopy(src, dst, checksum, md5_stream, 16, 4);
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



bool sha1copy(char *src, char *dst, unsigned char **checksum)
{
	//return false;
	return hashcopy(src, dst, checksum, sha1_stream, 20, 4);
}

bool sha1sum(char *src, unsigned char **checksum)
{
	return sha1copy(src, NULL, checksum);
}
bool sha1cmp(unsigned char *checksum1, unsigned char *checksum2)
{
	if (checksum1 != NULL && checksum2 != NULL)
		return (memcmp(checksum1, checksum2, 20) == 0) ? true : false;
	return false;
}
void sha1print(unsigned char *checksum)
{
	int i,j;
	for (i = 0, j = 0; i < 20 ; ++i, j+=2)
	{
		printf("%02x", checksum[i]);
	}
}



bool sha224copy(char *src, char *dst, unsigned char **checksum)
{
	//return false;
	return hashcopy(src, dst, checksum, sha224_stream, 28, 4);
}

bool sha224sum(char *src, unsigned char **checksum)
{
	return sha224copy(src, NULL, checksum);
}
bool sha224cmp(unsigned char *checksum1, unsigned char *checksum2)
{
	if (checksum1 != NULL && checksum2 != NULL)
		return (memcmp(checksum1, checksum2, 28) == 0) ? true : false;
	return false;
}
void sha224print(unsigned char *checksum)
{
	int i,j;
	for (i = 0, j = 0; i < 28 ; ++i, j+=2)
	{
		printf("%02x", checksum[i]);
	}
}


bool sha256copy(char *src, char *dst, unsigned char **checksum)
{
	//return false;
	return hashcopy(src, dst, checksum, sha256_stream, 32, 4);
}

bool sha256sum(char *src, unsigned char **checksum)
{
	return sha256copy(src, NULL, checksum);
}
bool sha256cmp(unsigned char *checksum1, unsigned char *checksum2)
{
	if (checksum1 != NULL && checksum2 != NULL)
		return (memcmp(checksum1, checksum2, 32) == 0) ? true : false;
	return false;
}
void sha256print(unsigned char *checksum)
{
	int i,j;
	for (i = 0, j = 0; i < 32 ; ++i, j+=2)
	{
		printf("%02x", checksum[i]);
	}
}

bool sha384copy(char *src, char *dst, unsigned char **checksum)
{
	//return false;
	return hashcopy(src, dst, checksum, sha384_stream, 48, 8);
}

bool sha384sum(char *src, unsigned char **checksum)
{
	return sha384copy(src, NULL, checksum);
}
bool sha384cmp(unsigned char *checksum1, unsigned char *checksum2)
{
	if (checksum1 != NULL && checksum2 != NULL)
		return (memcmp(checksum1, checksum2, 48) == 0) ? true : false;
	return false;
}
void sha384print(unsigned char *checksum)
{
	int i,j;
	for (i = 0, j = 0; i < 48 ; ++i, j+=2)
	{
		printf("%02x", checksum[i]);
	}
}


bool sha512copy(char *src, char *dst, unsigned char **checksum)
{
	//return false;
	return hashcopy(src, dst, checksum, sha512_stream, 64, 8);
}

bool sha512sum(char *src, unsigned char **checksum)
{
	return sha512copy(src, NULL, checksum);
}
bool sha512cmp(unsigned char *checksum1, unsigned char *checksum2)
{
	if (checksum1 != NULL && checksum2 != NULL)
		return (memcmp(checksum1, checksum2, 64) == 0) ? true : false;
	return false;
}
void sha512print(unsigned char *checksum)
{
	int i,j;
	for (i = 0, j = 0; i < 64 ; ++i, j+=2)
	{
		printf("%02x", checksum[i]);
	}
}

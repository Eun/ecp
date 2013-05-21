/* This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* Written by Tobias Salzmann <tobias@su.am>, 2013.  */

#include <stdio.h>
#include <unistd.h>
#include "fadvise.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> 
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "copyfunc.h"




struct HASHALGOCOL
{
	char name[10];
	bool (* copyfunc)();
	bool (* sumfunc)();
	bool (* cmpfunc)();
	void (* printfunc)();
} m_hashalgocol[] = {
	{"NONE", rawcopy, NULL, NULL, NULL},
	//{"CRC32", rawcopy, NULL, NULL, NULL},
	{"MD5", md5copy, md5sum, md5cmp, md5print},
	{"SHA1", sha1copy, sha1sum, sha1cmp, sha1print},
	{"SHA224", sha224copy, sha224sum, sha224cmp, sha224print},
	{"SHA256", sha256copy, sha256sum, sha256cmp, sha256print},
	{"SHA384", sha384copy, sha384sum, sha384cmp, sha384print},
	{"SHA512", sha512copy, sha512sum, sha512cmp, sha512print}
};

void DrawProgressBar(double percent, unsigned int speed, char *file, char progresssign);
char *basename(char *in, int len);
char *dirname(char *in, int len);
int getWidth();

unsigned long nChecksumErrors;
unsigned long nTotalFilesRead;
unsigned long nTotalFilesCopied;
bool bCreateEmptyDir;
struct HASHALGOCOL* pHashAlgo;

char *pDestinationPath;

unsigned int fSize;
unsigned int wSize;
unsigned int dSpeed;
char *ffile;
pthread_t tSpeadMeassure;

static void *SpeedMeasure(void *arg)
{
	unsigned int diff = 0;
	dSpeed = 0;
	pthread_t myID = pthread_self();
	while(wSize < fSize && tSpeadMeassure == myID)
	{
		dSpeed = wSize-diff;
		diff = wSize;
		usleep(1000000);
	}
	return NULL;
}


void copy_process_block (const void *buffer, size_t len, FILE* dststream)
{
	if (dststream != NULL)
	{
		if (fwrite(buffer, len, 1, dststream) < 0 )
		{
			printf("error on wrting to destination\n");
		}
	}
	if (ffile)
	{
		wSize+=len;
		if (dststream == NULL)
			DrawProgressBar((double)100*wSize/fSize, dSpeed, ffile, '#');
		else
			DrawProgressBar((double)100*wSize/fSize, dSpeed, ffile, '=');
	}

}
void copy_process_bytes (const void *buffer, size_t len, FILE* dststream)
{
	copy_process_block(buffer, len, dststream);
}



bool mkdir_p(char *folder, long mode)
{
	char *s = folder;
	while (1) 
	{
		char c;
		if (*s == '/' || *s == '\0')
		{
			c = *s;
			*s = '\0';
			struct stat st;
			if (stat(folder,&st) == -1)
			{
				if (mkdir(folder, mode) != 0)
				{
					*s = '/';
					return false;
				}
			}
			
			if (c == '\0')
			{
				return true;
			}
			*s = '/';
		}
		s++;
	}
	return true;
}



struct DataNode
{
	unsigned char *checksum;
	char *source;
	char *destination;
	struct DataNode *next;
};

void FreeNodeContents(struct DataNode *node)
{
	if (node->source != NULL)
	{
		free(node->source);
		node->source = NULL;
	}
	if (node->destination != NULL)
	{
		free(node->destination);
		node->destination = NULL;
	}
	if (node->checksum != NULL)
	{
		free(node->checksum);
		node->checksum = NULL;
	}
}

void JumpAndFree(struct DataNode **node, bool freeNode)
{
	struct DataNode *nnode = (*node)->next;
	FreeNodeContents(*node);
	if (freeNode)
		free(*node);
	*node = nnode;
}

void DoWork(struct DataNode *node)
{
	struct DataNode *firstNode = node;
	while (node)
	{
		if (!node->destination || !node->source)
		{
			JumpAndFree(&node, false);
			continue;
		}

		nTotalFilesRead++;
		struct stat fileDestination;
		struct stat fileSource;
		if (stat(node->source, &fileSource) == -1)
		{
			JumpAndFree(&node, false);
			continue;
		}
		if (stat(node->destination, &fileDestination) == 0 && S_ISREG(fileDestination.st_mode))
		{
		 if (fileDestination.st_size == fileSource.st_size)
		 {
			unsigned char *checksum;
			ffile = node->source;
			fSize = (unsigned int)fileSource.st_size;
			dSpeed = fSize;
			wSize = 0;
			pthread_create(&tSpeadMeassure, NULL, &SpeedMeasure, NULL);
			pHashAlgo->sumfunc(node->source, &(node->checksum)); 
			ffile = node->destination;
			dSpeed = fSize;
			wSize = 0;
			pthread_create(&tSpeadMeassure, NULL, &SpeedMeasure, NULL);
			pHashAlgo->sumfunc(node->destination, &checksum); 
			if (pHashAlgo->cmpfunc(checksum, node->checksum))
			{
				 free(checksum);
				 DrawProgressBar(100, fileDestination.st_size, ffile, '-');
				 JumpAndFree(&node, false);
				 continue; 
			}
			free(checksum);
		 }
		}

		// check if the destination folder exists
		char *destfolder = dirname(node->destination, strlen(node->destination));

		if (stat(destfolder, &fileDestination) == -1)
		{
			if (mkdir_p(destfolder, 0777) == 0)
			{
				printf("skipping '%s' could not create destination folder\n", node->source);
			}
		}
		free (destfolder);

		if (node->checksum == NULL && pHashAlgo->copyfunc != NULL)
		{
			ffile = node->source;
			fSize = (unsigned int)fileSource.st_size;
			dSpeed = fSize;
			wSize = 0;
			pthread_create(&tSpeadMeassure, NULL, &SpeedMeasure, NULL);
			pHashAlgo->copyfunc(node->source, node->destination, &(node->checksum)); 
		}
		nTotalFilesCopied++;
		node = node->next;
	}
	// after copying, check everything
	node = firstNode;
	while (node)
	{
		if (node->checksum != NULL && pHashAlgo->sumfunc != NULL)
		{
			struct stat fileDestination;
			unsigned char *checksum;
	
			if (stat(node->destination, &fileDestination) == 0 && S_ISREG(fileDestination.st_mode))
			{
				ffile = node->destination;
				fSize = (unsigned int)fileDestination.st_size;
				dSpeed = fSize;
				wSize = 0;
				pthread_create(&tSpeadMeassure, NULL, &SpeedMeasure, NULL);
				pHashAlgo->sumfunc(node->destination, &checksum); 
			}
			if (pHashAlgo->cmpfunc(node->checksum, checksum) == false)
			{
				printf("Mismatching Checksum! '%s' (", node->source);
				if (pHashAlgo->printfunc)
					pHashAlgo->printfunc(node->checksum);
				printf(") not matches '%s' (",node->destination);
				if (pHashAlgo->printfunc)
					pHashAlgo->printfunc(checksum);
				printf(")\n");
				nChecksumErrors++;
			}
			free(checksum);
		}
		JumpAndFree(&node, true);
	}
}



struct DataNode* CreateNode(char *src, int lsrc, char *dst, int ldst)
{
	struct DataNode *node = (struct DataNode*)malloc(sizeof(struct DataNode));
	node->source = (char*)malloc(sizeof(char*)*(lsrc+1));
	if (node->source==NULL)
	{
		printf("CreateNode: Failed to alloc memory (source path)\n");
		return NULL;
	}
	memcpy(node->source, src, lsrc);
	node->source[lsrc] = 0;
	node->destination = (char*)malloc(sizeof(char*)*(ldst+1));
	if (node->destination==NULL)
	{
		printf("CreateNode: Failed to alloc memory (destination path)\n");
		return NULL;
	}
	memcpy(node->destination, dst, ldst);
	node->destination[ldst] = 0;
	node->next = NULL;
	node->checksum = NULL;
	return node;
}

void AddFolder(char* src, int lsrc, char *dst, int ldst)
{
	struct DataNode *firstNode = NULL;
	struct DataNode *node;
	DIR           *d;
	struct dirent *dir;
	bool bAdded = false;

	d = opendir(src);
	if (d)
	{
		char *srcWithSlash;
		if (src[lsrc-1] != '/')
		{
			srcWithSlash = (char*)malloc(sizeof(char*)*(lsrc+2));

			if (srcWithSlash==NULL)
			{
				printf("AddFolder: Failed to alloc memory (source with slash)\n");
				closedir(d);
				return;
			}

			memcpy(srcWithSlash, src, lsrc);
			srcWithSlash[lsrc] = '/';
			srcWithSlash[++lsrc] = 0;
		}
		else
		{
			srcWithSlash = (char*)malloc(sizeof(char*)*(lsrc+1));

			if (srcWithSlash==NULL)
			{
				printf("AddFolder: Failed to alloc memory (source with slash)\n");
				closedir(d);
				return;
			}

			memcpy(srcWithSlash, src, lsrc);
			srcWithSlash[lsrc] = 0;
		}


		// loop trough all files 
		while ((dir = readdir(d)) != NULL)
		{
			int len2 = strlen(dir->d_name);


			if ((len2 == 1 && dir->d_name[0] == '.') || (len2 == 2 && dir->d_name[0] == '.' && dir->d_name[1] == '.'))
			{
				continue;
			}

			int lfullSourcePath = lsrc+len2;
			char *fullSourcePath = (char*)malloc(sizeof(char)*(lfullSourcePath+1));
			if (fullSourcePath==NULL)
			{
				printf("AddFolder: Failed to alloc memory (fullSourcePath)\n");
				closedir(d);
				return;
			}
			memcpy(fullSourcePath, srcWithSlash, lsrc); 
			memcpy(fullSourcePath+lsrc, dir->d_name, len2);
			fullSourcePath[lfullSourcePath] = 0;
			struct stat ft;
			if (stat(fullSourcePath, &ft) == 0 && S_ISREG(ft.st_mode))
			{
				unsigned char appendslash = 0;
				int lfullTargetPath = ldst+len2+1;
				char *fullTargetPath = (char*)malloc(sizeof(char)*(lfullTargetPath+1));
				if (fullTargetPath==NULL)
				{
					printf("AddFolder: Failed to alloc memory (fullTargetPath)\n");
					closedir(d);
					return;
				}
				memcpy(fullTargetPath, dst, ldst); 
				if (dst[ldst-1] != '/')
				{
					fullTargetPath[ldst] = '/';
					appendslash = 1;
				}
				else
				{
					lfullTargetPath--;
				}
				memcpy(fullTargetPath+ldst+appendslash, dir->d_name, len2);
				fullTargetPath[lfullTargetPath] = 0;

				struct DataNode *newnode = (struct DataNode*)malloc(sizeof(struct DataNode));
				if (newnode)
				{
					newnode->source = fullSourcePath;
					newnode->destination = fullTargetPath;
					newnode->checksum = NULL;
					newnode->next = NULL;
					if (firstNode == NULL)
						firstNode = newnode;
					else
						node->next = newnode;
					node = newnode;
					bAdded = true;
				}
				else
				{
					free(fullSourcePath);
					free(fullTargetPath);
				}
			}
			else
			{
				free(fullSourcePath);
			}
		}

		// do the work for the files
		//printf("DOWRK FOR FILES\n");
		DoWork(firstNode);

		rewinddir(d);
		// loop trough all folders
		while ((dir = readdir(d)) != NULL)
		{
			int len2 = strlen(dir->d_name);

			if ((len2 == 1 && dir->d_name[0] == '.') || (len2 == 2 && dir->d_name[0] == '.' && dir->d_name[1] == '.'))
			{
				continue;
			}

			int lfullSourcePath = lsrc+len2;
			char *fullSourcePath = (char*)malloc(sizeof(char)*(lfullSourcePath+1));
			if (fullSourcePath==NULL)
			{
				printf("AddFolder: Failed to alloc memory (fullSourcePath)\n");
				closedir(d);
				return;
			}
			memcpy(fullSourcePath, srcWithSlash, lsrc); 
			memcpy(fullSourcePath+lsrc, dir->d_name, len2);
			fullSourcePath[lfullSourcePath] = 0;
			struct stat ft;
			if (stat(fullSourcePath, &ft) == 0 && S_ISDIR(ft.st_mode))
			{
				unsigned char appendslash = 0;
				int lfullTargetPath = ldst+len2+1;
				char *fullTargetPath = (char*)malloc(sizeof(char)*(lfullTargetPath+1));
				if (fullTargetPath==NULL)
				{
					printf("AddFolder: Failed to alloc memory (fullTargetPath)\n");
					closedir(d);
					return;
				}
				memcpy(fullTargetPath, dst, ldst); 
				if (dst[ldst-1] != '/')
				{
					fullTargetPath[ldst] = '/';
					appendslash = 1;
				}
				else
				{
					lfullTargetPath--;
				}
				memcpy(fullTargetPath+ldst+appendslash, dir->d_name, len2);
				fullTargetPath[lfullTargetPath] = 0;
				AddFolder(fullSourcePath, lfullSourcePath, fullTargetPath, lfullTargetPath);
				free(fullTargetPath);
				bAdded = true;
			}
			free(fullSourcePath);

		}
		free(srcWithSlash);

		closedir(d);
	}
	if (!bAdded && bCreateEmptyDir)
	{
		struct stat fdst;
		if (stat(dst, &fdst) == -1)
		{
			mkdir_p(dst, 0777);
		}
	}
}


void sighandler( int sig )
{
	int i;
	putchar('\r');
	for (i = getWidth(); i > 0; i--)
			putchar(' ');
	printf("\rDone:  Files read:        %lu\n       Files copied:      %lu\n       Checksum errors:   %lu\n", nTotalFilesRead, nTotalFilesCopied, nChecksumErrors);
	free(pDestinationPath);
	exit(0);
}


// ecp a b => 		copy a (file) 					to b (file)
// ecp a b/ => 		copy a (file) 					in b (folder)
// ecp a/ b => 		copy a (folder) contents 		to b (folder)
// ecp a/ b/ => 	copy a (folder) contents 		in b (folder)
// ecp a b c =>		copy a (file) and b (file) 		in c (folder)
int main (int argc, char **argv)
{

	// test sums
	/*{
		int i;
		for (i = sizeof(m_hashalgocol) / sizeof(m_hashalgocol[0]) - 1; i >= 0; i--)
		{
			if (m_hashalgocol[i].sumfunc && m_hashalgocol[i].printfunc)
			{
				unsigned char *checksum;
				m_hashalgocol[i].sumfunc(argv[0], &checksum);
				printf("%s = ", m_hashalgocol[i].name);
				m_hashalgocol[i].printfunc(checksum);
				printf("\n");
				free(checksum);
			}
		}  
		return 0;
	}*/

	bCreateEmptyDir = false;
	nTotalFilesRead = nTotalFilesCopied = nChecksumErrors = 0;
	if (argc < 3)
	{
		char *exe = basename(argv[0], strlen(argv[0]));
		printf("usage: %s [OPTION] SOURCE... DESTINATION\n\nOPTION can be:\n -h=HASHALGO      decide which hash algorithm should be used.\n                    HASHALGO can be:\n                      NONE, MD5, SHA1, SHA224, SHA256, SHA384, SHA512.\n                    Default is MD5\n -d               create empty directorys\n", exe);
		free(exe);
		return 1;
	}

	pHashAlgo = &m_hashalgocol[1];

	while(*argv++)
	{
		argc--;
		if (*argv == NULL || (*argv)[0] != '-')
			break;
		
		if ((*argv)[1] == 'd')
		{
				bCreateEmptyDir = true;
		}
		else if ((*argv)[1] == 'h' && (*argv)[2] == '=')
		{
			char *ALGO = &(*argv)[3];
			int i;
			for (i = sizeof(m_hashalgocol) / sizeof(m_hashalgocol[0]) - 1; i >= 0; i--)
			{
				if (!strcasecmp(ALGO, m_hashalgocol[i].name))
				{
					*pHashAlgo = m_hashalgocol[i];
					break;
				}
			}  
		}
	}


	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	int numfiles = argc-1;
	char *target = argv[argc-1];
	int len = strlen(target);
	struct stat fdst;
	int dflags = stat(target, &fdst);
	bool appendslash = false;


	if (dflags == -1)
		dflags = 0;
	else
		dflags = fdst.st_mode;

	if (S_ISDIR(dflags))  // dest exists and is a folder
	{
		if (target[len-1] != '/')
		{
			appendslash = true;
		}
	}
	else if (dflags == 0) // dest does not exists
	{
		if (numfiles > 1)
		{
			// dest must be a folder
			if (mkdir_p(target, 0777) == 0)
			{
				printf("could not create %s\n", target);
				return 1;
			}

			if (target[len-1] != '/')
			{
				appendslash = true;
			}
		}
	}

	struct DataNode *firstNode = NULL;
	struct DataNode *node;

	while(*argv != target)
	{
		if (*argv != NULL)
		{
			int lsrc = strlen(*argv);
			char* bname = basename(*argv, lsrc);
			int lbname = strlen(bname);
			int lsFullPath = len+appendslash+lbname;
			char* sFullPath = (char*)malloc(sizeof(char*)*(lsFullPath+1));
			memcpy(sFullPath, target, len);
			if (appendslash)
			{
				sFullPath[len+appendslash-1] = '/';
			}
			memcpy(sFullPath+len+appendslash, bname, lbname);
			free(bname);

			sFullPath[lsFullPath] = 0;
			//printf("%s => %s\n", *argv, sFullPath);

			int dflags = stat(*argv, &fdst);
			if (dflags != -1)
			{

				dflags = fdst.st_mode;
				if (S_ISDIR(dflags))
				{
					AddFolder(*argv, lsrc, sFullPath, lsFullPath);
				}
				else if(S_ISREG(dflags))
				{
					//char *sdir = dirname(argv[i], len);
					struct DataNode *newnode = CreateNode(*argv, lsrc, sFullPath, lsFullPath);
					//free(sdir);
					if (newnode)
					{
						if (firstNode == NULL)
							firstNode = newnode;
						else
							node->next = newnode;
						node = newnode;
					}
				}
			}
			free(sFullPath);
		}
		*argv++;
	}

	DoWork(firstNode);
	sighandler(0);

	return 0;
}

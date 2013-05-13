#include <stdio.h>
#include <unistd.h>
#include "md5.h"
#include "fadvise.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> 
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


void DrawProgressBar(double percent, unsigned int speed, char *file, char progresssign);
char *basename(char *in, int len);
char *dirname(char *in, int len);




unsigned int fSize;
unsigned int wSize;
unsigned int dSpeed;
char *ffile;
pthread_t tSpeadMeassure;

static void *SpeedMeasure(void *arg)
{
  unsigned int diff = 0;
  dSpeed = 0;
  while(wSize < fSize && dSpeed != fSize)
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

static inline void *
ptr_align (void const *ptr, size_t alignment)
{
  char const *p0 = ptr;
  char const *p1 = p0 + alignment - 1;
  return (void *) (p1 - (size_t) p1 % alignment);
}



unsigned char rawcopy(char *src, int filesize, char *dst)
{

  FILE *fp, *fpdst;

  fp = fopen (src, "rb");
  if (fp == NULL)
  {
    printf("error on opening: %s", src);
    return 0;
  }

  fpdst = fopen (dst, "wb");
  if (fp == NULL)
  {
    fclose(fp);
    printf("error on opening: %s", dst);
    return 0;
  }

  fSize = (unsigned int)filesize;
  ffile = src;

  fadvise (fp, FADVISE_SEQUENTIAL);
  char buffer[100];
  dSpeed = fSize;
  pthread_create(&tSpeadMeassure, NULL, &SpeedMeasure, NULL);
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
    copy_process_block(buffer,numr, fpdst);
  } 
  DrawProgressBar(100, dSpeed, src, '=');


  if (fclose (fpdst) != 0)
  {
    fclose(fp);
    printf("error on close: %s", dst);
    return 0;
  }
  if (fclose (fp) != 0)
  {
    printf("error on close: %s", src);
    return 0;
  }
  return 1;
}


unsigned char md5copy(char *src, int filesize, char *dst, char *checksum)
{
   //__asm__("int3");
  unsigned char bin_buffer_unaligned[20];
  unsigned char *bin_buffer = ptr_align (bin_buffer_unaligned, 4);


  FILE *fp, *fpdst;
  int err;

  fp = fopen (src, "rb");
  if (fp == NULL)
  {
   printf("error on opening: %s", src);
   return 0;
  }

  if (dst != NULL)
  {
   fpdst = fopen (dst, "wb");
   if (fp == NULL)
   {
     fclose(fp);
     printf("error on opening: %s", dst);
     return 0;
   }
  }
  else
  {
    fpdst = NULL;
  }
  fSize = (unsigned int)filesize;
  wSize = 0;
  ffile = src;


fadvise (fp, FADVISE_SEQUENTIAL);
dSpeed = fSize;
pthread_create(&  tSpeadMeassure, NULL, &SpeedMeasure, NULL);
err = md5_stream(fp, fpdst, bin_buffer);
if (err)
{
  printf("error on copy: %s", src);
  fclose (fp);
  return 0;
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
  return 0;
}
if (fclose (fp) != 0)
{
  printf("error on close: %s", src);
  return 0;
}


int i;
int j = 0;
memset(checksum, 0, 32);
for (i = 0; i < 16 ; ++i, j+=2)
{
  sprintf(checksum+j,"%02x", bin_buffer[i]);
}

return 1;
}

unsigned char md5sum(char *src, int filesize, char *checksum)
{
  return md5copy(src, filesize, NULL, checksum);
}


unsigned char mkdir_p(char *folder, size_t len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    if (folder[i] == '/')
    {
      char *pst = (char*)malloc(sizeof(char)*(i+1));

      if (pst==NULL)
      {
        printf("mkdir_p: Failed to alloc memory (destination)\n");
        return 0;
      }

      memcpy(pst, folder, i);
      pst[i] = 0;
      struct stat st;
      if (stat(pst,&st) == -1)
      {
        if (mkdir(pst,0644) != 0)
        {
          free(pst);
          return 0;
        }
      }
      free(pst);
      i++;
    }
  }
  return 1;
}



char *TargetPreCheck(char *target, int numfiles)
{
  int len = strlen(target);
  
  struct stat FileType;
  if (stat(target,&FileType) == -1)
  {

    // ecp file1 file2 folder(/)
    if (numfiles > 1)
    {
      char *newtarget;

      if (target[len-1] != '/')
      {
        newtarget = (char*)malloc(sizeof(char)*(len+2));

        if (newtarget==NULL)
        {
          printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
          return NULL;
        }

        memcpy(newtarget, target, len);
        newtarget[len] = '/';
        newtarget[++len] = 0;
      }
      else
      {
       newtarget = (char*)malloc(sizeof(char)*(len+1));
       if (newtarget==NULL)
       {
         printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
         return NULL;
       }
       memcpy(newtarget, target, len);
       newtarget[len] = 0;

     }

     if (mkdir_p(newtarget, len) == 0)
     {
      free(newtarget);
      printf("could not create %s\n", target);
      return NULL;
    }

    return newtarget;
  }


    // ecl file target
  if (target[len-1] != '/')
  {
   if (mkdir_p(target, len) == 0)
   {
    printf("could not create %s\n", target);
    return NULL;
  }
  char *newtarget = (char*)malloc(sizeof(char)*(len+1));
  if (newtarget==NULL)
  {
    printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
    return NULL;
  }
  memcpy(newtarget, target, len);
  newtarget[len] = 0;
  return newtarget;
}
else
{
  if (mkdir_p(target, len) == 0)
  {
    printf("could not create %s\n", target);
    return NULL;
  }
  char *newtarget = (char*)malloc(sizeof(char)*(len+1));
  if (newtarget==NULL)
  {
    printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
    return NULL;
  }
  memcpy(newtarget, target, len);
  newtarget[len] = 0;
  return newtarget;
}
}
else if (S_ISDIR(FileType.st_mode)) // FOLDER
{
 if (target[len-1] != '/')
 {
  char *newtarget = (char*)malloc(sizeof(char)*(len+2));
  if (newtarget==NULL)
  {
    printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
    return NULL;
  }
  memcpy(newtarget, target, len);
  newtarget[len] = '/';
  newtarget[len+1] = 0;
  return newtarget;
}
else
{
  char *newtarget = (char*)malloc(sizeof(char)*(len+1));
  if (newtarget==NULL)
  {
    printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
    return NULL;
  }
  memcpy(newtarget, target, len);
  newtarget[len] = 0;
  return newtarget;
}
}  
else if (S_ISREG(FileType.st_mode)) // FILE
{
  if (numfiles > 1)
  {
    printf("destination '%s' is not a directory\n", target);
    return NULL;
  }
  else
  {
    char *newtarget = (char*)malloc(sizeof(char)*(len+1));
    if (newtarget==NULL)
    {
      printf("TargetPreCheck: Failed to alloc memory (newtarget)\n");
      return NULL;
    }
    memcpy(newtarget, target, len);
    newtarget[len] = 0;
    return newtarget;
  }
}    
else 
{
  printf("destination '%s' is invalid\n", target);
  return NULL;
}    
}



struct DataNode
{
  char *checksum;
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


void DoWork(struct DataNode *node)
{
  struct DataNode *firstNode = node;
  while (node)
  {
    if (!node->destination || !node->source)
    {
      node = node->next;
      continue;
    }


    struct stat fileDestination;
    struct stat fileSource;
    if (stat(node->source, &fileSource) == -1)
    {
      continue;
    }
    if (stat(node->destination, &fileDestination) == 0 && S_ISREG(fileDestination.st_mode))
    {
     if (fileDestination.st_size == fileSource.st_size)
     {
       char *checksum = (char*)malloc(sizeof(char)*32);
       if (checksum==NULL)
       {
         printf("DoWork: Failed to alloc memory (checksum)\n");
         return;
       }
       node->checksum = (char*)malloc(sizeof(char)*32);
       if (node->checksum==NULL)
       {
         printf("DoWork: Failed to alloc memory (node-checksum)\n");
         return;
       }
       //printf("Comparing Checksums (%s == %s)\n",node->destination, node->source);
      // __asm__("int3");
       md5sum(node->source, fileSource.st_size, node->checksum); 
       md5sum(node->destination, fileDestination.st_size, checksum); 
       if (memcmp(checksum, node->checksum, 32) == 0)
       {
         free(checksum);
         //printf("Skipping %s\n", node->source);
         DrawProgressBar(100, fileDestination.st_size, ffile, '-');
         FreeNodeContents(node);
         continue; 
       }
       free(checksum);
     }
    }

    // check if the destination folder exists
    char *destfolder = dirname(node->destination, strlen(node->destination));
    //printf("DOWORK: destfolder: %s ,%d, (%d)\n",destfolder, strlen(node->destination), GetFileType(destfolder));

    if (stat(destfolder, &fileDestination) == -1)
    {
      //printf("Creating %s\n", destfolder);
      if (mkdir_p(destfolder, strlen(destfolder)) == 0)
      {
        printf("skipping '%s' could not create destination folder\n", node->source);
      }
    }
    free (destfolder);

    if (node->checksum == NULL)
    {
      node->checksum = (char*)malloc(sizeof(char)*32);

      if (node->checksum==NULL)
      {
        printf("DoWork: Failed to alloc memory (node-checksum)\n");
        return;
      }
      //DrawProgressBar(, 962486234, node->source);
      //printf("%s => %s\n", node->source, node->destination);
      md5copy(node->source,  fileSource.st_size, node->destination, node->checksum); 
      //printf("%s => %s (%s)\n", node->source, node->destination, node->checksum);
    }
    else
    {
      rawcopy(node->source, fileSource.st_size, node->destination);
    }
    puts("");
    node = node->next;
  }
  // after copying, check everything
  node = firstNode;
  while (node)
  {
    if (node->checksum != NULL)
    {
      struct stat fileDestination;
      char *checksum = (char*)malloc(sizeof(char)*32);
      if (checksum==NULL)
      {
        printf("DoWork: Failed to alloc memory (checksum)\n");
        return;
      }
  
      if (stat(node->destination, &fileDestination) == 0 && S_ISREG(fileDestination.st_mode))
      {
       md5sum(node->destination, fileDestination.st_size, checksum); 
      }
      if (memcmp(node->checksum, checksum, 32) != 0)
      {
        printf("Checksum for '%s' failed\n", node->source);
      }
      free(checksum);
    }
    struct DataNode *nnode = node->next;
    FreeNodeContents(node);
    free(node);
    node = nnode;
  }
}



struct DataNode* AddFile(char *src, int lsrc, char *path, int lpath, char *dst, int ldst)
{
  char *dstpath = (char*)malloc(sizeof(char*)*(lpath+ldst-lsrc+1));
  if (dstpath==NULL)
  {
    printf("AddFile: Failed to alloc memory (destination path)\n");
    return NULL;
  }
  memcpy(dstpath, dst, ldst);
  memcpy(dstpath+ldst, path+lsrc, lpath-lsrc);
  dstpath[lpath+ldst-lsrc] = 0;

  struct DataNode *node = (struct DataNode*)malloc(sizeof(struct DataNode));
  node->checksum = NULL;
  node->source = (char*)malloc(sizeof(char*)*(lsrc+1));
  if (node->source==NULL)
  {
    printf("AddFile: Failed to alloc memory (source path)\n");
    return NULL;
  }

  memcpy(node->source, path, lpath);
  node->source[lpath] = 0;
  node->destination = dstpath;
  node->next = NULL;
  return node;
}

//  /src/  /src/file1  /dst/

void AddFolder(char* src, int lsrc, char *path, int lpath, char *dst, int ldst)
{
  //printf("Building FileList.(%s)..\n", path);
  struct DataNode *firstNode = NULL;
  struct DataNode *node;
  struct DataNode *newnode;
  DIR           *d;
  struct dirent *dir;
  d = opendir(path);
  if (d)
  {
    char *srcWithSlash = NULL;
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

      //printf("Adding slash to %s\n", src);
    }
    while ((dir = readdir(d)) != NULL)
    {
      int len2 = strlen(dir->d_name);

      if ((len2 == 1 && dir->d_name[0] == '.') || (len2 == 2 && dir->d_name[0] == '.' && dir->d_name[1] == '.'))
      {
        continue;
      }

      unsigned char appendslash = 0;
      int lfolder = lpath+len2+1;
      printf("%d\n", lfolder);
      char *folder = (char*)malloc(sizeof(char)*(lfolder+1));

      if (folder==NULL)
      {
        printf("AddFolder: Failed to alloc memory (destination folder)\n");
        closedir(d);
        return;
      }
      memcpy(folder, path, lpath); 
      if (path[lpath-1] != '/')
      {
        folder[lpath] = '/';
        appendslash = 1;
      }
      else
      {
        lfolder--;
      }
      memcpy(folder+lpath+appendslash, dir->d_name, len2);
      folder[lfolder] = 0;

      struct stat ft;
      stat(folder, &ft);
      if (S_ISDIR(ft.st_mode))
      {
        char *subdst = basename(path, lpath-((appendslash) ? 0 : 1));
        int lsubdst = strlen(subdst);
        int lndst = ldst+lsubdst+1;
        char *ndst = (char*)malloc(sizeof(char*)*(lndst+1));

        if (ndst==NULL)
        {
          printf("AddFolder: Failed to alloc memory (new destination)\n");
          closedir(d);
          return;
        }

        memcpy(ndst, dst, ldst);
        memcpy(ndst+ldst, subdst, lsubdst);
        ndst[lndst-1] = '/';
        ndst[lndst] = 0;
        free(subdst);
        AddFolder((srcWithSlash) ? srcWithSlash : src, lsrc, folder, lfolder, ndst, lndst);
        free(ndst);
      }
      else if (S_ISREG(ft.st_mode))
      {
        newnode = AddFile((srcWithSlash) ? srcWithSlash : src, lsrc, folder, lfolder, dst, ldst);
        if (newnode)
        {
          if (firstNode == NULL)
            firstNode = newnode;
          else
            node->next = newnode;
          node = newnode;
        }
      }
      free(folder);
    }

    if (srcWithSlash != NULL)
    {
      free(srcWithSlash);
    }

    closedir(d);
  }

  //printf("Building FileList.(%s)..Done\n", path);

  //printf("Doing WORK.(%s)..\n", path);
  DoWork(firstNode);
  //printf("Doing WORK.(%s)..Done\n", path);

  // cleanup
  //printf("Cleaning up.(%s)..\n", path);
  /*node = firstNode;
  while (node != NULL) 
  {
    struct DataNode *nnode = node->next;
    FreeNodeContents(node);
    free(node);
    node = nnode;
  }*/
  //printf("Cleaning up.(%s)..Done\n", path);


}



// ecp file file
// ecp file folder    // folder must exist
// ecp file folder/   // folder musn't exist
// ecp file1 file2 folder // folder musn't exist
// ecp file1 file2 folder/ // folder musn't exist
int main (int argc, char **argv)
{
 
  // test progressbar
  /*
  {
    double i;
    for (i = 0; i <= 100; i+=0.5f)
    {
      DrawProgressBar(i, 962486234, argv[1]);
      usleep(10000);
      }
    return 1;
  }*/

    if (argc < 3)
    {
      // todo: basename
      char *exe = basename(argv[0], strlen(argv[0]));
      printf("usage: %s <source> <destination>\n", exe);
      free(exe);
      return 1;
    }

    int i;
    char *target = argv[--argc];

    target = TargetPreCheck(target, argc--);
    if (target == NULL)
    {
      return 1;
    }

    //printf("Building FileList.(Main)..\n");
    int ltarget = strlen(target);

    struct DataNode *firstNode = NULL;
    struct DataNode *node;
    struct DataNode *newnode;
    for (i = 0; i < argc; i++)
    {
      struct stat ft;
      if (stat(argv[i+1], &ft) == -1)
      {
        continue;
      }
      int len = strlen(argv[i+1]);
      if (S_ISDIR(ft.st_mode))
      {
        AddFolder(argv[i+1], len, argv[i+1], len, target, ltarget);
      }
      else if(S_ISREG(ft.st_mode))
      {
        char *sdir = dirname(argv[i+1], len);
        newnode = AddFile(sdir, strlen(sdir), argv[i+1], len, target, ltarget);
        free(sdir);
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
    //printf("Building FileList.(Main)..Done\n");

    //printf("Doing WORK.(Main)..\n");
    DoWork(firstNode);
    //printf("Doing WORK.(Main)..Done\n");

  // cleanup
    //printf("Cleaning up.(Main)..\n");
    /*node = firstNode;
    while (node != NULL) 
    {
      struct DataNode *nnode = node->next;
      FreeNodeContents(node);
      free(node);
      node = nnode;
    }*/
    //printf("Cleaning up.(Main)..Done\n");
    free(target);
    return 0;
  }

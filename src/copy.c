#include <stdio.h>
#include <unistd.h>
#include "md5.h"
#include "fadvise.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> 
#include <string.h>
#include <stdlib.h>



void DrawProgressBar(double percent, double speed, char *file);
char *basename(char *in, int len);


int fSize;
int wSize;
char *ffile;

void copy_process_block (const void *buffer, size_t len, FILE* dststream)
{
  if (dststream != NULL)
  {
    if (fwrite(buffer, len, 1, dststream) < 0 )
    {
      printf("error on wrting to destination\n");
    }
  }
  wSize+=len;
  DrawProgressBar((double)100*wSize/fSize, 0.0, ffile);

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



unsigned char rawcopy(char *src, char *dst)
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

  fseek (fp , 0 , SEEK_END);
  fSize = ftell (fp);
  rewind (fp);
  ffile = src;

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
      copy_process_block(buffer,numr, fpdst);
    } 
    DrawProgressBar(100, 0.0, src);


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


  unsigned char md5copy(char *src, char *dst, char *checksum)
  {

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

    ffile = src;
    fseek (fp , 0 , SEEK_END);
    fSize = ftell (fp);
    rewind (fp);
    wSize = 0;


  }
  else
  {
    fpdst = NULL;
    fSize = -1;
  }


  fadvise (fp, FADVISE_SEQUENTIAL);

  err = md5_stream(fp, fpdst, bin_buffer);
  if (err)
  {
    printf("error on copy: %s", src);
    fclose (fp);
    return 0;
  }
  if (fpdst)
  {
    DrawProgressBar(100, 0.0, src);
  }

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

unsigned char md5sum(char *src, char *checksum)
{
  return md5copy(src, NULL, checksum);
}

int GetFileType(char *target)
{
  struct stat st;
  if(stat(target,&st) == 0)
  {
    if(S_ISDIR(st.st_mode))
    {
     return -1; // folder
   }
   else if(S_ISREG(st.st_mode))
   {
     return st.st_size; // file
   }
   else
    return -2; // unknown file
 }
 return -3; // stat failed
}

unsigned char mkdir_p(char *folder, size_t len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    if (folder[i] == '/')
    {
      char *pst = (char*)malloc(sizeof(char)*(i+1));
      memcpy(pst, folder, i);
      pst[i] = 0;
      if (GetFileType(pst) == -3)
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
  
  int FileType = GetFileType(target);
  if (FileType == -3)
  {

    // ecp file1 file2 folder(/)
    if (numfiles > 1)
    {
      char *newtarget;

      if (target[len-1] != '/')
      {
        newtarget = (char*)malloc(sizeof(char)*(len+2));
        memcpy(newtarget, target, len);
        newtarget[len] = '/';
        newtarget[++len] = 0;
      }
      else
      {
       newtarget = (char*)malloc(sizeof(char)*(len+1));
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
  memcpy(newtarget, target, len);
  newtarget[len] = 0;
  return newtarget;
}
}
else if (FileType == -1)
{
 if (target[len-1] != '/')
 {
  char *newtarget = (char*)malloc(sizeof(char)*(len+2));
  memcpy(newtarget, target, len);
  newtarget[len] = '/';
  newtarget[len+1] = 0;
  return newtarget;
}
else
{
  char *newtarget = (char*)malloc(sizeof(char)*(len+1));
  memcpy(newtarget, target, len);
  newtarget[len] = 0;
  return newtarget;
}
}  
else if (FileType > 0)
{
  if (numfiles > 1)
  {
    printf("destination '%s' is not a directory\n", target);
    return NULL;
  }
  else
  {
    char *newtarget = (char*)malloc(sizeof(char)*(len+1));
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



char *SetTarget(char *target, char *dstname)
{
  int len = strlen(target);
  if (target[len-1] != '/')
  {

   char *newtarget = (char*)malloc(sizeof(char)*(len+1));
   memcpy(newtarget, target, len);
   newtarget[len] = 0;
   return newtarget;
 }
 else
 {
   int dlen = strlen(dstname);
   char *newtarget = (char*)malloc(sizeof(char)*(len+dlen+1));
   memcpy(newtarget, target, len);
   memcpy(newtarget+len, dstname, dlen);
   newtarget[len+dlen] = 0;
   return newtarget;
 }
}

struct DataList
{
  char *checksum;
  char *source;
  char *destination;
};

void FreeDataSet(struct DataList *datalist, int i)
{
  if (datalist[i].source != NULL)
  {
    free(datalist[i].source);
    datalist[i].source = NULL;
  }
  if (datalist[i].destination != NULL)
  {
    free(datalist[i].destination);
    datalist[i].destination = NULL;
  }
  if (datalist[i].checksum != NULL)
  {
    free(datalist[i].checksum);
    datalist[i].checksum = NULL;
  }

}

/*
void DoWork()
{
  struct DataList *datalist = (struct DataList*)malloc(sizeof(struct DataList)*argc);
  
  for (i = 0; i < argc; i++)
  {
   int len = strlen(argv[i+1]);
   datalist[i].source = (char*)malloc(sizeof(char)*(len+1));
   memcpy(datalist[i].source, argv[i+1], len);
   datalist[i].source[len] = 0;

   char *dstname = basename(datalist[i].source, len);
   datalist[i].destination = SetTarget(target,dstname);
   free(dstname);

   datalist[i].checksum = NULL;
   int filesize = GetFileType(datalist[i].destination);
   if (filesize > 0)
   {

    if (GetFileType(datalist[i].source) == filesize)
    {
      char *checksum = (char*)malloc(sizeof(char)*32);
      datalist[i].checksum = (char*)malloc(sizeof(char)*32);
      //printf("Comparing Checksums (%s)\n",datalist[i].destination);
      md5sum(datalist[i].source, datalist[i].checksum); 
      md5sum(datalist[i].destination, checksum); 

      if (memcmp(checksum, datalist[i].checksum, 32) == 0)
      {
        free(checksum);
        printf("Skipping %s\n", datalist[i].source);
        FreeDataSet(datalist, i);
        continue; 
      }
      free(checksum);
    }
  }

  if (datalist[i].checksum == NULL)
  {
    datalist[i].checksum = (char*)malloc(sizeof(char)*32);
    md5copy(datalist[i].source,  datalist[i].destination, datalist[i].checksum); 
  }
  else
  {
    rawcopy(datalist[i].source,  datalist[i].destination);
  }
  puts("");
}

for (i = 0; i < argc; i++)
{
 if (datalist[i].checksum != NULL)
 {
   printf("\rComparing Checksums (%s)",datalist[i].destination);
   char *checksum = (char*)malloc(sizeof(char)*32);
   md5sum(datalist[i].destination, checksum); 
   if (memcmp(datalist[i].checksum, checksum, 32) != 0)
   {
      printf("Checksum for '%s' failed\n", datalist[i].source);
   }
   free(checksum);
   FreeDataSet(datalist, i);
 }
}
}*/



void AddFile(char *src, int lsrc, char *path, int lpath, char *dst, int ldst)
{

  char *dstpath = (char*)malloc(sizeof(char*)*(lpath-lsrc+ldst+1));
  memcpy(dstpath, dst, ldst);
  memcpy(dstpath+ldst, path+lsrc, lpath-lsrc);
  dstpath[lpath-lsrc+ldst] = 0;
  printf("Adding File: %s => %s\n", path, dstpath);
  free(dstpath);
}

void AddFolder(char* src, int lsrc, char *path, int lpath, char *dst, int ldst)
{
  DIR           *d;
  struct dirent *dir;
  d = opendir(path);
  if (d)
  {
    char *srcWithSlash = NULL;
    if (src[lsrc-1] != '/')
    {
        srcWithSlash = (char*)malloc(sizeof(char*)*(lsrc+2));
        memcpy(srcWithSlash, src, lsrc);
        srcWithSlash[lsrc] = '/';
        srcWithSlash[++lsrc] = 0;
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
      char *folder = (char*)malloc(sizeof(char)*(lfolder+1));
      memcpy(folder, path, lpath); 
      if (folder[lpath-1] != '/')
      {
        folder[lpath] = '/';
        appendslash = 1;
      }
      memcpy(folder+lpath+appendslash, dir->d_name, len2);
      folder[lpath+appendslash+len2] = 0;
      int ft = GetFileType(folder);
      if (ft == -1)
      {
          AddFolder((srcWithSlash) ? srcWithSlash : src, lsrc, folder, lfolder, dst, ldst);
      }
      else
      {
          AddFile((srcWithSlash) ? srcWithSlash : src, lsrc, folder, lfolder, dst, ldst);
      }
      free(folder);
    }

    if (srcWithSlash != NULL)
    {
      free(srcWithSlash);
    }

    closedir(d);
  }


}



// ecp file file
// ecp file folder    // folder must exist
// ecp file folder/   // folder musn't exist
// ecp file1 file2 folder // folder musn't exist
// ecp file1 file2 folder/ // folder musn't exist



int main (int argc, char **argv)
{ 

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

  int ltarget = strlen(target);

  //std::list<std::string> files;
  for (i = 0; i < argc; i++)
  {
    //printf("F: %s\n", argv[i+1]);
    int ft = GetFileType(argv[i+1]);
    int len = strlen(argv[i+1]);
    if (ft == -1)
    {

        AddFolder(argv[i+1], len, argv[i+1], len, target, ltarget);
    }
    else
    {
        AddFile(argv[i+1], len, argv[i+1], len, target, ltarget);
    }
    
  }




  //free(datalist);
  free(target);
  return 0;
}

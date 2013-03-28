#include <stdio.h>
#include <unistd.h>
#include "md5.h"
#include "fadvise.h"

unsigned char copy(char *src, char *dst, unsigned char *md5_result)
{


  FILE *fp;
  int err;

  fp = fopen (src, "rb");
  if (fp == NULL)
  {
  	printf("error on opening: %s", src);
  	return 0;
  }
  fadvise (fp, FADVISE_SEQUENTIAL);

  err = md5_stream(fp, md5_result);
  if (err)
  {
    printf("error on copy: %s", src);
   	fclose (fp);
    return 0;
  }

  if (fclose (fp) != 0)
  {
    printf("error on close: %s", src);
    return 0;
  }

  return 1;
}

#define DIGEST_BITS 128
#define DIGEST_ALIGN 4
#define DIGEST_HEX_BYTES (DIGEST_BITS / 4)
#define DIGEST_BIN_BYTES (DIGEST_BITS / 8)

static inline void *
ptr_align (void const *ptr, size_t alignment)
{
  char const *p0 = ptr;
  char const *p1 = p0 + alignment - 1;
  return (void *) (p1 - (size_t) p1 % alignment);
}

int main (int argc, char **argv) {

	unsigned char bin_buffer_unaligned[DIGEST_BIN_BYTES + DIGEST_ALIGN];
    unsigned char *bin_buffer = ptr_align (bin_buffer_unaligned, DIGEST_ALIGN);

    copy(argv[1],argv[2], bin_buffer);
    //printf("md5: %s\n", md5_result);
    int i;
 	for (i = 0; i < DIGEST_HEX_BYTES / 2 ; ++i)
                printf ("%02x", bin_buffer[i]);
    return 0;
}

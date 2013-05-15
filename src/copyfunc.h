#ifndef false
	#define false 0
#endif
#ifndef true
	#define true 1
#endif
#ifndef bool
	#define bool unsigned char
#endif

bool rawcopy(char *src, char *dst, unsigned char **checksum);
bool md5copy(char *src, char *dst, unsigned char **checksum);
bool md5sum(char *src, unsigned char **checksum);
bool md5cmp(unsigned char *checksum1, unsigned char *checksum2);
void md5print(unsigned char *checksum);

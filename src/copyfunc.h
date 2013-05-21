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


bool sha1copy(char *src, char *dst, unsigned char **checksum);
bool sha1sum(char *src, unsigned char **checksum);
bool sha1cmp(unsigned char *checksum1, unsigned char *checksum2);
void sha1print(unsigned char *checksum);

bool sha1copy(char *src, char *dst, unsigned char **checksum);
bool sha1sum(char *src, unsigned char **checksum);
bool sha1cmp(unsigned char *checksum1, unsigned char *checksum2);
void sha1print(unsigned char *checksum);

bool sha224copy(char *src, char *dst, unsigned char **checksum);
bool sha224sum(char *src, unsigned char **checksum);
bool sha224cmp(unsigned char *checksum1, unsigned char *checksum2);
void sha224print(unsigned char *checksum);

bool sha256copy(char *src, char *dst, unsigned char **checksum);
bool sha256sum(char *src, unsigned char **checksum);
bool sha256cmp(unsigned char *checksum1, unsigned char *checksum2);
void sha256print(unsigned char *checksum);

bool sha384copy(char *src, char *dst, unsigned char **checksum);
bool sha384sum(char *src, unsigned char **checksum);
bool sha384cmp(unsigned char *checksum1, unsigned char *checksum2);
void sha384print(unsigned char *checksum);

bool sha512copy(char *src, char *dst, unsigned char **checksum);
bool sha512sum(char *src, unsigned char **checksum);
bool sha512cmp(unsigned char *checksum1, unsigned char *checksum2);
void sha512print(unsigned char *checksum);

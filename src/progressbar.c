#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>



int getWidth()
{
	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

void GetSpeed(unsigned int bytes, char *buffer)
{
	char unit[5] = "B/s";
	if (bytes > 1024)
	{
		bytes = bytes / 1024;
		strcpy(unit, "KB/s");
	}
	if (bytes > 1024)
	{
		bytes = bytes / 1024;
		strcpy(unit, "MB/s");
	}
	if (bytes > 1024)
	{
		bytes = bytes / 1024;
		strcpy(unit, "GB/s");
	}
	if (bytes > 1024)
	{
		bytes = bytes / 1024;
		strcpy(unit, "TB/s");
	}
	memset(buffer, 0, 9);
	if (strlen(unit) == 3)
		sprintf(buffer, " %4d%s", bytes, unit);
	else
		sprintf(buffer, "%4d%s", bytes, unit);
}



char *basename(char *in, int len)
{
	int i;
	for (i = len - 1; i >= 0; i--)
	{
		if (in[i] == '/')
		{
			i++;
			char *ret = (char*)malloc(sizeof(char)*(len-i+1));
			memcpy(ret, in+i, len-i);
			ret[len-i] = 0;
			return ret;
		}
	}
	char *ret = (char*)malloc(sizeof(char)*(len+1));
	memcpy(ret, in, len);
	ret[len] = 0;
	return ret;
}

char *dirname(char *in, int len)
{
	int i;
	for (i = len - 1; i >= 0; i--)
	{
		if (in[i] == '/')
		{
			i++;
			char *ret = (char*)malloc(sizeof(char)*(i+1));
			memcpy(ret, in, i);
			ret[i] = 0;
			return ret;
		}
	}
	char *ret = (char*)malloc(sizeof(char)*(len+1));
	memcpy(ret, in, len);
	ret[len] = 0;
	return ret;
}


void DrawProgressBar(double percent, unsigned int speed, char *file, char progresssign)
{
	int width = getWidth();
	char speedbuffer[9];
	GetSpeed(speed, speedbuffer);


	int flen = strlen(file);

    //|01234567890123456789|
    //|01234 1023KB/s 99.9%|
    unsigned char showstats = 1;
	int width_of_file_and_bar = width - 16; // 16 = ' 9999MB/s 100.0%'


	int width_of_file = (width_of_file_and_bar*35/100); // File has 35 Percent of space
	int width_of_bar = width_of_file_and_bar - width_of_file - 1; // Bar has the rest
	if (width_of_bar < 12 || width_of_file < 10) // if Bar is less then 12 hide the bar or file is too small
	{
		width_of_file = width_of_file_and_bar;
		width_of_bar = 0;
	}

	if (width_of_file < 10)
	{
		width_of_file = width;
		showstats = 0;
	}


	if (width_of_file < flen)  // if the file is greather then the width that is reserved, get the basename
	{
		char *filebuffer = basename(file, flen);
		flen = strlen(filebuffer);
		if (width_of_file < flen)	// still longer cut off
		{
			filebuffer[width_of_file] = 0;
			flen = width_of_file;
		}

		printf("\r%s", filebuffer);
		free(filebuffer);

		while (flen < width_of_file)
		{
			putchar(' ');
			flen++;
		}
	}
	else
	{
		printf("\r%s", file);

		while (flen < width_of_file)
		{
			putchar(' ');
			flen++;
		}

	}

	if (showstats)
	{	
		if (width_of_bar > 0)
		{
			int i;
			int p = (width_of_bar*percent/100);
			fputs("  [", stdout);

			for (i = 3; i < p; i++)
			{
				putchar(progresssign);	
			}
			if (p < width_of_bar)
			{
				putchar('>');	
				for (i=i+1; i < width_of_bar; i++)
				{
					putchar(' ');	
				}
			}
			putchar(']');
		}

		printf(" %s %5.1f%%", speedbuffer, percent);
		fflush(stdout);
	}
	
}

/*
int main (int argc, char **argv) {
	double i;
	//printf("\e[?25l");
	//puts("");
	//srand (time(NULL));
	//DrawProgressBar(99.9, 962486234, "/usr/bin/path/blup");
	for (i = 0; i <= 100; i+=0.5f)
	{
		DrawProgressBar(i, 962486234, "/usr/bin/path/blup");
		usleep(10000);
    }
	
    //printf("\e[?25h");
    return 0;
}
*/

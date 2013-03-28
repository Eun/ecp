#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>



int getWidth()
{
	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

void RevertLines(int count)
{
	int i;
	for (i = 0; i < count; i++) {
		putchar(0x1B);
		fputs("[1A", stdout); // up
		putchar(0x1B);
		fputs("[2K", stdout); // clear
	}
}

void DrawLines(char *line1, char *line2)
{
	RevertLines(2);
	puts(line1);
	puts(line2);
}

void GetSpeed(int bytes, char *buffer)
{
	char unit[] = "B/s ";
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
	memset(buffer, 0, 8);
	sprintf(buffer, "%4d%s", bytes, unit);
}



char *basename(char *in, int len)
{
	int i;
	for (i = len - 1; i >= 0; i--)
	{
		if (in[i] == '/')
		{
			char *ret = (char*)malloc(len-i);
			memcpy(ret, in+i, len-i);
			return ret;
		}
	}
	char *ret = (char*)malloc(len);
	memcpy(ret, in, len);
	return ret;
}

void DrawProgressBar(double percent, double speed, char *file)
{
	int width = getWidth();

	char speedbuffer[8];
	GetSpeed(speed, speedbuffer);

	int flen = strlen(file);

    //|01234567890123456789|
    //|01234 1023KB/s 99.9%|
	int width_of_file_and_bar = width - 16;

	int width_of_file = (width_of_file_and_bar*2/10);
	int width_of_bar = width_of_file_and_bar - width_of_file;
	if (width_of_bar < 13)
	{
		width_of_file = width_of_file_and_bar;
		width_of_bar = 0;
	}
	if (width_of_file < flen)
	{
		char *filebuffer = basename(file, flen);
		if (width_of_file < strlen(filebuffer))
		{
			filebuffer[width_of_file] = 0;
		}
		printf("\r%s", filebuffer);
	}
	else
	{
		printf("\r%s", file);
	}

	if (width_of_bar > 0)
	{
		int i;
		int p = (width_of_bar*percent/100);
		fputs(" [", stdout);

		for (i = 3; i < p; i++)
		{
			putchar('=');	
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

	printf(" %s %5.1f%% ", speedbuffer, percent);
	fflush(stdout);

}

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

#include <stdio.h>
#include <string.h>

void prompt(char *p, int fd)
/* 0040138c void (char*, int) */
/* 0040138c void (char*, int) */
{
	write(fd, p, strlen(p));
}

void getInput(int fd, char *p, size_t l)
/* 004013b5 void (int, char*, unsigned int) */
/* 004013b5 void (int, char*, unsigned int) */
{
	read(fd, p, l);
	p = strchr(p, '\n');
	if (p != NULL)
		*p = '\0';
}

static char const * const mask = "spiderman";
/* 00404070: 32bit (00404064) */
/* 00404070: char* (00404064) */

size_t encrypt(char *e, char *p)
/* 004013f8 char* (32bit, char*) */
/* 004013f8 unsigned int (char*, char*) */
{
	char const *x = mask;
	size_t len = strlen(p);

	while (*p != '\0')
	{
		if (*x == '\0')
			x = mask;

		*e++ = *p++ ^ *x++;
	}
	return len;
}

void decrypt(char *p, char *e, size_t len)
/* 0040144f void (32bit, 32bit, 32bit) */
/* 0040144f void (char*, char*, int) */
{
	char const *x = mask;
	size_t i = 0;

	while (i < len)
	{
		if (*x == '\0')
			x = mask;

		*p++ = *e++ ^ *x++;
		i++;
	}
}

void convertToReadable(char *p, char *e, int len)
/* 0040149e void (char*, 32bit, char*) */
/* 0040149e void (char*, char*, int) */
{
	while (len--)
	{
		sprintf(p, "%02x", *e++);
		p += 2;
	}

	*p = '\0';
}

void putOutput(char *p, int fd)
/* 004014e2 void (char*, int) */
/* 004014e2 void (char*, int) */
{
	write(fd, p, strlen(p));
}

void writeToFile(char *f, char *p, size_t len)
/* 0040150b void (char*, 32bit, char*) */
/* 0040150b void (char*, char*, unsigned int) */
{
	FILE *fp = fopen(f, "wb");
	int i;

	for (i = 0; i < len; i++)
		putc(p[i], fp);
	fclose(fp);
}

size_t readFromFile(char *f, char *p)
/* 00401564 32bit (char*, 32bit) */
/* 00401564 int (char*, char*) */
{
	size_t len = 0;
	int c;
	FILE *fp = fopen(f, "rb");
	if (fp == NULL){
		printf("File could not be opened.");
		exit(1);
	}

	while ((c = getc(fp)) != EOF)
		p[len++] = c;
	fclose(fp);

	return len;
}

/*
* One argument.  If it is D or d, decryption is done.  If it is E or e, encryption is done.
*/
int main(int argc, const char* argv[] )
/* 004015d9 NTSTATUS (32bit, 32bit) */
/* 004015d9 NTSTATUS (unsigned int, [char**, int**]) */
{
	char pbuff[1024], ebuff[1024], xbuff[1024];
	char filename[1024];
	size_t elen = -1;

	if (argc != 2){
		printf("Usage: e D|d|E|e   Decryption is selected by D or d.  Encryption by E or e.");
		exit(1);
	}

	if (strcmp(argv[1],"E") && strcmp(argv[1],"e") && strcmp(argv[1],"D") && strcmp(argv[1],"d")){
		printf("Unknown argument");
		exit(1);
	}

	memset(filename, '\0', sizeof(filename));

	prompt("Enter a filename: ", 1);
	getInput(0, filename, sizeof(filename));
	strcat(filename, ".txt");

	if (strlen(filename) < 1){
		printf("No name entered.");
		exit(1);
	}

	if (!strcmp(argv[1],"E") || !strcmp(argv[1],"e")){
		memset(pbuff, '\0', sizeof(pbuff));
		memset(ebuff, '\0', sizeof(ebuff));
		/* ------------------------ */
		prompt("Enter a string: ", 1);
		getInput(0, pbuff, sizeof(pbuff));

		elen = encrypt(ebuff, pbuff);

		writeToFile(filename, ebuff, elen);
	} else if (!strcmp(argv[1],"D") || !strcmp(argv[1],"d")){
		/* ------------------------ */
		/* debug - set all to 0 */
		memset(pbuff, '\0', sizeof(pbuff));
		memset(ebuff, '\0', sizeof(ebuff));

		elen = 0;

		/* ------------------------ */
		elen = readFromFile(filename, ebuff);
		decrypt(pbuff, ebuff, elen);
	} 


	memset(xbuff, '\0', sizeof(xbuff));
	convertToReadable(xbuff, ebuff, elen);

	putOutput("<", 1);
	putOutput(filename, 1);
	putOutput("> encrypted=<", 1);
	putOutput(ebuff, 1);
	putOutput("> plainText=<", 1);
	putOutput(pbuff, 1);
	putOutput(">\n", 1);

	return 0;
}

// file format gleaned from https://github.com/OpenRA/OpenRA/blob/629fe95ebdfd669d04af5cdbdb900bf09e08e39e/OpenRA.FileFormats/Filesystem/InstallShieldPackage.cs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/utime.h>

#include "blast.h"

typedef struct direntry_s
{
	char *name;
	int numFiles;
}
direntry_t;

typedef struct fchunk_s
{
	FILE *fp;
	unsigned int size;
}
fchunk_t;

#if defined(SHOWTIME)
char *months[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
#endif

#define GET_UI32(x) ((x)[0] | ((x)[1] << 8) | ((x)[2] << 16) | ((x)[3] << 24))
#define GET_UI16(x) ((x)[0] | ((x)[1] << 8))

unsigned inf(void *how, unsigned char **buf)
{
	static unsigned char hold[16384];
	fchunk_t *fc = how;
	int numBytes;

	*buf = hold;

	numBytes = fc->size;
	if (numBytes == 0) return -1;
	if (numBytes > 16384) numBytes = 16384;

	fc->size -= numBytes;

	return fread(hold, 1, numBytes, fc->fp);
}

int outf(void *how, unsigned char *buf, unsigned len)
{
	return fwrite(buf, 1, len, (FILE *)how) != len;
}

int unZ(char *archiveFilename, char *extractPath)
{
	FILE *fp;
	fchunk_t fc;

	unsigned int i, destsize;
	unsigned char header[256], *toc;

	unsigned int numFiles, compsize, tocOffset, tocSize, numDirs;
	direntry_t *dirs;

	destsize = strlen(extractPath);

	fp = fopen(archiveFilename, "rb");

	if (!fp)
	{
		printf("Error opening %s!\n", archiveFilename);
		return;
	}

	fseek(fp, 0, SEEK_END);
	compsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	fread(header, 256, 1, fp);

	// check signature
	if (GET_UI32(header) != 0x8C655D13)
	{
		printf("Archive corrupt: Bad signature!\n");
		return 0;
	}

	//numFiles = GET_UI16(header + 12);

	// check size
	if (compsize != GET_UI32(header + 18))
	{
		printf("Archive corrupt: Incorrect size!\n");
		return 0;
	}

	// read toc
	tocOffset = GET_UI32(header + 41);
	tocSize = compsize - tocOffset;

	toc = malloc(tocSize);
	fseek(fp, tocOffset, SEEK_SET);
	fread(toc, 1, tocSize, fp);
	tocOffset = 0;


	// parse dirs
	numDirs = GET_UI16(header + 49);
	dirs = malloc(sizeof(*dirs) * numDirs);

	for (i = 0; i < numDirs; i++)
	{
		unsigned int dir_numFiles  = GET_UI16(toc + tocOffset);
		unsigned int dir_entrySize = GET_UI16(toc + tocOffset + 2);
		unsigned int dir_nameSize  = GET_UI16(toc + tocOffset + 4);

		dirs[i].name = malloc(destsize + 1 + dir_nameSize + 1);
		strcpy(dirs[i].name, extractPath);
		dirs[i].name[destsize] = '\\';
		memcpy(dirs[i].name + destsize + 1, toc + tocOffset + 6, dir_nameSize);
		dirs[i].name[destsize + 1 + dir_nameSize] = '\0';
		dirs[i].numFiles = dir_numFiles;

		tocOffset += dir_entrySize;
	}

	// parse and save files
	fseek(fp, 255, SEEK_SET);
	fc.fp = fp;
	for (i = 0; i < numDirs; i++)
	{
		unsigned int dir_nameSize;
		int j;

		_mkdir(extractPath);
		if (dirs[i].name[0])
			_mkdir(dirs[i].name);

		printf("Dir %s, %d files\n", dirs[i].name, dirs[i].numFiles);
		dir_nameSize = strlen(dirs[i].name);

		for (j = 0; j < dirs[i].numFiles; j++)
		{
			FILE *fpw;
			int blastErr;

			unsigned int file_decompressedSize = GET_UI32(toc + tocOffset + 3);
			unsigned int file_compressedSize   = GET_UI32(toc + tocOffset + 7);
			unsigned int file_modifiedDate     = GET_UI16(toc + tocOffset + 15);
			unsigned int file_modifiedTime     = GET_UI16(toc + tocOffset + 17);
			unsigned int file_entrySize        = GET_UI16(toc + tocOffset + 23);
			unsigned int file_nameSize         = toc[tocOffset + 29];

			char *filename;

			struct tm tmm;
			struct _utimbuf ut;

			filename = malloc(dir_nameSize + file_nameSize + 2);
			strcpy(filename, dirs[i].name);
			filename[dir_nameSize] = '\\';
			strncpy(filename + dir_nameSize + 1, toc + tocOffset + 30, file_nameSize);
			filename[dir_nameSize + 1 + file_nameSize] = '\0';

			printf("%s, %d bytes, %.3f%%\n", filename, file_decompressedSize, (float)file_compressedSize / file_decompressedSize * 100.0f);

			memset(&tmm, 0, sizeof(tmm));
			tmm.tm_year = ( file_modifiedDate           >> 9) + 80;
			tmm.tm_mon  = ((file_modifiedDate & 0x01E0) >> 5) - 1;
			tmm.tm_mday =   file_modifiedDate & 0x001F;
			tmm.tm_hour =   file_modifiedTime           >> 11;
			tmm.tm_min  =  (file_modifiedTime & 0x07E0) >> 5;
			tmm.tm_sec  =  (file_modifiedTime & 0x001F) * 2;

#if defined(SHOWTIME)
			printf("last modified %d %s %d %02d:%02d:%02d\n", 
				tmm.tm_year + 1900, months[tmm.tm_mon], tmm.tm_mday, tmm.tm_hour, tmm.tm_min, tmm.tm_sec);
#endif

			tocOffset += file_entrySize;

			fpw = fopen(filename, "wb");
			fc.size = file_compressedSize;

			blastErr = blast(inf, &fc, outf, fpw);

			if (blastErr == 1)
				printf("Error writing file!\n");
			else if (blastErr != 0)
				printf("Archive corrupt: Error decompressing, trying to continue");

			fclose(fpw);

			ut.actime = time(NULL);
			ut.modtime = mktime(&tmm);
			_utime(filename, &ut);

			free(filename);
		}
	}

	fclose(fp);

	for (i = 0; i < numDirs; i++)
		free(dirs[i].name);

	free(dirs);
	free(toc);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("Usage:\n  %s <input file> <output directory>\n", argv[0]);
		return 0;
	}
	
	return unZ(argv[1], argv[2]);
}

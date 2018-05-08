/*--------------------------------------------------------------------------*/
/*  hw2.c 																*/
/*--------------------------------------------------------------------------*/
/*																			*/
/*  Created by Ahmet Mert Gülbahçe on 08.03.2017.							*/
/*																			*/
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>


void readFromFile(FILE *log, const char *stringToFind, const char *fileName, const char *fname);
void searchDirectory(FILE *log, const char *stringToFind, const char *dirName);


int main(int argc, const char *argv[])
{
	FILE *log = fopen("log.txt","a");

	if(argc != 3)
	{
		printf("Usage: %s string path\n",argv[0]);
		fprintf(log,"Usage: %s string path\n",argv[0]);
		return -1;
	}

	if(isdirectory(argv[2]) == 0)
	{
		perror("Lütfen geçerli bir klasör yolu giriniz.");
		fprintf(log,"Lütfen geçerli bir klasör yolu giriniz.\n");
		return -2;
	}

	searchDirectory(log,argv[1],argv[2]);
	fprintf(log,"\nToplam %s bulundu.\n",argv[1]);

	fclose(log);

	return 0;
}

void readFromFile(FILE *log, const char *stringToFind, const char *fileName, const char *fname)
{
	FILE *fp;
	char c;
	char bs = 8;
	int length = strlen(stringToFind); /* bulunacak stringin uzunluğu */
	int rowCount = 1, columnCount = 1,  /* satır sütun için counter */
		foundRow, foundColomn, /* bulunduğu konum bilgileri */
		totalCount = 0, /* bulunan toplam string */
		skippedCharacters = 0; /* atlanan karakterler için counter */
	char *tmpStr; /* karşılaştırmak için kullanılan geçici string */
	int isEof = 0; /* eof kontrolcüsü */
	int strLength = 0;/* geçici stringin uzunluğu */

	if( (fp = fopen(fileName,"r")) == NULL )
	{
		return;
	}

	while(isEof != 1) 
	{
		c = getc(fp);

		if(c == EOF)
			isEof = 1;;

		/* okunan karakter tab, new line ve space değilse */
		if (c != 9 && c != 10 && c != 32)
		{
			/* string boş ise yeni yer açılır ve bulunduğu konum kaydedilir */
			if(strLength == 0)
			{
				tmpStr = (char *) malloc(length*sizeof(char));
				foundRow = rowCount;
				foundColomn = columnCount;
			}
			tmpStr[strLength] = c;
			strLength++;
			columnCount++;
		}
		else
		{
			/* alt satıra geçtiyse */
			if(c == 10)
			{
				rowCount++;
				columnCount = 1;
			}
			else
				columnCount++;;

			if(strLength != 0)
				skippedCharacters++;
		}
		/* uzunluklar eşitse */
		if(strLength == length)
		{
			/* string kıyaslaması yapılır, eşitse konumlar yazılır */
			if(strcmp(tmpStr,stringToFind) == 0)
			{
				fprintf(log,"%s: [%d, %d] konumunda ilk karakter bulundu.\n",fname,foundRow,foundColomn);
				totalCount++;
			}
			strLength = 0;
			
			/* bulunduğu konumdan sonra tekrar aramaya geri döner */
			fseek(fp, -1*sizeof(char)*(length+skippedCharacters-1), SEEK_CUR);
			
			if(columnCount <= length+skippedCharacters && rowCount != 1)
				rowCount = foundRow;
				
			columnCount = foundColomn+1;

			skippedCharacters = 0;
			free(tmpStr);
		}
	}

	fclose(fp);

}

/*
Referans: Ders kitabı örnek kodu
shownames.c
*/
void searchDirectory(FILE *log, const char *stringToFind, const char *dirName)
{
	struct dirent *direntp;
	DIR *dirp;
	char *nextPath;
	pid_t pid;

	if ((dirp = opendir(dirName)) != NULL)
	{
		while ((direntp = readdir(dirp)) != NULL)
		{
			if(strcmp(direntp->d_name,".") != 0 && strcmp(direntp->d_name,"..") != 0)
			{
				nextPath = (char *) malloc(strlen(dirName)+1+strlen(direntp->d_name));
				strcpy(nextPath,dirName);strcat(nextPath,"/");strcat(nextPath,direntp->d_name);
				printf("%s\n", nextPath);
				if(isdirectory(nextPath) == 0)
				{
					pid = fork();
					if(pid == -1)
					{
						perror("Fork yapılamadı.");
					}
					if(pid == 0)
					{
						readFromFile(log,stringToFind,nextPath,direntp->d_name);
						exit(0);
					}
					else
						wait(NULL);
				}
				else
				{
					pid = fork();
					if(pid == -1)
					{
						perror("Fork yapılamadı.");
					}
					if(pid == 0)
					{
						searchDirectory(log,stringToFind,nextPath);
						exit(0);
					}
					else
						wait(NULL);
				}
				free(nextPath);
			}
			
		}
		while ((closedir(dirp) == -1) && (errno == EINTR));

	}
}

/*
Referans: Ders kitabı örnek kodu
isdirectory.c
*/
int isdirectory(char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) == -1)
		return 0;
	else
		return S_ISDIR(statbuf.st_mode);
}
